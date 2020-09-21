/**
 *  Plugin helper routines.
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2019
 */
#include <fstream>
#include <iostream>

#include <string.h>
#include <stdlib.h>
#include "pluginhelper.h"

using namespace std;

int plugin_parse_uint (const char *str, const char *key, uint32_t *val)
{
  char *end;
  
  if (!val)
    return -1;
  
  // Find key in string
  const char *ptr = strstr (str, key);
  if (!ptr)
    return -1;

  // Skip over key
  ptr += strlen (key);

  // Parse value
  *val = strtoul (ptr, &end, 0);
  if (*val && *end) {
    if (*end == 'k')
      *val *= 1024;
    else if (*end == 'M')
      *val *= (1024 * 1024);
  }
  return 0;
}

int plugin_parse_bool (const char *str, const char *key, bool *val)
{
  char *end;
  uint32_t pval;
  
  if (!val)
    return -1;
  
  // Find key in string
  const char *ptr = strstr (str, key);
  if (!ptr)
    return -1;

  // Skip over key
  ptr += strlen (key);

  // Parse value
  pval = strtoul (ptr, &end, 0);
  if (pval)
    *val = true;
  else
    *val = false;
  return 0;
}

int plugin_parse_str (const char *args, const char *key, const char **str, int *len)
{
  const char *end;
  
  if (!str)
    return -1;
  
  // Find key in string
  const char *ptr = strstr (args, key);
  if (!ptr)
    return -1;

  // Skip over key
  ptr += strlen (key);
  end = ptr;
  
  // Find end of string
  while ((*end != ' ') && (*end != '\0'))
    end++;

  // Return string and length
  *str = ptr;
  *len = end - ptr;
  return 0;
}

int plugin_load_binary (const char *args, const char *key, char *ptr, int max)
{
  const char *end;
  char *filename;
  
  // Find key in string
  const char *p = strstr (args, key);
  if (!p)
    return -1;

  // Skip over key
  p += strlen (key);

  // Terminate with space or \0
  end = strchr (p, ' ');
  if (!end)
    end = p + strlen (p);
  
  // Copy to string
  filename = (char *)malloc (end - p + 1);
  if (!filename)
    return -1;
  memcpy (filename, p, end - p);
  filename[end - p] = '\0';

  // Open file and read
  ifstream file (filename, ios::in | ios::binary);

  // Read file up to max
  file.read (ptr, max);

  // Close file
  file.close ();
  return 0;
}
