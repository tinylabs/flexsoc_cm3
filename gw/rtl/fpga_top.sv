/**
 *  Top wrapper for FPGA. This is necessary as Verilator doesn't handle INOUT nets currently
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 **/

module fpga_top 
  #(
    parameter XILINX_ENC_CM3       = 0,
    parameter ROM_SZ               = 0,
    parameter RAM_SZ               = 0,
    parameter ROM_FILE             = "",
    parameter TRANSPORT_FREQ       = 0,
    parameter TRANSPORT_BAUD       = 0
    )(
      input       CLK_100M,
      input       RESET,
      // JTAG/SWD pins
      input       TCK_SWDCLK,
      input       TDI,
      inout       TMS_SWDIO,
      output      TDO,
      // UART to host
      input       UART_RX,
      output      UART_TX
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

   generate
      if (XILINX_ENC_CM3) begin : gen_pll
         
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
      end // block: gen_pll
      else begin : gen_pll

         // Obsfucated CM3 core can only support 32MHz HCLK
         PLLE2_BASE #(
                      .BANDWIDTH ("OPTIMIZED"),
                      .CLKFBOUT_MULT (16),
                      .CLKOUT0_DIVIDE(50),    // 32MHz
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

      end
   endgenerate

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
   
   // Inferred BUFIO on SWDIO
   logic               swdoe, swdout;
   assign TMS_SWDIO = swdoe ? swdout : 1'bz;

   // Instantiate soc
   flexsoc_cm3
     #(
       .XILINX_ENC_CM3       (XILINX_ENC_CM3),
       .ROM_SZ               (ROM_SZ),
       .RAM_SZ               (RAM_SZ),
       .ROM_FILE             (ROM_FILE),
       .TRANSPORT_FREQ       (TRANSPORT_FREQ),
       .TRANSPORT_BAUD       (TRANSPORT_BAUD)
       )
   u_soc (
          .CLK           (hclk),
          .TRANSPORT_CLK (transport_clk),
          .PORESETn      (poreset_n),
          .TCK_SWDCLK    (TCK_SWDCLK),
          .TDI           (TDI),
          .TMS_SWDIN     (TMS_SWDIO),
          .TDO           (TDO),
          .SWDOUT        (swdout),
          .SWDOUTEN      (swdoe),
          .UART_TX       (UART_TX),
          .UART_RX       (UART_RX)
          );
   
endmodule // fpga_top
