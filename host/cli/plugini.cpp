/**
 *  Load custom plugins as Peripherals or BusPeripherals.
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2019
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dlfcn.h>

#include "BusPeripheral.h"
#include "plugin.h"
#include "plugini.h"
#include "ll.h"
#include "err.h"
#include "log.h"

#include "sysmap_parse.h"

#define SZ_BYTE   0
#define SZ_HWRD   1
#define SZ_WORD   2
#define WRITE     (1 << 3)
#define FAIL      1
#define SUCCESS   0

// These match defs in host_fifo_pkg.sv
#define READ_BYTE (1 << 4)
#define READ_HWRD (2 << 4)
#define READ_WORD (3 << 4)

typedef struct {
  list_t    list;
  int       refcnt;
  char     *name;
  void     *shlib;
  plugin_t *plugin;
} plugin_list_t;

// Linked list head
static list_t plist;
static char **search;
static int scnt;

// Array of root BusPeripheral plugins
static BusPeripheral **plugin;
static int pcnt;

// Pointer to target
Target *target;

static void *plugin_open (const char *name)
{
  int i;
  void *hdl;
  
  // Search through search path
  for (i = 0; i < scnt; i++) {

    // Skip empty path
    if (!search[i])
      continue;
    
    // Malloc space
    // path + / + lib + name + .so + \0
    char *path = (char *)malloc (strlen (name) + strlen (search[i]) + 8);
    if (!path)
      return NULL;
    
    // Create full path
    strcpy (path, search[i]);
    strcat (path, "/lib");
    strcat (path, name);
    strcat (path, ".so");

    // Try to open
    hdl = dlopen (path, RTLD_NOW);

    // Free path
    free (path);

    // If succeeded return
    if (hdl)
      return hdl;
  }

  // Not found
  return NULL;
}

void *plugin_load (const char *name, const char *args, plugin_type_t *type)
{
  void *shlib;
  plugin_t *plugin;
  plugin_list_t *e;
  char *err;
  list_t *cur;
  
  // Search to see if it's already loaded
  ll_for_each (cur, &plist) {

    // Get struct
    e = ll_entry (cur, plugin_list_t, list);
    
    // Check if it's a match by name
    if (!strcmp (e->name, name)) {
      if (type)
        *type = e->plugin->type;
      e->refcnt++;
      return e->plugin->create (args);
    }
  }
  
  // Clear any error codes
  dlerror ();
  
  // Load plugin
  shlib = plugin_open (name);
  if (!shlib)
    goto fail;
  
  // Resolve plugin structure symbol
  plugin = (plugin_t *)dlsym  (shlib, "__plugin");
  if ((err = dlerror ()) != NULL) {
    log (LOG_ERR, "Couldn't locate plugin syms: %s (%s)\n", name, err);
    return NULL;
  }

  // Malloc new node to track plugin
  e = (plugin_list_t *)malloc (sizeof (plugin_list_t));
  if (!e)
    goto fail;

  // Store plugin info
  e->name = (char *)malloc (strlen (name) + 1);
  if (!e->name)
    goto fail;
  strcpy (e->name, name);
  e->shlib = shlib;
  e->plugin = plugin;
  e->refcnt = 1;
  LL_INIT (&e->list);
  ll_add_head (&plist, &e->list);

  // Save type
  if (type)
    *type = plugin->type;
  
  // Create plugin
  log (LOG_DEBUG, "Loaded plugin: %s [%s]", name, plugin->version);
  return plugin->create (args);

  // Failed to load plugin
 fail:
  err ("Failed to load plugin: %s", name);
  return NULL;
}

static void plugin_handler (uint8_t *buf, int len)
{
  int i, rv;
  uint8_t size;
  uint32_t base, addr, data, mask;
  bool write;
  uint8_t resp[5];
      
  // Override any reads from 0xFFFFFFFx
  /*
  if (((trans.type & 4) == 0) &&
      ((trans.addr & 0xfffffff0) == 0xfffffff0)) {
    resp[0] = 0x12;
    resp[1] = resp[2] = resp[3] = resp[4] = 0;
    arbiter_reply (resp, 5);
    continue;
  }
  */
      
  // Get transaction type
  if (buf[0] & WRITE)
    write = true;
  else
    write = false;
  
  // Get size
  size = buf[0] & 3;
  
  // Verify length
  if (write) {
    switch (size) {
      case SZ_BYTE:
        if (len != 6)
          err ("Invalid len=%d", len);
        break;
      case SZ_HWRD:
        if (len != 7)
          err ("Invalid len=%d", len);
        break;
      case SZ_WORD:
        if (len != 9)
          err ("Invalid len=%d", len);
        break;
    }

    // Parse address
    addr = ntohl (*((uint32_t *)&buf[1]));

    // Parse data
    switch (size) {
      case SZ_BYTE:
        data = buf[5] << (8 * (addr & 3));
        mask = 0xff << 8 * (addr & 3);
        break;
      case SZ_HWRD:
        data = ntohs (*((uint16_t *)&buf[5])) << (8 * (addr & 3));
        mask = 0xffff << 8 * (addr & 3);
        break;
      case SZ_WORD:
        data = ntohl (*((uint32_t *)&buf[5]));
        mask = 0xffffffff;
        break;
    }
  }
  else {
    if (len != 5)
      err ("Invalid len=%d", len);

    // Parse address
    addr = ntohl (*((uint32_t *)&buf[1]));
  }

  // Transaction trace
  log (LOG_TRACE, "[%c%c] %08X",
       write ? 'W' : 'R',
       size == SZ_BYTE ? 'B' :
       (size == SZ_HWRD ? 'H' : 'W'),
       addr);
  
  // Find matching plugin
  for (i = 0; i < pcnt; i++) {
    base = plugin[i]->Base();
    
    // Check for match
    if ((addr >= base) &&
        (addr < base + plugin[i]->Size()))
      break;
  }    

  // Check if not found
  if (i == pcnt) {
    log (LOG_ERR, "Err: Unmatched addr: 0x%08X", addr);
    resp[0] = (write << 3) | FAIL;
    target->SlaveSend (resp, 1);
    return;
  }

  // Send to slave
  if (write) {

    // Dispatch to plugin
    plugin[i]->WriteW ((addr & ~3) - base, data, mask);

    // Send success
    resp[0] = SUCCESS;
    target->SlaveSend (resp, 1);
  }
  else {
    switch (size) {
      case SZ_BYTE:
        // Dispatch to plugin
        resp[1] = plugin[i]->ReadB (addr - base);

        // Send response
        resp[0] = READ_BYTE | SUCCESS;
        target->SlaveSend (resp, 2);
        break;
        
      case SZ_HWRD:
        // Dispatch to plugin
        *((uint16_t *)&resp[1]) = htons (plugin[i]->ReadH (addr - base));

        // Send response
        resp[0] = READ_HWRD | SUCCESS;
        target->SlaveSend (resp, 3);
        break;

      case SZ_WORD:
        // Dispatch to plugin
        *((uint32_t *)&resp[1]) = htonl (plugin[i]->ReadW (addr - base));

        // Send response
        resp[0] = READ_WORD | SUCCESS;
        target->SlaveSend (resp, 5);
        break;
    }
  }
}

void plugin_init (Target *targ, char *sysmap, char **path, int cnt)
{
  int rv;

  // Save target pointer
  target = targ;
  
  // Save search path
  search = path;
  scnt = cnt;
  
  // Init linked list
  LL_INIT (&plist);

  // Parse system map
  rv = sysmap_parse (sysmap, &plugin, &pcnt);
  if (rv)
    err ("Failed to parse sysmap: %s", sysmap);

  if (pcnt >= 1) {
    // Install slave callback
    target->SlaveRegister (&plugin_handler);

    // Enable slave
    target->SlaveEn (true);
  }
}

void plugin_cleanup (void)
{
  // Disable slave
  if (pcnt) {
    target->SlaveEn (false);
    target->SlaveUnregister ();
  }

  // Cleanup sysmap parser
  sysmap_cleanup ();
}
