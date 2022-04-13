# Setup global configs
set_property CFGBVS Vcco [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]

# System clock
create_clock -add -name sys_clk_pin -period 83.33 -waveform {0 41.66} [get_ports { CLK_12M }];

# 10MHz JTAG/SWD clock
create_clock -add -name jtag_clk -period 200.00 -waveform {0 100} [get_ports { TCK_SWDCLK }];

# Create virtual clock for IO
create_clock -name slow_clk -period 1000.0

# Set transport clock as async to hclk
set_clock_groups -asynchronous -group transport_clk -group hclk -group jtag_clk
