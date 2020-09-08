/**
 *  flexsoc_cm3 entry point - Parse args and pass to daemon
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <argp.h>
#include <string.h>

#include "flexsoc_cm3.h"
#include "err.h"
#include "log.h"

// Argument storage
static args_t args;

static int parse_opts (int key, char *arg, struct argp_state *state)
{
  switch (key) {
    // Append to load list
    case 'l':
      if (arg) {
        char *addr;
        args.load = (load_t *)realloc (args.load, sizeof (load_t) * (args.load_cnt + 1));

        // Parse into name/addr
        addr = strchr (arg, '@');
        if (addr) {
          *addr = '\0';
          addr++;
          args.load[args.load_cnt].addr = strtoul (addr, NULL, 0);
        }
        else
          args.load[args.load_cnt].addr = 0;
        
        args.load[args.load_cnt].name = (char *)malloc (strlen (arg) + 1);
        if (!args.load[args.load_cnt].name)
          err ("Malloc failed!");
        strcpy (args.load[args.load_cnt].name, arg);
        args.load_cnt++;
      }
      break;

    case 'v':
      if (arg)
        args.verbose = strtoul (arg, NULL, 0);
      break;
      
    case 'm':
      if (arg) {
        args.map = (char *)malloc (strlen (arg) + 1);
        strcpy (args.map, arg);
      }
      break;
      
    case 'p':
      if (arg) {
        args.path = (char **)realloc (args.path, sizeof (char *) * (args.path_cnt + 1));
        args.path[args.path_cnt] = (char *)malloc (strlen (arg) + 1);
        if (!args.path[args.path_cnt])
          err ("Malloc failed!");
        strcpy (args.path[args.path_cnt], arg);
        args.path_cnt++;
      }
      break;
      
    case ARGP_KEY_ARG:
      args.device = arg;
      break;
      
    // Check arguments
    case ARGP_KEY_END:
      if ((args.load_cnt == 0) || !args.device)
        argp_usage (state);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

// Command line options
static struct argp_option options[] = {
                                       {0, 0, 0, 0, "Operations:", 1},
                                       {"map",     'm', "FILE", 0, "system map file"},
                                       {"load",    'l', "FILE", 0, "filename[@address] (default=0)\n"
                                        "multiple load opts supported"},
                                       {"path",    'p', "DIR", 0,  "Plugin search path\n"
                                        "multiple path opts supported"},
                                       {0, 0, 0, 0, "Remote:", 2},
                                       {"speed",   's', "INT", 0,  "speed in kHz: 100-25000"},
                                       {0, 0, 0, 0, "Debugging:", 3},
                                       {"verbose", 'v', "INT", 0,  "verbosity level (0-3)"},
                                       {0}
};

struct argp flexsoc_cm3_argp = {options, &parse_opts, 0, 0};

int main (int argc, char **argv)
{
  int rv, i;
  
  // Clear args
  memset (&args, 0, sizeof (args));
  args.verbose = LOG_NORMAL;
  
  // Add system path to plugin path
  
  // Parse args
  argp_parse (&flexsoc_cm3_argp, argc, argv, 0, 0, 0);

  // Do work
  rv = flexsoc_cm3 (&args);

  // Clean up
  for (i = 0; i < args.load_cnt; i++)
    free (args.load[i].name);
  free (args.load);
  for (i = 0; i < args.path_cnt; i++)
    free (args.path[i]);
  free (args.path);
  if (args.map)
    free (args.map);
  return rv;
}

