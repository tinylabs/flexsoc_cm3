/**
 *  Flexible System On Chip emulator for ARM Cortex-M3 devices
 *
 *  All rights reserved 
 *  Tiny Labs Inc
 *  2020
 */

module flexsoc_cm3
  #(
    parameter CORE_FREQ      = 0,
    parameter XILINX_ENC_CM3 = 0,
    parameter ROM_SZ         = 0,
    parameter RAM_SZ         = 0,
    parameter ROM_FILE       = "",
    parameter REMOTE_BASE    = 0
  ) (
     // Clock and reset
     input  CLK,
     input  PORESETn,
     input  TRANSPORT_CLK,
     
     // JTAG/SWD
     input  TCK,
     input  TDI,
     output TDO,
     input  TMSIN,
     output TMSOUT,
     output TMSOE,

     // SWD bridge
     input  BRG_SWDIN,
     output BRG_SWDOUT,
     output BRG_SWDOE,
     output BRG_SWDCLK,
     
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
`define IRQ_CNT   32
   logic [`IRQ_CNT-1:0] irq;   

   // Dropped host comm bytes
   logic [9:0]          dropped;
   
   // Initialize SoC registers
   assign cpu_reset_i = ~PORESETn ? 1'b1 : cpu_reset_o;
   assign slave_en_i = slave_en_o;
   assign flexsoc_id = 32'hf1ec50c1;
   assign memory_id = (ROM_SZ >> 10) | ((RAM_SZ >> 10) << 16);
   assign core_freq = CORE_FREQ;
   assign brg_base = REMOTE_BASE;

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

   // Generate IRQs from CSR
   for (genvar n = 0; n < `IRQ_CNT / 32; n++)
     begin
        assign irq[(n*32)+31:(n*32)] = irq_level_o[n] | irq_edge_o[n];

        // Level follows, edge resets
        assign irq_level_o[n] = irq_level_i[n];
        assign irq_edge_o[n] = 32'h0;
     end

   // Redirect AHB3 master addresses
   wire [31:0] cm3_code_haddr;   
   assign ahb3_cm3_code_HADDR = ((cm3_code_haddr >= code_remap_base_o[0]) && 
                                 (cm3_code_haddr < code_remap_end_o[0])) ?
                                (cm3_code_haddr - code_remap_base_o[0]) + code_remap_off_o[0] : 
                                (((cm3_code_haddr >= code_remap_base_o[1]) && 
                                  (cm3_code_haddr < code_remap_end_o[1])) ?
                                 (cm3_code_haddr - code_remap_base_o[1]) + code_remap_off_o[1]
                                 : cm3_code_haddr);
   assign code_remap_base_i[0] = code_remap_base_o[0];
   assign code_remap_end_i[0]  = code_remap_end_o[0];
   assign code_remap_off_i[0]  = code_remap_off_o[0];
   assign code_remap_base_i[1] = code_remap_base_o[1];
   assign code_remap_end_i[1]  = code_remap_end_o[1];
   assign code_remap_off_i[1]  = code_remap_off_o[1];

   wire [31:0] cm3_sys_haddr;
   assign ahb3_cm3_sys_HADDR = ((cm3_sys_haddr >= sys_remap_base_o) &&
                                (cm3_sys_haddr < sys_remap_end_o)) ?
                                (cm3_sys_haddr - sys_remap_base_o) + sys_remap_off_o 
                               : cm3_sys_haddr;
   assign sys_remap_base_i = sys_remap_base_o;
   assign sys_remap_end_i  = sys_remap_end_o;
   assign sys_remap_off_i  = sys_remap_off_o;
   
   // Instantiate ROM
   ahb3lite_sram1rw
     #(
       .MEM_SIZE (ROM_SZ),
       .HADDR_SIZE (32),
       .HDATA_SIZE (32),
       .TECHNOLOGY ("GENERIC"),
       .REGISTERED_OUTPUT ("NO"),
       .INIT_FILE (ROM_FILE)
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

   // Loop output back to input where necessary
   assign brg_en_i = brg_en_o;
   assign brg_clkdiv_i = brg_clkdiv_o;   
   assign brg_ahb_en_i = brg_ahb_en_o;
   assign brg_apsel_i = brg_apsel_o;
   assign brg_remap32_i[0] = brg_remap32_o[0];
   assign brg_remap32_i[1] = brg_remap32_o[1];
   assign brg_remap256_i = brg_remap256_o;
   assign brg_irq_scanen_i = brg_irq_scanen_o;
   assign brg_irq_mask_i[0] = brg_irq_mask_o[0];
   assign brg_irq_mask_i[1] = brg_irq_mask_o[1];
   assign brg_irq_len_i = brg_irq_len_o;
   assign brg_irq_off_i = brg_irq_off_o;
   assign brg_csw_fixed_i = brg_csw_fixed_o;

   // Remote bridge to target
   ahb3lite_remote_bridge
     #( .BASE_ADDR (REMOTE_BASE))
   u_remote (
             // Clock and reset
             .CLK          (CLK),
             .RESETn       (PORESETn),

             // Interface
             .EN           (brg_en_o),
             .CLKDIV       (brg_clkdiv_o),
             .CTRLI        (brg_ctrl_o),
             .CTRLO        (brg_ctrl_i),
             .DATI         (brg_data_o),
             .DATO         (brg_data_i),
             .STAT         (brg_stat),
             .AHB_EN       (brg_ahb_en_o),
             .AHB_APSEL    (brg_apsel_o),
             .IDCODE       (brg_idcode),
             .REMAP32M     ({brg_remap32_o[1], brg_remap32_o[0]}),
             .REMAP256M    (brg_remap256_o),
             .IRQ_SCAN_EN  (brg_irq_scanen_o),
             .IRQ_MASK     ({64'h0, brg_irq_mask_o[1], brg_irq_mask_o[0]}),
             .IRQ_LEN      (brg_irq_len_o),
             .IRQ_OFF      (brg_irq_off_o),
             .IRQ          (),
             //.CSW_FIXED    (brg_csw_fixed_o),
             
             // AHB3 slave interface
             .HSEL         (ahb3_brg_HSEL),
             .HADDR        (ahb3_brg_HADDR),
             .HWDATA       (ahb3_brg_HWDATA),
             .HRDATA       (ahb3_brg_HRDATA),
             .HWRITE       (ahb3_brg_HWRITE),
             .HSIZE        (ahb3_brg_HSIZE),
             .HBURST       (ahb3_brg_HBURST),
             .HPROT        (ahb3_brg_HPROT),
             .HTRANS       (ahb3_brg_HTRANS),
             .HREADYOUT    (ahb3_brg_HREADYOUT),
             .HREADY       (ahb3_brg_HREADY),
             .HRESP        (ahb3_brg_HRESP),

             // SWD bridge bus
             .SWDIN        (BRG_SWDIN),
             .SWDOUT       (BRG_SWDOUT),
             .SWDOE        (BRG_SWDOE),
             .SWDCLK       (BRG_SWDCLK)
             );
   
   // Master <=> arbiter
   wire master_RDEN, master_WREN, master_WRFULL, master_RDEMPTY;
   wire [7:0] master_RDDATA, master_WRDATA;

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
                    .RDEN      (master_RDEN),
                    .RDEMPTY   (master_RDEMPTY),
                    .RDDATA    (master_RDDATA),
                    .WREN      (master_WREN),
                    .WRFULL    (master_WRFULL),
                    .WRDATA    (master_WRDATA)
                    );

   // Slave <=> arbiter
   wire slave_RDEN, slave_WREN, slave_WRFULL, slave_RDEMPTY;
   wire [7:0] slave_RDDATA, slave_WRDATA;

   // Shuttle unmatched bus transactions to host
   ahb3lite_host_slave
     u_host_slave (
                   .CLK       (CLK),
                   .RESETn    (PORESETn),
                   .EN        (slave_en_o),
                   .HSEL      (ahb3_host_slave_HSEL),
                   .HADDR     (ahb3_host_slave_HADDR),
                   .HWDATA    (ahb3_host_slave_HWDATA),
                   .HTRANS    (ahb3_host_slave_HTRANS),
                   .HSIZE     (ahb3_host_slave_HSIZE),
                   .HBURST    (ahb3_host_slave_HBURST),
                   .HPROT     (ahb3_host_slave_HPROT),
                   .HWRITE    (ahb3_host_slave_HWRITE),
                   .HRDATA    (ahb3_host_slave_HRDATA),
                   .HRESP     (ahb3_host_slave_HRESP),
                   .HREADY    (ahb3_host_slave_HREADY),
                   .HREADYOUT (ahb3_host_slave_HREADYOUT),
                   .RDEN      (slave_RDEN),
                   .RDEMPTY   (slave_RDEMPTY),
                   .WREN      (slave_WREN),
                   .WRFULL    (slave_WRFULL),
                   .RDDATA    (slave_RDDATA),
                   .WRDATA    (slave_WRDATA)
                   );

   // ARB <=> FIFO
   wire arb_RDEN, arb_WREN, arb_WRFULL, arb_RDEMPTY;
   wire [7:0] arb_RDDATA, arb_WRDATA;

   // FIFO <=> Transport
   wire trans_RDEN, trans_WREN, trans_WRFULL, trans_RDEMPTY;
   wire [7:0] trans_RDDATA, trans_WRDATA;

   // Create fifo arbiter to share connection with host
   fifo_arb #(
              .AW  (4),
              .DW  (8))
   u_fifo_arb (
               .CLK          (CLK),
               .RESETn       (PORESETn),
               // Connect to dual clock transport fifo
               .com_rden     (arb_RDEN),
               .com_rdempty  (arb_RDEMPTY),
               .com_rddata   (arb_RDDATA),
               .com_wren     (arb_WREN),
               .com_wrfull   (arb_WRFULL),
               .com_wrdata   (arb_WRDATA),
               // Connect to host_master (selmask matches)
               .c1_rden      (master_RDEN),
               .c1_rdempty   (master_RDEMPTY),
               .c1_rddata    (master_RDDATA),
               .c1_wren      (master_WREN),
               .c1_wrfull    (master_WRFULL),
               .c1_wrdata    (master_WRDATA),
               // Connect to host_slave
               .c2_rden      (slave_RDEN),
               .c2_rdempty   (slave_RDEMPTY),
               .c2_rddata    (slave_RDDATA),
               .c2_wren      (slave_WREN),
               .c2_wrfull    (slave_WRFULL),
               .c2_wrdata    (slave_WRDATA)
               );
   
   // Arb => Transport
   dual_clock_fifo #(
                     .ADDR_WIDTH   (8),
                     .DATA_WIDTH   (8))
   u_tx_fifo (
              .wr_clk_i   (CLK),
              .rd_clk_i   (TRANSPORT_CLK),
              .rd_rst_i   (~PORESETn),
              .wr_rst_i   (~PORESETn),
              .wr_en_i    (arb_WREN),
              .wr_data_i  (arb_WRDATA),
              .full_o     (arb_WRFULL),
              .rd_en_i    (trans_RDEN),
              .rd_data_o  (trans_RDDATA),
              .empty_o    (trans_RDEMPTY)
              );

   // Transport => Arb
   dual_clock_fifo #(
                     .ADDR_WIDTH   (8),
                     .DATA_WIDTH   (8))
   u_rx_fifo (
              .rd_clk_i   (CLK),
              .wr_clk_i   (TRANSPORT_CLK),
              .rd_rst_i   (~PORESETn),
              .wr_rst_i   (~PORESETn),
              .rd_en_i    (arb_RDEN),
              .rd_data_o  (arb_RDDATA),
              .empty_o    (arb_RDEMPTY),
              .wr_en_i    (trans_WREN),
              .wr_data_i  (trans_WRDATA),
              .full_o     (trans_WRFULL)
              );
  
 
   // Host transport
   uart_fifo 
     u_uart (
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
             .FIFO_DIN   (trans_RDDATA),
             // Dropped bytes
             .DROPPED    (dropped)
             );

   // Enable master ports
   assign ahb3_cm3_code_HSEL = 1'b1;
   assign ahb3_cm3_sys_HSEL = 1'b1;
   
   // Instantiate cortex-m3 core
   cm3_core
     #(
       .XILINX_ENC_CM3  (XILINX_ENC_CM3),
       .NUM_IRQ         (`IRQ_CNT),
       .SINGLE_MEMORY   (0),
       .CACHE_ENABLE    (0),
       .ROM_SZ          (ROM_SZ),
       .RAM_SZ          (RAM_SZ)
       )
     u_cm3 (
            // Clock and reset
            .FCLK         (CLK),
            .HCLK         (CLK),
            .PORESETn     (PORESETn),
            .CPURESETn    (cpureset_n & ~cpu_reset_o),
            .SYSRESETREQ  (sysresetreq),
            
            // IRQs
            .INTISR       (irq),
            .INTNMI       (1'b0),
            
            // Debug
            .SWCLKTCK     (TCK),
            .SWDITMS      (TMSIN),
            .SWDO         (TMSOUT),
            .SWDOEN       (TMSOE),
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
            .code_HADDR     (cm3_code_haddr),  // Run through redirect
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
            .sys_HADDR     (cm3_sys_haddr),
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
