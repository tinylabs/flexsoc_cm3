/**
 *  Loopback top level for FPGA implementation
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 **/

module loopback_top 
  (
   input  CLK_100M,
   input  RESET,
   // UART to host
   input UART_RX,
   output UART_TX
   );

   // Clocks and PLL
   logic     hclk, transport_clk;
   logic     hpll_locked, hpll_feedback;
   logic     tpll_locked, tpll_feedback;

   // Transport PLL
   PLLE2_BASE #(
                .BANDWIDTH ("OPTIMIZED"),
                .CLKFBOUT_MULT (12),
                .CLKOUT0_DIVIDE(25),    // 48MHz HCLK
                .CLKFBOUT_PHASE(0.0),   // Phase offset in degrees of CLKFB, (-360-360)
                .CLKIN1_PERIOD(10.0),   // 100MHz input clock
                .CLKOUT0_DUTY_CYCLE(0.5),
                .CLKOUT0_PHASE(0.0),
                .DIVCLK_DIVIDE(1),    // Master division value , (1-56)
                .REF_JITTER1(0.0),    // Reference input jitter in UI (0.000-0.999)
                .STARTUP_WAIT("FALSE") // Delay DONE until PLL Locks, ("TRUE"/"FALSE")
                ) u_tpll (
                         // Clock outputs: 1-bit (each) output
                         .CLKOUT0(transport_clk),
                         .CLKOUT1(),
                         .CLKOUT2(),
                         .CLKOUT3(),
                         .CLKOUT4(),
                         .CLKOUT5(),
                         .CLKFBOUT(tpll_feedback), // 1-bit output, feedback clock
                         .LOCKED(tpll_locked),
                         .CLKIN1(CLK_100M),
                         .PWRDWN(1'b0),
                         .RST(1'b0),
                         .CLKFBIN(tpll_feedback)    // 1-bit input, feedback clock
                         );

   // Full CM3 core PLL timing
   PLLE2_BASE #(
                .BANDWIDTH ("OPTIMIZED"),
                .CLKFBOUT_MULT (12),
                .CLKOUT0_DIVIDE(24),    // 50MHz HCLK
                .CLKFBOUT_PHASE(0.0),   // Phase offset in degrees of CLKFB, (-360-360)
                .CLKIN1_PERIOD(10.0),   // 100MHz input clock
                .CLKOUT0_DUTY_CYCLE(0.5),
                .CLKOUT0_PHASE(0.0),
                .DIVCLK_DIVIDE(1),    // Master division value , (1-56)
                .REF_JITTER1(0.0),    // Reference input jitter in UI (0.000-0.999)
                .STARTUP_WAIT("FALSE") // Delay DONE until PLL Locks, ("TRUE"/"FALSE")
                ) u_hpll (
                          // Clock outputs: 1-bit (each) output
                          .CLKOUT0(hclk),
                          .CLKOUT1(),
                          .CLKOUT2(),
                          .CLKOUT3(),
                          .CLKOUT4(),
                          .CLKOUT5(),
                          .CLKFBOUT(hpll_feedback), // 1-bit output, feedback clock
                          .LOCKED(hpll_locked),
                          .CLKIN1(CLK_100M),
                          .PWRDWN(1'b0),
                          .RST(1'b0),
                          .CLKFBIN(hpll_feedback)    // 1-bit input, feedback clock
                          );

   // Generate reset logic from pushbutton/pll
   logic               poreset_n;
   logic [7:0]         reset_ctr;
   always @(posedge hclk)
     begin
        if (RESET | !hpll_locked)
          reset_ctr <= 'hff;
        else if (reset_ctr)
          reset_ctr = reset_ctr - 1;
     end
   assign poreset_n = reset_ctr ? 1'b0 : 1'b1;
   
   // Instantiate soc
   io_loopback
     u_soc (
            .CLK           (hclk),
            .TRANSPORT_CLK (transport_clk),
            .PORESETn      (poreset_n),
            .UART_TX       (UART_TX),
            .UART_RX       (UART_RX)
            );

   
endmodule // loopback_top

