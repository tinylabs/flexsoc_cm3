/**
 *  Top wrapper for FPGA. This is necessary as Verilator doesn't handle INOUT nets currently
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 **/

module cmod_a7_top
  #(
    parameter XILINX_ENC_CM3       = 0,
    parameter ROM_SZ               = 0,
    parameter RAM_SZ               = 0,
    parameter ROM_FILE             = "",
    parameter REMOTE_BASE          = 0,
    parameter CORE_FREQ            = 0
    )(
      input  CLK_12M,
      input  RESETn,
      // UART to host
      input  UART_RX,
      output UART_TX,
      // Bridge to target
      output BRG_SWDCLK,
      inout  BRG_SWDIO
      );

   // Clocks and PLL
   logic     hclk, transport_clk;
   logic     hpll_locked, hpll_feedback;
   logic     tpll_locked, tpll_feedback;

`define CORE_FREQ (XILINX_ENC_CM3 == 1) ? 48_000_000 : 32_000_000
   
   // Transport PLL
   PLLE2_BASE #(
                .BANDWIDTH ("OPTIMIZED"),
                .CLKFBOUT_MULT (4),
                .CLKOUT0_DIVIDE(1),    // 48MHz HCLK
                .CLKFBOUT_PHASE(0.0),   // Phase offset in degrees of CLKFB, (-360-360)
                .CLKIN1_PERIOD(83.33),   // 12MHz input clock
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
                      .CLKFBOUT_MULT (4),
                      .CLKOUT0_DIVIDE(1),     // 48MHz HCLK
                      .CLKFBOUT_PHASE(0.0),   // Phase offset in degrees of CLKFB, (-360-360)
                      .CLKIN1_PERIOD(83.33),  // 12MHz input clock
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

         PLLE2_BASE #(
                      .BANDWIDTH ("OPTIMIZED"),
                      .CLKFBOUT_MULT (8),
                      .CLKOUT0_DIVIDE(3),     // 32MHz
                      .CLKFBOUT_PHASE(0.0),   // Phase offset in degrees of CLKFB, (-360-360)
                      .CLKIN1_PERIOD(83.33),  // 12MHz input clock
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
   logic [3:0]         reset_ctr;
   initial reset_ctr <= 'hf;
   
   always @(posedge hclk)
     begin
        if (!RESETn | !hpll_locked | !tpll_locked)
          reset_ctr <= 'hf;
        else if (reset_ctr)
          reset_ctr <= reset_ctr - 1;
     end
   assign poreset_n = (reset_ctr != 0) ? 1'b0 : 1'b1;
   
   // flexsoc debug
   //logic               swdoe, swdout;
   //logic               swdin;
   //IOBUF u_swdio0 (.IO (TMS_SWDIO), .I (swdout), .O (swdin), .T (!swdoe));

   // target bridge
   logic               brg_swdoe, brg_swdout;
   logic               brg_swdin;
   IOBUF u_swdio1 (.IO (BRG_SWDIO), .I (brg_swdout), .O (brg_swdin), .T (!brg_swdoe));

   // Instantiate SoC
   flexsoc_cm3
     #(
       .CORE_FREQ        (`CORE_FREQ),
       .XILINX_ENC_CM3   (XILINX_ENC_CM3),
       .ROM_SZ           (ROM_SZ),
       .RAM_SZ           (RAM_SZ),
       .ROM_FILE         (ROM_FILE),
       .REMOTE_BASE      (REMOTE_BASE)
       )
   u_soc (
          .CLK           (hclk),
          .TRANSPORT_CLK (transport_clk),
          .PORESETn      (poreset_n),
          .TCK           (1'b0),
          .TDI           (1'b0),
          .TMSIN         (1'b0),
          .TDO           (),
          .TMSOUT        (),
          .TMSOE         (),
          .UART_TX       (UART_TX),
          .UART_RX       (UART_RX),
          .BRG_SWDIN     (brg_swdin),
          .BRG_SWDOUT    (brg_swdout),
          .BRG_SWDOE     (brg_swdoe),
          .BRG_SWDCLK    (BRG_SWDCLK)
          );
               
endmodule // cmod_a7_top
