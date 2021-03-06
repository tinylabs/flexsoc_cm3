/*
 * Flexsoc_cm3 simulation in verilator. Heavily borrowed from the following
 * project:
 *
 * mor1kx-generic system Verilator testbench
 *
 * Author: Olof Kindgren <olof.kindgren@gmail.com>
 * Author: Franck Jullien <franck.jullien@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <stdint.h>
#include <signal.h>
#include <argp.h>
#include <verilator_utils.h>

#include "Vio_loopback.h"

static bool done;

#define RESET_TIME		4

vluint64_t main_time = 0;       // Current simulation time
// This is a 64-bit integer to reduce wrap over issues and
// allow modulus.  You can also use a double, if you wish.

double sc_time_stamp () {   // Called by $time in Verilog
  return main_time;        // converts to double, to match
                           // what SystemC does
}

void INThandler(int signal)
{
	printf("\nCaught ctrl-c\n");
	done = true;
}

static int parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case ARGP_KEY_INIT:
		state->child_inputs[0] = state->input;
		break;
	// Add parsing of custom options here
	}

	return 0;
}

static int parse_args(int argc, char **argv, VerilatorUtils* utils)
{
	struct argp_option options[] = {
		// Add custom options here
		{ 0 }
	};
	struct argp_child child_parsers[] = {
		{ &verilator_utils_argp, 0, "", 0 },
		{ 0 }
	};
	struct argp argp = { options, parse_opt, 0, 0, child_parsers };

	return argp_parse(&argp, argc, argv, 0, 0, utils);
}

int main(int argc, char **argv, char **env)
{
	uint32_t insn = 0;
	uint32_t ex_pc = 0;

	Verilated::commandArgs(argc, argv);

	Vio_loopback* top = new Vio_loopback;
	VerilatorUtils* utils =
      new VerilatorUtils(NULL);
    
	parse_args(argc, argv, utils);
	signal(SIGINT, INThandler);

    // Setup initial signals
    top->CLK = 0;
    top->TRANSPORT_CLK = 0;
    top->PORESETn = 0;
    top->UART_RX = 1; // RX should be default high

	top->trace(utils->tfp, 99);
    
	while (utils->doCycle() && !done) {

      // Deassert RESET after some time
      if (utils->getTime() > RESET_TIME)
        top->PORESETn = 1;
      
      top->eval();
      top->CLK = !top->CLK;
      top->TRANSPORT_CLK = !top->TRANSPORT_CLK;
      if (top->PORESETn)
        utils->doUARTServer (top->UART_TX, &top->UART_RX);
	}
    
	delete utils;
	exit(0);
}
