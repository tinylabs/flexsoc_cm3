/**
 *  Flexible System On Chip emulator for ARM Cortex-M3 devices
 *
 *  All rights reserved 
 *  Tiny Labs Inc
 *  2020
 */

module flexsoc_cm3
  #(
    parameter XILINX_ENC_CM3 = 0,
    parameter ROM_SZ         = 0,
    parameter RAM_SZ         = 0,
    parameter ROM_FILE       = "",
    parameter TRANSPORT_FREQ = 50000000,
    parameter TRANSPORT_BAUD = 15625000
  ) (
     // Clock and reset
     input  CLK,
     input  PORESETn,
     input  TRANSPORT_CLK,
     
     // JTAG/SWD
     input  TCK_SWDCLK,
     input  TDI,
     input  TMS_SWDIN,
     output TDO,
     output SWDOUT,
     output SWDOUTEN,

     // Host interface
     output UART_TX,
     input  UART_RX
   );

   // Implicit reset for autogen interconnect
   logic           RESETn;
   assign RESETn = PORESETn;   

   // Include generated AHB3lite interconnect crossbar
`include "ahb3lite_intercon.vh"

   // Include autgen config/status registers
`include "flexsoc_csr.vh"
   
   // IRQs to cm3 core
   logic [15:0] irq;   
   assign irq = {16'h0};

   // Assign registers
   always @(posedge CLK)
     begin
        flexsoc_id <= 32'hdeadd00d;
        slave_en_i <= slave_en_o;
     end
  
   
   // CPU reset controller
   logic        cpureset_n, sysresetreq;
   logic [3:0]  cpureset_ctr;
   always @(posedge CLK)
     begin
        if (!PORESETn | sysresetreq)
          cpureset_ctr <= 4'hf;
        else if (|cpureset_ctr)
          cpureset_ctr <= cpureset_ctr - 1;
     end
   assign cpureset_n = |cpureset_ctr ? 1'b0 : 1'b1;
   
   // Instantiate ROM
   ahb3lite_sram1rw
     #(
       .MEM_SIZE (ROM_SZ),
       .HADDR_SIZE (32),
       .HDATA_SIZE (32),
       .TECHNOLOGY ("GENERIC"),
       .REGISTERED_OUTPUT ("NO"),
       .LOAD_FILE (ROM_FILE)
       ) u_rom (
                .HCLK      (CLK),
                .HRESETn   (PORESETn),
                .HSEL      (ahb3_rom_HSEL),
                .HADDR     (ahb3_rom_HADDR),
                .HWDATA    (ahb3_rom_HWDATA),
                .HRDATA    (ahb3_rom_HRDATA),
                .HWRITE    (ahb3_rom_HWRITE),
                .HSIZE     (ahb3_rom_HSIZE),
                .HBURST    (ahb3_rom_HBURST),
                .HPROT     (ahb3_rom_HPROT),
                .HTRANS    (ahb3_rom_HTRANS),
                .HREADYOUT (ahb3_rom_HREADYOUT),
                .HREADY    (ahb3_rom_HREADY),
                .HRESP     (ahb3_rom_HRESP)
                );
   
   // Instantiate RAM
   ahb3lite_sram1rw
     #(
       .MEM_SIZE (RAM_SZ),
       .HADDR_SIZE (32),
       .HDATA_SIZE (32),
       .TECHNOLOGY ("GENERIC"),
       .REGISTERED_OUTPUT ("NO")
       ) u_ram (
                .HCLK      (CLK),
                .HRESETn   (PORESETn),
                .HSEL      (ahb3_ram_HSEL),
                .HADDR     (ahb3_ram_HADDR),
                .HWDATA    (ahb3_ram_HWDATA),
                .HRDATA    (ahb3_ram_HRDATA),
                .HWRITE    (ahb3_ram_HWRITE),
                .HSIZE     (ahb3_ram_HSIZE),
                .HBURST    (ahb3_ram_HBURST),
                .HPROT     (ahb3_ram_HPROT),
                .HTRANS    (ahb3_ram_HTRANS),
                .HREADYOUT (ahb3_ram_HREADYOUT),
                .HREADY    (ahb3_ram_HREADY),
                .HRESP     (ahb3_ram_HRESP)
                );

   // Default slave to handle bad requests
   ahb3lite_default_slave
     u_dslave (
               .CLK       (CLK),
               .RESETn    (PORESETn),
               .HSEL      (ahb3_default_slave_HSEL),
               .HTRANS    (ahb3_default_slave_HTRANS),
               .HREADY    (ahb3_default_slave_HREADY),
               .HREADYOUT (ahb3_default_slave_HREADYOUT),
               .HRESP     (ahb3_default_slave_HRESP),
               .HRDATA    (ahb3_default_slave_HRDATA)
               );

   // Connection to host
   wire host_RDEN, host_WREN, host_WRFULL, host_RDEMPTY;
   wire [7:0] host_RDDATA, host_WRDATA;

   // FIFO <=> Transport
   wire trans_RDEN, trans_WREN, trans_WRFULL, trans_RDEMPTY;
   wire [7:0] trans_RDDATA, trans_WRDATA;

   // Host AHB3 master
   assign ahb3_host_master_HSEL = 1'b1;
   assign ahb3_host_master_HMASTLOCK = 1'b0;
   
   ahb3lite_host_master
     u_host_master (
                    .CLK       (CLK),
                    .RESETn    (PORESETn),
                    .HADDR     (ahb3_host_master_HADDR),
                    .HWDATA    (ahb3_host_master_HWDATA),
                    .HTRANS    (ahb3_host_master_HTRANS),
                    .HSIZE     (ahb3_host_master_HSIZE),
                    .HBURST    (ahb3_host_master_HBURST),
                    .HPROT     (ahb3_host_master_HPROT),
                    .HWRITE    (ahb3_host_master_HWRITE),
                    .HRDATA    (ahb3_host_master_HRDATA),
                    .HRESP     (ahb3_host_master_HRESP),
                    .HREADY    (ahb3_host_master_HREADY),
                    .RDEN      (host_RDEN),
                    .RDEMPTY   (host_RDEMPTY),
                    .RDDATA    (host_RDDATA),
                    .WREN      (host_WREN),
                    .WRFULL    (host_WRFULL),
                    .WRDATA    (host_WRDATA)
                    );

   // Host => Transport
   dual_clock_fifo #(
                     .ADDR_WIDTH   (4),
                     .DATA_WIDTH   (8))
   u_tx_fifo (
              .wr_clk_i   (CLK),
              .rd_clk_i   (TRANSPORT_CLK),
              .rd_rst_i   (~PORESETn),
              .wr_rst_i   (~PORESETn),
              .wr_en_i    (host_WREN),
              .wr_data_i  (host_WRDATA),
              .full_o     (host_WRFULL),
              .rd_en_i    (trans_RDEN),
              .rd_data_o  (trans_RDDATA),
              .empty_o    (trans_RDEMPTY)
              );

   // Transport => Host
   dual_clock_fifo #(
                     .ADDR_WIDTH   (4),
                     .DATA_WIDTH   (8))
   u_rx_fifo (
              .rd_clk_i   (CLK),
              .wr_clk_i   (TRANSPORT_CLK),
              .rd_rst_i   (~PORESETn),
              .wr_rst_i   (~PORESETn),
              .rd_en_i    (host_RDEN),
              .rd_data_o  (host_RDDATA),
              .empty_o    (host_RDEMPTY),
              .wr_en_i    (trans_WREN),
              .wr_data_i  (trans_WRDATA),
              .full_o     (trans_WRFULL)
              );
   
   // Host transport
   uart_transport #(
                    .FREQ (TRANSPORT_FREQ),
                    .BAUD (TRANSPORT_BAUD))
   u_transport (
                .CLK        (TRANSPORT_CLK),
                .RESETn     (PORESETn),
                // UART interface
                .TX_PIN     (UART_TX),
                .RX_PIN     (UART_RX),
                // FIFO interface
                .FIFO_WREN  (trans_WREN),
                .FIFO_FULL  (trans_WRFULL),
                .FIFO_DOUT  (trans_WRDATA),
                .FIFO_RDEN  (trans_RDEN),
                .FIFO_EMPTY (trans_RDEMPTY),
                .FIFO_DIN   (trans_RDDATA)
                );
   
   // Enable master ports
   assign ahb3_cm3_code_HSEL = 1'b1;
   assign ahb3_cm3_sys_HSEL = 1'b1;
   
   // Instantiate cortex-m3 core
   cm3_core
     #(
       .XILINX_ENC_CM3  (XILINX_ENC_CM3),
       .NUM_IRQ         (16)
       )
     u_cm3 (
            // Clock and reset
            .FCLK         (CLK),
            .HCLK         (CLK),
            .PORESETn     (PORESETn),
            .CPURESETn    (cpureset_n),
            .SYSRESETREQ  (sysresetreq),
            
            // IRQs
            .INTISR       (irq),
            .INTNMI       (1'b0),
            
            // Debug
            .SWCLKTCK     (TCK_SWDCLK),
            .SWDITMS      (TMS_SWDIN),
            .SWDO         (SWDOUT),
            .SWDOEN       (SWDOUTEN),
            .nTRST        (1'b1),
            .TDI          (TDI),
            .TDO          (TDO),
            .nTDOEN       (),
            .SWV          (),
            
            // Status
            .HALTED       (),
            .LOCKUP       (),
            .JTAGNSW      (),
            
            // AHB3 code master
            .code_HADDR     (ahb3_cm3_code_HADDR),
            .code_HWDATA    (ahb3_cm3_code_HWDATA),
            .code_HTRANS    (ahb3_cm3_code_HTRANS),
            .code_HSIZE     (ahb3_cm3_code_HSIZE),
            .code_HBURST    (ahb3_cm3_code_HBURST),
            .code_HPROT     (ahb3_cm3_code_HPROT),
            .code_HWRITE    (ahb3_cm3_code_HWRITE),
            .code_HMASTLOCK (ahb3_cm3_code_HMASTLOCK),
            .code_HRDATA    (ahb3_cm3_code_HRDATA),
            .code_HRESP     (ahb3_cm3_code_HRESP),
            .code_HREADY    (ahb3_cm3_code_HREADY),

            // AHB3 system master
            .sys_HADDR     (ahb3_cm3_sys_HADDR),
            .sys_HWDATA    (ahb3_cm3_sys_HWDATA),
            .sys_HTRANS    (ahb3_cm3_sys_HTRANS),
            .sys_HSIZE     (ahb3_cm3_sys_HSIZE),
            .sys_HBURST    (ahb3_cm3_sys_HBURST),
            .sys_HPROT     (ahb3_cm3_sys_HPROT),
            .sys_HWRITE    (ahb3_cm3_sys_HWRITE),
            .sys_HMASTLOCK (ahb3_cm3_sys_HMASTLOCK),
            .sys_HRDATA    (ahb3_cm3_sys_HRDATA),
            .sys_HRESP     (ahb3_cm3_sys_HRESP),
            .sys_HREADY    (ahb3_cm3_sys_HREADY) 
           );

   
endmodule // cm3_min_soc
