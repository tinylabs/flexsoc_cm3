/**
 *  Flexible System On Chip emulator for ARM Cortex-M3 devices
 *
 *  All rights reserved 
 *  Tiny Labs Inc
 *  2020
 */

module io_loopback
  (
   // Clock and reset
   input  CLK,
   input  PORESETn,
   input  TRANSPORT_CLK,
   
   // Host interface
   output UART_TX,
   input  UART_RX
   );

   // ARB <=> FIFO
   logic    arb_RDEN, arb_WREN, arb_WRFULL, arb_RDEMPTY;
   wire [7:0] arb_RDDATA;
   wire [7:0]  arb_WRDATA;
   
   // FIFO <=> Transport
   logic      trans_RDEN, trans_WREN, trans_WRFULL, trans_RDEMPTY;
   wire [7:0] trans_RDDATA, trans_WRDATA;
   
   // Loopback for link test
   assign arb_RDEN = ~arb_WRFULL & ~arb_RDEMPTY;
   assign arb_WRDATA = arb_RDDATA;

   always @(posedge CLK)
     begin
        if (arb_RDEN)
          arb_WREN <= 1;
        else
          arb_WREN <= 0;
     end

   // Arb => Transport
   dual_clock_fifo #(
                     .ADDR_WIDTH   (14),
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
                     .ADDR_WIDTH   (14),
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
             .FIFO_DIN   (trans_RDDATA)
             );
   
endmodule // io_loopback
