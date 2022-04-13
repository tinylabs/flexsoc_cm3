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
      // Debug SWD pins
      input  TCK_SWDCLK,
      inout  TMS_SWDIO,
      // UART to host
      input  UART_RX,
      output UART_TX,
      // Bridge to target
      output BRG_SWDCLK,
      inout  BRG_SWDIO
      );

   // Clocks and PLL
   logic     hclk, transport_clk;
   logic     pll_locked, pll_feedback;

   // Make sure clock is global
   BUFG u_gclk (
                .O (XTAL),
                .I (CLK_12M));
   
`define CORE_FREQ (XILINX_ENC_CM3 == 1) ? 50_000_000 : 32_000_000
   
   generate
      if (XILINX_ENC_CM3) begin : gen_pll
         
         // Full CM3 core = 50MHz
         MMCME2_BASE #(
                       .BANDWIDTH("OPTIMIZED"), // Jitter programming (OPTIMIZED, HIGH, LOW)
                       .CLKFBOUT_MULT_F(62.5),   // Multiply value for all CLKOUT (2.000-64.000).
                       .CLKFBOUT_PHASE(0.0),    // Phase offset in degrees of CLKFB (-360.000-360.000).
                       .CLKIN1_PERIOD(83.333),  // 12MHz input
                       // CLKOUT0_DIVIDE - CLKOUT6_DIVIDE: Divide amount for each CLKOUT (1-128)
                       .CLKOUT1_DIVIDE(15),
                       .CLKOUT2_DIVIDE(1),
                       .CLKOUT3_DIVIDE(1),
                       .CLKOUT4_DIVIDE(1),
                       .CLKOUT5_DIVIDE(1),
                       .CLKOUT6_DIVIDE(1),
                       .CLKOUT0_DIVIDE_F(15.625), // Divide amount for CLKOUT0 (1.000-128.000).
                       // CLKOUT0_DUTY_CYCLE - CLKOUT6_DUTY_CYCLE: Duty cycle for each CLKOUT (0.01-0.99).
                       .CLKOUT0_DUTY_CYCLE(0.5),
                       .CLKOUT1_DUTY_CYCLE(0.5),
                       .CLKOUT2_DUTY_CYCLE(0.5),
                       .CLKOUT3_DUTY_CYCLE(0.5),
                       .CLKOUT4_DUTY_CYCLE(0.5),
                       .CLKOUT5_DUTY_CYCLE(0.5),
                       .CLKOUT6_DUTY_CYCLE(0.5),
                       // CLKOUT0_PHASE - CLKOUT6_PHASE: Phase offset for each CLKOUT (-360.000-360.000).
                       .CLKOUT0_PHASE(0.0),
                       .CLKOUT1_PHASE(0.0),
                       .CLKOUT2_PHASE(0.0),
                       .CLKOUT3_PHASE(0.0),
                       .CLKOUT4_PHASE(0.0),
                       .CLKOUT5_PHASE(0.0),
                       .CLKOUT6_PHASE(0.0),
                       .CLKOUT4_CASCADE("FALSE"), // Cascade CLKOUT4 counter with CLKOUT6 (FALSE, TRUE)
                       .DIVCLK_DIVIDE(1), // Master division value (1-106)
                       .REF_JITTER1(0.0), // Reference input jitter in UI (0.000-0.999).
                       .STARTUP_WAIT("FALSE") // Delays DONE until MMCM is locked (FALSE, TRUE)
                       )
         u_pll (
                 // Clock Outputs: 1-bit (each) output: User configurable clock outputs
                 .CLKOUT0(transport_clk), // 1-bit output: CLKOUT0
                 .CLKOUT0B(), // 1-bit output: Inverted CLKOUT0
                 .CLKOUT1(hclk), // 1-bit output: CLKOUT1
                 .CLKOUT1B(), // 1-bit output: Inverted CLKOUT1
                 .CLKOUT2(), // 1-bit output: CLKOUT2
                 .CLKOUT2B(), // 1-bit output: Inverted CLKOUT2
                 .CLKOUT3(), // 1-bit output: CLKOUT3
                 .CLKOUT3B(), // 1-bit output: Inverted CLKOUT3
                 .CLKOUT4(), // 1-bit output: CLKOUT4
                 .CLKOUT5(), // 1-bit output: CLKOUT5
                 .CLKOUT6(), // 1-bit output: CLKOUT6
                 // Feedback Clocks: 1-bit (each) output: Clock feedback ports
                 .CLKFBOUT(pll_feedback), // 1-bit output: Feedback clock
                 .CLKFBOUTB(), // 1-bit output: Inverted CLKFBOUT
                 // Status Ports: 1-bit (each) output: MMCM status ports
                 .LOCKED(pll_locked), // 1-bit output: LOCK
                 // Clock Inputs: 1-bit (each) input: Clock input
                 .CLKIN1(XTAL), // 1-bit input: Clock
                 // Control Ports: 1-bit (each) input: MMCM control ports
                 .PWRDWN(1'b0), // 1-bit input: Power-down
                 .RST(1'b0), // 1-bit input: Reset
                 // Feedback Clocks: 1-bit (each) input: Clock feedback ports
                 .CLKFBIN(pll_feedback) // 1-bit input: Feedback clock
                 );
   
      end // block: gen_pll
      else begin : gen_pll

         // Obsfucated CM3 core = 32MHz
         MMCME2_BASE #(
                       .BANDWIDTH("OPTIMIZED"), // Jitter programming (OPTIMIZED, HIGH, LOW)
                       .CLKFBOUT_MULT_F(64.0),   // Multiply value for all CLKOUT (2.000-64.000).
                       .CLKFBOUT_PHASE(0.0),    // Phase offset in degrees of CLKFB (-360.000-360.000).
                       .CLKIN1_PERIOD(83.333),  // 12MHz input
                       // CLKOUT0_DIVIDE - CLKOUT6_DIVIDE: Divide amount for each CLKOUT (1-128)
                       .CLKOUT1_DIVIDE(24),
                       .CLKOUT2_DIVIDE(1),
                       .CLKOUT3_DIVIDE(1),
                       .CLKOUT4_DIVIDE(1),
                       .CLKOUT5_DIVIDE(1),
                       .CLKOUT6_DIVIDE(1),
                       .CLKOUT0_DIVIDE_F(16.0), // Divide amount for CLKOUT0 (1.000-128.000).
                       // CLKOUT0_DUTY_CYCLE - CLKOUT6_DUTY_CYCLE: Duty cycle for each CLKOUT (0.01-0.99).
                       .CLKOUT0_DUTY_CYCLE(0.5),
                       .CLKOUT1_DUTY_CYCLE(0.5),
                       .CLKOUT2_DUTY_CYCLE(0.5),
                       .CLKOUT3_DUTY_CYCLE(0.5),
                       .CLKOUT4_DUTY_CYCLE(0.5),
                       .CLKOUT5_DUTY_CYCLE(0.5),
                       .CLKOUT6_DUTY_CYCLE(0.5),
                       // CLKOUT0_PHASE - CLKOUT6_PHASE: Phase offset for each CLKOUT (-360.000-360.000).
                       .CLKOUT0_PHASE(0.0),
                       .CLKOUT1_PHASE(0.0),
                       .CLKOUT2_PHASE(0.0),
                       .CLKOUT3_PHASE(0.0),
                       .CLKOUT4_PHASE(0.0),
                       .CLKOUT5_PHASE(0.0),
                       .CLKOUT6_PHASE(0.0),
                       .CLKOUT4_CASCADE("FALSE"), // Cascade CLKOUT4 counter with CLKOUT6 (FALSE, TRUE)
                       .DIVCLK_DIVIDE(1), // Master division value (1-106)
                       .REF_JITTER1(0.0), // Reference input jitter in UI (0.000-0.999).
                       .STARTUP_WAIT("FALSE") // Delays DONE until MMCM is locked (FALSE, TRUE)
                       )
         u_pll (
                 // Clock Outputs: 1-bit (each) output: User configurable clock outputs
                 .CLKOUT0(transport), // 1-bit output: CLKOUT0
                 .CLKOUT0B(), // 1-bit output: Inverted CLKOUT0
                 .CLKOUT1(hclk), // 1-bit output: CLKOUT1
                 .CLKOUT1B(), // 1-bit output: Inverted CLKOUT1
                 .CLKOUT2(), // 1-bit output: CLKOUT2
                 .CLKOUT2B(), // 1-bit output: Inverted CLKOUT2
                 .CLKOUT3(), // 1-bit output: CLKOUT3
                 .CLKOUT3B(), // 1-bit output: Inverted CLKOUT3
                 .CLKOUT4(), // 1-bit output: CLKOUT4
                 .CLKOUT5(), // 1-bit output: CLKOUT5
                 .CLKOUT6(), // 1-bit output: CLKOUT6
                 // Feedback Clocks: 1-bit (each) output: Clock feedback ports
                 .CLKFBOUT(pll_feedback), // 1-bit output: Feedback clock
                 .CLKFBOUTB(), // 1-bit output: Inverted CLKFBOUT
                 // Status Ports: 1-bit (each) output: MMCM status ports
                 .LOCKED(pll_locked), // 1-bit output: LOCK
                 // Clock Inputs: 1-bit (each) input: Clock input
                 .CLKIN1(XTAL), // 1-bit input: Clock
                 // Control Ports: 1-bit (each) input: MMCM control ports
                 .PWRDWN(1'b0), // 1-bit input: Power-down
                 .RST(1'b0), // 1-bit input: Reset
                 // Feedback Clocks: 1-bit (each) input: Clock feedback ports
                 .CLKFBIN(pll_feedback) // 1-bit input: Feedback clock
                 );
      end
   endgenerate
   
   // Generate reset logic from pushbutton/pll
   logic [3:0]         reset_ctr;
   initial reset_ctr <= 'hf;
   
   always @(posedge hclk)
     begin
        if (!RESETn | !pll_locked)
          reset_ctr <= 'hf;
        else if (reset_ctr)
          reset_ctr <= reset_ctr - 1;
     end
   assign poreset_n = (reset_ctr != 0) ? 1'b0 : 1'b1;
   
   // flexsoc debug
   logic               swdoe, swdout;
   logic               swdin;
   IOBUF u_swdio0 (.IO (TMS_SWDIO), .I (swdout), .O (swdin), .T (!swdoe));

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
          .TCK           (TCK_SWDCLK),
          .TDI           (1'b0),
          .TMSIN         (swdin),
          .TDO           (),
          .TMSOUT        (swdout),
          .TMSOE         (swdoe),
          .UART_TX       (UART_TX),
          .UART_RX       (UART_RX),
          .BRG_SWDIN     (brg_swdin),
          .BRG_SWDOUT    (brg_swdout),
          .BRG_SWDOE     (brg_swdoe),
          .BRG_SWDCLK    (BRG_SWDCLK)
          );
               
endmodule // cmod_a7_top
