################################################################################

# This XDC is used only for OOC mode of synthesis, implementation
# This constraints file contains default clock frequencies to be used during
# out-of-context flows such as OOC Synthesis and Hierarchical Designs.
# This constraints file is not used in normal top-down synthesis (default flow
# of Vivado)
################################################################################
create_clock -name HDMI1_LLC -period 6.154 [get_ports HDMI1_LLC]
create_clock -name MGTCLK0_clk_p -period 6.400 [get_ports MGTCLK0_clk_p]
create_clock -name default_sysclk_250_clk_p -period 4 [get_ports default_sysclk_250_clk_p]

################################################################################