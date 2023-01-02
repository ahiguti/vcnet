
################################################################
# This is a generated script based on design: vcnet_10g
#
# Though there are limitations about the generated script,
# the main purpose of this utility is to make learning
# IP Integrator Tcl commands easier.
################################################################

namespace eval _tcl {
proc get_script_folder {} {
   set script_path [file normalize [info script]]
   set script_folder [file dirname $script_path]
   return $script_folder
}
}
variable script_folder
set script_folder [_tcl::get_script_folder]

################################################################
# Check if script is running in correct Vivado version.
################################################################
set scripts_vivado_version 2018.2
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   catch {common::send_msg_id "BD_TCL-109" "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."}

   return 1
}

################################################################
# START
################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source vcnet_10g_script.tcl


# The design that will be created by this Tcl script contains the following 
# module references:
# axis_audio_add_len, hdmi_sig_count, i2s_axis, rgb_to_axis, adv7611_init, i2c_master, address_mac_ip, axis_to_xgmii, axis_udp_ip_ether, eth_ctrl, peer_addr_expire, xgmii_arbiter_exclusive, xgmii_arp_resp, xgmii_fcs, xgmii_to_axis, xgmii_to_axis, xgmii_udp_echo, axis_to_i2cmaster, i2c_master, vcnet_ir_control, vcnet_parse_command, vcnet_reg_array, vcnet_spi_peripheral_multi

# Please add the sources of those modules before sourcing this Tcl script.

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xcku040-fbva676-1-c
   set_property BOARD_PART em.avnet.com:ku040:part0:1.0 [current_project]
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name vcnet_10g

# If you do not already have an existing IP Integrator design open,
# you can create a design using the following command:
#    create_bd_design $design_name

# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

if { ${design_name} eq "" } {
   # USE CASES:
   #    1) Design_name not set

   set errMsg "Please set the variable <design_name> to a non-empty value."
   set nRet 1

} elseif { ${cur_design} ne "" && ${list_cells} eq "" } {
   # USE CASES:
   #    2): Current design opened AND is empty AND names same.
   #    3): Current design opened AND is empty AND names diff; design_name NOT in project.
   #    4): Current design opened AND is empty AND names diff; design_name exists in project.

   if { $cur_design ne $design_name } {
      common::send_msg_id "BD_TCL-001" "INFO" "Changing value of <design_name> from <$design_name> to <$cur_design> since current design is empty."
      set design_name [get_property NAME $cur_design]
   }
   common::send_msg_id "BD_TCL-002" "INFO" "Constructing design in IPI design <$cur_design>..."

} elseif { ${cur_design} ne "" && $list_cells ne "" && $cur_design eq $design_name } {
   # USE CASES:
   #    5) Current design opened AND has components AND same names.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 1
} elseif { [get_files -quiet ${design_name}.bd] ne "" } {
   # USE CASES: 
   #    6) Current opened design, has components, but diff names, design_name exists in project.
   #    7) No opened design, design_name exists in project.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 2

} else {
   # USE CASES:
   #    8) No opened design, design_name not in project.
   #    9) Current opened design, has components, but diff names, design_name not in project.

   common::send_msg_id "BD_TCL-003" "INFO" "Currently there is no design <$design_name> in project, so creating one..."

   create_bd_design $design_name

   common::send_msg_id "BD_TCL-004" "INFO" "Making design <$design_name> as current_bd_design."
   current_bd_design $design_name

}

common::send_msg_id "BD_TCL-005" "INFO" "Currently the variable <design_name> is equal to \"$design_name\"."

if { $nRet != 0 } {
   catch {common::send_msg_id "BD_TCL-114" "ERROR" $errMsg}
   return $nRet
}

set bCheckIPsPassed 1
##################################################################
# CHECK IPs
##################################################################
set bCheckIPs 1
if { $bCheckIPs == 1 } {
   set list_check_ips "\ 
xilinx.com:ip:clk_wiz:6.0\
xilinx.com:ip:proc_sys_reset:5.0\
xilinx.com:ip:xlconcat:2.1\
xilinx.com:ip:axis_data_fifo:1.1\
xilinx.com:ip:axis_dwidth_converter:1.1\
xilinx.com:ip:xlconstant:1.1\
xilinx.com:ip:ten_gig_eth_pcs_pma:6.0\
xilinx.com:ip:util_vector_logic:2.0\
xilinx.com:ip:system_ila:1.1\
xilinx.com:ip:vio:3.0\
xilinx.com:ip:xlslice:1.0\
"

   set list_ips_missing ""
   common::send_msg_id "BD_TCL-006" "INFO" "Checking if the following IPs exist in the project's IP catalog: $list_check_ips ."

   foreach ip_vlnv $list_check_ips {
      set ip_obj [get_ipdefs -all $ip_vlnv]
      if { $ip_obj eq "" } {
         lappend list_ips_missing $ip_vlnv
      }
   }

   if { $list_ips_missing ne "" } {
      catch {common::send_msg_id "BD_TCL-115" "ERROR" "The following IPs are not found in the IP Catalog:\n  $list_ips_missing\n\nResolution: Please add the repository containing the IP(s) to the project." }
      set bCheckIPsPassed 0
   }

}

##################################################################
# CHECK Modules
##################################################################
set bCheckModules 1
if { $bCheckModules == 1 } {
   set list_check_mods "\ 
axis_audio_add_len\
hdmi_sig_count\
i2s_axis\
rgb_to_axis\
adv7611_init\
i2c_master\
address_mac_ip\
axis_to_xgmii\
axis_udp_ip_ether\
eth_ctrl\
peer_addr_expire\
xgmii_arbiter_exclusive\
xgmii_arp_resp\
xgmii_fcs\
xgmii_to_axis\
xgmii_to_axis\
xgmii_udp_echo\
axis_to_i2cmaster\
i2c_master\
vcnet_ir_control\
vcnet_parse_command\
vcnet_reg_array\
vcnet_spi_peripheral_multi\
"

   set list_mods_missing ""
   common::send_msg_id "BD_TCL-006" "INFO" "Checking if the following modules exist in the project's sources: $list_check_mods ."

   foreach mod_vlnv $list_check_mods {
      if { [can_resolve_reference $mod_vlnv] == 0 } {
         lappend list_mods_missing $mod_vlnv
      }
   }

   if { $list_mods_missing ne "" } {
      catch {common::send_msg_id "BD_TCL-115" "ERROR" "The following module(s) are not found in the project: $list_mods_missing" }
      common::send_msg_id "BD_TCL-008" "INFO" "Please add source files for the missing module(s) above."
      set bCheckIPsPassed 0
   }
}

if { $bCheckIPsPassed != 1 } {
  common::send_msg_id "BD_TCL-1003" "WARNING" "Will not continue with creation of design due to the error(s) above."
  return 3
}

##################################################################
# DESIGN PROCs
##################################################################


# Hierarchical cell: vcnet_control
proc create_hier_cell_vcnet_control { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_msg_id "BD_TCL-102" "ERROR" "create_hier_cell_vcnet_control() - Empty argument(s)!"}
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_msg_id "BD_TCL-100" "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_msg_id "BD_TCL-101" "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 ADV7611_CMD
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 UDP_RECV

  # Create pins
  create_bd_pin -dir I -type clk CLK100
  create_bd_pin -dir I -type rst CLK100_RESETN
  create_bd_pin -dir I -type clk ETERHENT_CLK
  create_bd_pin -dir I -type rst ETHERNET_RESETN
  create_bd_pin -dir O -from 4 -to 0 EXT_MCU_DOUT
  create_bd_pin -dir IO EXT_MCU_SCL
  create_bd_pin -dir I -from 4 -to 0 EXT_MCU_SCLK
  create_bd_pin -dir IO EXT_MCU_SDA
  create_bd_pin -dir O IR_OUT
  create_bd_pin -dir O -from 0 -to 0 LED

  # Create instance: axis_dwidth_converter_0, and set properties
  set axis_dwidth_converter_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_dwidth_converter:1.1 axis_dwidth_converter_0 ]
  set_property -dict [ list \
   CONFIG.HAS_MI_TKEEP {0} \
   CONFIG.HAS_TLAST {1} \
   CONFIG.HAS_TSTRB {1} \
   CONFIG.M_TDATA_NUM_BYTES {2} \
   CONFIG.S_TDATA_NUM_BYTES {8} \
 ] $axis_dwidth_converter_0

  # Create instance: axis_to_i2cmaster_extmcu, and set properties
  set block_name axis_to_i2cmaster
  set block_cell_name axis_to_i2cmaster_extmcu
  if { [catch {set axis_to_i2cmaster_extmcu [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $axis_to_i2cmaster_extmcu eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: cdc_ethernet_to_100, and set properties
  set cdc_ethernet_to_100 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:1.1 cdc_ethernet_to_100 ]
  set_property -dict [ list \
   CONFIG.IS_ACLK_ASYNC {1} \
 ] $cdc_ethernet_to_100

  # Create instance: fifo_extmcu_cmd, and set properties
  set fifo_extmcu_cmd [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:1.1 fifo_extmcu_cmd ]

  # Create instance: fifo_ir_cmd, and set properties
  set fifo_ir_cmd [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:1.1 fifo_ir_cmd ]
  set_property -dict [ list \
   CONFIG.FIFO_MODE {2} \
 ] $fifo_ir_cmd

  # Create instance: i2c_master_extmcu, and set properties
  set block_name i2c_master
  set block_cell_name i2c_master_extmcu
  if { [catch {set i2c_master_extmcu [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $i2c_master_extmcu eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property -dict [ list \
   CONFIG.CLOCK_DIV {250} \
 ] $i2c_master_extmcu

  # Create instance: system_ila_0, and set properties
  set system_ila_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila_0 ]
  set_property -dict [ list \
   CONFIG.C_DATA_DEPTH {16384} \
   CONFIG.C_MON_TYPE {MIX} \
   CONFIG.C_NUM_MONITOR_SLOTS {3} \
   CONFIG.C_NUM_OF_PROBES {2} \
   CONFIG.C_SLOT {2} \
   CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
   CONFIG.C_SLOT_1_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
   CONFIG.C_SLOT_2_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
 ] $system_ila_0

  # Create instance: system_ila_2, and set properties
  set system_ila_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila_2 ]
  set_property -dict [ list \
   CONFIG.ALL_PROBE_SAME_MU_CNT {1} \
   CONFIG.C_DATA_DEPTH {16384} \
   CONFIG.C_EN_STRG_QUAL {0} \
   CONFIG.C_MON_TYPE {NATIVE} \
   CONFIG.C_NUM_MONITOR_SLOTS {5} \
   CONFIG.C_NUM_OF_PROBES {5} \
   CONFIG.C_PROBE0_MU_CNT {1} \
   CONFIG.C_PROBE1_MU_CNT {1} \
   CONFIG.C_PROBE1_WIDTH {8} \
   CONFIG.C_PROBE2_MU_CNT {1} \
   CONFIG.C_PROBE2_WIDTH {32} \
   CONFIG.C_PROBE3_MU_CNT {1} \
   CONFIG.C_PROBE3_WIDTH {5} \
   CONFIG.C_PROBE4_MU_CNT {1} \
   CONFIG.C_PROBE4_WIDTH {5} \
   CONFIG.C_PROBE_WIDTH_PROPAGATION {MANUAL} \
 ] $system_ila_2

  # Create instance: vcnet_ir_control_0, and set properties
  set block_name vcnet_ir_control
  set block_cell_name vcnet_ir_control_0
  if { [catch {set vcnet_ir_control_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $vcnet_ir_control_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: vcnet_parse_command_0, and set properties
  set block_name vcnet_parse_command
  set block_cell_name vcnet_parse_command_0
  if { [catch {set vcnet_parse_command_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $vcnet_parse_command_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: vcnet_reg_array_0, and set properties
  set block_name vcnet_reg_array
  set block_cell_name vcnet_reg_array_0
  if { [catch {set vcnet_reg_array_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $vcnet_reg_array_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: vcnet_spi_peripheral_1, and set properties
  set block_name vcnet_spi_peripheral_multi
  set block_cell_name vcnet_spi_peripheral_1
  if { [catch {set vcnet_spi_peripheral_1 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $vcnet_spi_peripheral_1 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: vio_0, and set properties
  set vio_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:vio:3.0 vio_0 ]
  set_property -dict [ list \
   CONFIG.C_NUM_PROBE_IN {2} \
   CONFIG.C_NUM_PROBE_OUT {0} \
 ] $vio_0

  # Create instance: xlconcat_0, and set properties
  set xlconcat_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0 ]
  set_property -dict [ list \
   CONFIG.NUM_PORTS {1} \
 ] $xlconcat_0

  # Create instance: xlslice_0, and set properties
  set xlslice_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 xlslice_0 ]
  set_property -dict [ list \
   CONFIG.DIN_FROM {7} \
   CONFIG.DIN_TO {0} \
   CONFIG.DIN_WIDTH {48} \
   CONFIG.DOUT_WIDTH {8} \
 ] $xlslice_0

  # Create instance: xlslice_1, and set properties
  set xlslice_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 xlslice_1 ]
  set_property -dict [ list \
   CONFIG.DIN_FROM {47} \
   CONFIG.DIN_TO {16} \
   CONFIG.DIN_WIDTH {48} \
   CONFIG.DOUT_WIDTH {1} \
 ] $xlslice_1

  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins UDP_RECV] [get_bd_intf_pins cdc_ethernet_to_100/S_AXIS]
  connect_bd_intf_net -intf_net axis_data_fifo_0_M_AXIS [get_bd_intf_pins axis_to_i2cmaster_extmcu/I] [get_bd_intf_pins fifo_extmcu_cmd/M_AXIS]
  connect_bd_intf_net -intf_net axis_data_fifo_1_M_AXIS [get_bd_intf_pins axis_dwidth_converter_0/S_AXIS] [get_bd_intf_pins fifo_ir_cmd/M_AXIS]
  connect_bd_intf_net -intf_net axis_dwidth_converter_0_M_AXIS [get_bd_intf_pins axis_dwidth_converter_0/M_AXIS] [get_bd_intf_pins vcnet_ir_control_0/I]
  connect_bd_intf_net -intf_net [get_bd_intf_nets axis_dwidth_converter_0_M_AXIS] [get_bd_intf_pins system_ila_0/SLOT_2_AXIS] [get_bd_intf_pins vcnet_ir_control_0/I]
  connect_bd_intf_net -intf_net cdc_ethernet_to_100_M_AXIS [get_bd_intf_pins cdc_ethernet_to_100/M_AXIS] [get_bd_intf_pins vcnet_parse_command_0/I]
  connect_bd_intf_net -intf_net [get_bd_intf_nets cdc_ethernet_to_100_M_AXIS] [get_bd_intf_pins system_ila_0/SLOT_0_AXIS] [get_bd_intf_pins vcnet_parse_command_0/I]
  connect_bd_intf_net -intf_net vcnet_parse_command_0_ADV7611 [get_bd_intf_pins ADV7611_CMD] [get_bd_intf_pins vcnet_parse_command_0/ADV7611]
  connect_bd_intf_net -intf_net vcnet_parse_command_0_EXT [get_bd_intf_pins fifo_extmcu_cmd/S_AXIS] [get_bd_intf_pins vcnet_parse_command_0/EXT]
  connect_bd_intf_net -intf_net vcnet_parse_command_0_IR [get_bd_intf_pins fifo_ir_cmd/S_AXIS] [get_bd_intf_pins vcnet_parse_command_0/IR]
  connect_bd_intf_net -intf_net [get_bd_intf_nets vcnet_parse_command_0_IR] [get_bd_intf_pins system_ila_0/SLOT_1_AXIS] [get_bd_intf_pins vcnet_parse_command_0/IR]

  # Create port connections
  connect_bd_net -net EXT_MCU_SCLK_1 [get_bd_pins EXT_MCU_SCLK] [get_bd_pins system_ila_2/probe3] [get_bd_pins vcnet_spi_peripheral_1/SCLK]
  connect_bd_net -net Net [get_bd_pins EXT_MCU_SCL] [get_bd_pins i2c_master_extmcu/I2C_SCL]
  connect_bd_net -net Net1 [get_bd_pins EXT_MCU_SDA] [get_bd_pins i2c_master_extmcu/I2C_SDA]
  connect_bd_net -net axis_to_i2cmaster_0_I2CM_DEVADDR [get_bd_pins axis_to_i2cmaster_extmcu/I2CM_DEVADDR] [get_bd_pins i2c_master_extmcu/DEVADDR]
  connect_bd_net -net axis_to_i2cmaster_0_I2CM_RBYTES [get_bd_pins axis_to_i2cmaster_extmcu/I2CM_RBYTES] [get_bd_pins i2c_master_extmcu/RBYTES]
  connect_bd_net -net axis_to_i2cmaster_0_I2CM_REGADDR [get_bd_pins axis_to_i2cmaster_extmcu/I2CM_REGADDR] [get_bd_pins i2c_master_extmcu/REGADDR]
  connect_bd_net -net axis_to_i2cmaster_0_I2CM_VALID [get_bd_pins axis_to_i2cmaster_extmcu/I2CM_VALID] [get_bd_pins i2c_master_extmcu/VALID]
  connect_bd_net -net axis_to_i2cmaster_0_I2CM_WBYTES [get_bd_pins axis_to_i2cmaster_extmcu/I2CM_WBYTES] [get_bd_pins i2c_master_extmcu/WBYTES]
  connect_bd_net -net axis_to_i2cmaster_0_I2CM_WDATA [get_bd_pins axis_to_i2cmaster_extmcu/I2CM_WDATA] [get_bd_pins i2c_master_extmcu/WDATA]
  connect_bd_net -net i2c_master_0_ERROR [get_bd_pins axis_to_i2cmaster_extmcu/I2CM_ERR] [get_bd_pins i2c_master_extmcu/ERROR]
  connect_bd_net -net i2c_master_0_RDATA [get_bd_pins axis_to_i2cmaster_extmcu/I2CM_RDATA] [get_bd_pins i2c_master_extmcu/RDATA]
  connect_bd_net -net i2c_master_0_READY [get_bd_pins axis_to_i2cmaster_extmcu/I2CM_READY] [get_bd_pins i2c_master_extmcu/READY]
  connect_bd_net -net m_axis_aclk_1 [get_bd_pins CLK100] [get_bd_pins axis_dwidth_converter_0/aclk] [get_bd_pins axis_to_i2cmaster_extmcu/CLK] [get_bd_pins cdc_ethernet_to_100/m_axis_aclk] [get_bd_pins fifo_extmcu_cmd/s_axis_aclk] [get_bd_pins fifo_ir_cmd/s_axis_aclk] [get_bd_pins i2c_master_extmcu/CLK] [get_bd_pins system_ila_0/clk] [get_bd_pins system_ila_2/clk] [get_bd_pins vcnet_ir_control_0/CLK] [get_bd_pins vcnet_parse_command_0/CLK] [get_bd_pins vcnet_reg_array_0/CLK] [get_bd_pins vcnet_spi_peripheral_1/CLK] [get_bd_pins vio_0/clk]
  connect_bd_net -net m_axis_aresetn_1 [get_bd_pins CLK100_RESETN] [get_bd_pins axis_dwidth_converter_0/aresetn] [get_bd_pins axis_to_i2cmaster_extmcu/RESET_N] [get_bd_pins cdc_ethernet_to_100/m_axis_aresetn] [get_bd_pins fifo_extmcu_cmd/s_axis_aresetn] [get_bd_pins fifo_ir_cmd/s_axis_aresetn] [get_bd_pins i2c_master_extmcu/RESET_N] [get_bd_pins system_ila_0/resetn] [get_bd_pins vcnet_ir_control_0/RESETN] [get_bd_pins vcnet_parse_command_0/RESETN] [get_bd_pins vcnet_reg_array_0/RESETN] [get_bd_pins vcnet_spi_peripheral_1/RESETN]
  connect_bd_net -net regs_spi_peripheral_0_DOUT [get_bd_pins EXT_MCU_DOUT] [get_bd_pins system_ila_2/probe4] [get_bd_pins vcnet_spi_peripheral_1/DOUT]
  connect_bd_net -net s_axis_aclk_1 [get_bd_pins ETERHENT_CLK] [get_bd_pins cdc_ethernet_to_100/s_axis_aclk]
  connect_bd_net -net s_axis_aresetn_1 [get_bd_pins ETHERNET_RESETN] [get_bd_pins cdc_ethernet_to_100/s_axis_aresetn]
  connect_bd_net -net vcnet_ir_control_0_DBG_STATE [get_bd_pins system_ila_0/probe1] [get_bd_pins vcnet_ir_control_0/DBG_STATE]
  connect_bd_net -net vcnet_ir_control_0_IR_OUT1 [get_bd_pins IR_OUT] [get_bd_pins system_ila_0/probe0] [get_bd_pins vcnet_ir_control_0/IR_OUT] [get_bd_pins xlconcat_0/In0]
  connect_bd_net -net vcnet_parse_command_0_EXT2_TDATA [get_bd_pins vcnet_parse_command_0/EXT2_TDATA] [get_bd_pins xlslice_0/Din] [get_bd_pins xlslice_1/Din]
  connect_bd_net -net vcnet_parse_command_0_EXT2_TVALID [get_bd_pins system_ila_2/probe0] [get_bd_pins vcnet_parse_command_0/EXT2_TVALID] [get_bd_pins vcnet_reg_array_0/WR_ENABLE]
  connect_bd_net -net vcnet_reg_array_0_VAL_ARRAY [get_bd_pins vcnet_reg_array_0/VAL_ARRAY] [get_bd_pins vcnet_spi_peripheral_1/VAL_ARRAY] [get_bd_pins vio_0/probe_in1]
  connect_bd_net -net vcnet_spi_peripheral_1_STAT_INTERVAL [get_bd_pins vcnet_spi_peripheral_1/STAT_INTERVAL] [get_bd_pins vio_0/probe_in0]
  connect_bd_net -net xlconcat_0_dout [get_bd_pins LED] [get_bd_pins xlconcat_0/dout]
  connect_bd_net -net xlslice_0_Dout [get_bd_pins system_ila_2/probe1] [get_bd_pins vcnet_reg_array_0/WR_IDX] [get_bd_pins xlslice_0/Dout]
  connect_bd_net -net xlslice_1_Dout [get_bd_pins system_ila_2/probe2] [get_bd_pins vcnet_reg_array_0/WR_VALUE] [get_bd_pins xlslice_1/Dout]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: udp_server_10g
proc create_hier_cell_udp_server_10g { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_msg_id "BD_TCL-102" "ERROR" "create_hier_cell_udp_server_10g() - Empty argument(s)!"}
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_msg_id "BD_TCL-100" "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_msg_id "BD_TCL-101" "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 ETHERNET_SEND
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 MGTCLK0
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 UDP_RECV

  # Create pins
  create_bd_pin -dir I -type clk CLK100
  create_bd_pin -dir I -type rst CLK100_RESET
  create_bd_pin -dir O -type clk ETHERNET_CLK
  create_bd_pin -dir O -from 0 -to 0 -type rst ETHERNET_RESETN
  create_bd_pin -dir O -from 1 -to 0 LED
  create_bd_pin -dir I SFP1_RX_LOS
  create_bd_pin -dir I SFP1_RX_N
  create_bd_pin -dir I SFP1_RX_P
  create_bd_pin -dir O SFP1_TX_DISABLE
  create_bd_pin -dir O SFP1_TX_N
  create_bd_pin -dir O SFP1_TX_P

  # Create instance: address_mac_ip_0, and set properties
  set block_name address_mac_ip
  set block_cell_name address_mac_ip_0
  if { [catch {set address_mac_ip_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $address_mac_ip_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: arb_suppress_0, and set properties
  set arb_suppress_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 arb_suppress_0 ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0} \
 ] $arb_suppress_0

  # Create instance: arb_suppress_1, and set properties
  set arb_suppress_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 arb_suppress_1 ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0} \
 ] $arb_suppress_1

  # Create instance: axis_interconnect_0, and set properties
  set axis_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_interconnect:2.1 axis_interconnect_0 ]
  set_property -dict [ list \
   CONFIG.ARB_ALGORITHM {0} \
   CONFIG.ARB_ON_MAX_XFERS {0} \
   CONFIG.ARB_ON_TLAST {1} \
   CONFIG.M00_FIFO_MODE {1} \
   CONFIG.NUM_MI {1} \
   CONFIG.NUM_SI {2} \
   CONFIG.S00_FIFO_DEPTH {1024} \
   CONFIG.S00_FIFO_MODE {1} \
   CONFIG.S01_FIFO_DEPTH {1024} \
   CONFIG.S01_FIFO_MODE {1} \
 ] $axis_interconnect_0

  # Create instance: axis_to_xgmii_0, and set properties
  set block_name axis_to_xgmii
  set block_cell_name axis_to_xgmii_0
  if { [catch {set axis_to_xgmii_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $axis_to_xgmii_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: axis_udp_ip_ether_0, and set properties
  set block_name axis_udp_ip_ether
  set block_cell_name axis_udp_ip_ether_0
  if { [catch {set axis_udp_ip_ether_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $axis_udp_ip_ether_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: eth_ctrl_0, and set properties
  set block_name eth_ctrl
  set block_cell_name eth_ctrl_0
  if { [catch {set eth_ctrl_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $eth_ctrl_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: fifo_packet, and set properties
  set fifo_packet [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:1.1 fifo_packet ]
  set_property -dict [ list \
   CONFIG.FIFO_MODE {2} \
   CONFIG.HAS_TKEEP {0} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.TDATA_NUM_BYTES {8} \
   CONFIG.TUSER_WIDTH {4} \
 ] $fifo_packet

  # Create instance: peer_addr_expire_0, and set properties
  set block_name peer_addr_expire
  set block_cell_name peer_addr_expire_0
  if { [catch {set peer_addr_expire_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $peer_addr_expire_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: ten_gig_eth_pcs_pma_0, and set properties
  set ten_gig_eth_pcs_pma_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ten_gig_eth_pcs_pma:6.0 ten_gig_eth_pcs_pma_0 ]
  set_property -dict [ list \
   CONFIG.Locations {X0Y8} \
   CONFIG.RefClk {clk0} \
   CONFIG.SupportLevel {1} \
   CONFIG.base_kr {BASE-R} \
   CONFIG.no_ebuff {false} \
 ] $ten_gig_eth_pcs_pma_0

  # Create instance: util_vector_logic_0, and set properties
  set util_vector_logic_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_0 ]
  set_property -dict [ list \
   CONFIG.C_OPERATION {not} \
   CONFIG.C_SIZE {1} \
 ] $util_vector_logic_0

  # Create instance: xgmii_arbiter_exclus_0, and set properties
  set block_name xgmii_arbiter_exclusive
  set block_cell_name xgmii_arbiter_exclus_0
  if { [catch {set xgmii_arbiter_exclus_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $xgmii_arbiter_exclus_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: xgmii_arp_resp_0, and set properties
  set block_name xgmii_arp_resp
  set block_cell_name xgmii_arp_resp_0
  if { [catch {set xgmii_arp_resp_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $xgmii_arp_resp_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: xgmii_fcs_0, and set properties
  set block_name xgmii_fcs
  set block_cell_name xgmii_fcs_0
  if { [catch {set xgmii_fcs_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $xgmii_fcs_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: xgmii_to_axis_0, and set properties
  set block_name xgmii_to_axis
  set block_cell_name xgmii_to_axis_0
  if { [catch {set xgmii_to_axis_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $xgmii_to_axis_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: xgmii_to_axis_1, and set properties
  set block_name xgmii_to_axis
  set block_cell_name xgmii_to_axis_1
  if { [catch {set xgmii_to_axis_1 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $xgmii_to_axis_1 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: xgmii_udp_echo_0, and set properties
  set block_name xgmii_udp_echo
  set block_cell_name xgmii_udp_echo_0
  if { [catch {set xgmii_udp_echo_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $xgmii_udp_echo_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: xlconcat_0, and set properties
  set xlconcat_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0 ]
  set_property -dict [ list \
   CONFIG.NUM_PORTS {2} \
 ] $xlconcat_0

  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins ETHERNET_SEND] [get_bd_intf_pins axis_udp_ip_ether_0/I]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins MGTCLK0] [get_bd_intf_pins ten_gig_eth_pcs_pma_0/refclk_diff_port]
  connect_bd_intf_net -intf_net Conn3 [get_bd_intf_pins UDP_RECV] [get_bd_intf_pins xgmii_to_axis_1/O]
  connect_bd_intf_net -intf_net S00_AXIS_1 [get_bd_intf_pins axis_interconnect_0/S00_AXIS] [get_bd_intf_pins fifo_packet/M_AXIS]
  connect_bd_intf_net -intf_net axis_interconnect_0_M00_AXIS [get_bd_intf_pins axis_interconnect_0/M00_AXIS] [get_bd_intf_pins axis_to_xgmii_0/I]
  connect_bd_intf_net -intf_net axis_udp_ip_ether_0_O [get_bd_intf_pins axis_interconnect_0/S01_AXIS] [get_bd_intf_pins axis_udp_ip_ether_0/O]
  connect_bd_intf_net -intf_net ten_gig_eth_pcs_pma_0_core_to_gt_drp [get_bd_intf_pins ten_gig_eth_pcs_pma_0/core_to_gt_drp] [get_bd_intf_pins ten_gig_eth_pcs_pma_0/gt_drp]
  connect_bd_intf_net -intf_net xgmii_to_axis_0_O [get_bd_intf_pins fifo_packet/S_AXIS] [get_bd_intf_pins xgmii_to_axis_0/O]

  # Create port connections
  connect_bd_net -net S01_ARB_REQ_SUPPRESS_1 [get_bd_pins arb_suppress_1/dout] [get_bd_pins axis_interconnect_0/S01_ARB_REQ_SUPPRESS]
  connect_bd_net -net SFP1_RX_LOS_1 [get_bd_pins SFP1_RX_LOS] [get_bd_pins eth_ctrl_0/rx_loss]
  connect_bd_net -net SFP1_RX_N_1 [get_bd_pins SFP1_RX_N] [get_bd_pins ten_gig_eth_pcs_pma_0/rxn]
  connect_bd_net -net SFP1_RX_P_1 [get_bd_pins SFP1_RX_P] [get_bd_pins ten_gig_eth_pcs_pma_0/rxp]
  connect_bd_net -net address_mac_ip_0_IP_ADDR [get_bd_pins address_mac_ip_0/IP_ADDR] [get_bd_pins axis_udp_ip_ether_0/SRC_IP_ADDR] [get_bd_pins xgmii_arp_resp_0/LOCAL_IP] [get_bd_pins xgmii_udp_echo_0/LOCAL_IP]
  connect_bd_net -net address_mac_ip_0_IP_PORT [get_bd_pins address_mac_ip_0/IP_PORT] [get_bd_pins axis_udp_ip_ether_0/SRC_PORT] [get_bd_pins xgmii_udp_echo_0/LOCAL_PORT]
  connect_bd_net -net address_mac_ip_0_MAC_ADDR [get_bd_pins address_mac_ip_0/MAC_ADDR] [get_bd_pins axis_udp_ip_ether_0/SRC_MAC_ADDR] [get_bd_pins xgmii_arp_resp_0/LOCAL_MAC] [get_bd_pins xgmii_udp_echo_0/LOCAL_MAC]
  connect_bd_net -net axis_packet_gap_0_O_TDATA [get_bd_pins axis_to_xgmii_0/O_DATA] [get_bd_pins xgmii_fcs_0/TXD_IN]
  connect_bd_net -net axis_packet_gap_0_O_TUSER [get_bd_pins axis_to_xgmii_0/O_LEN] [get_bd_pins xgmii_fcs_0/TXLEN_IN]
  connect_bd_net -net const_zero_dout [get_bd_pins arb_suppress_0/dout] [get_bd_pins axis_interconnect_0/S00_ARB_REQ_SUPPRESS]
  connect_bd_net -net dclk_1 [get_bd_pins CLK100] [get_bd_pins ten_gig_eth_pcs_pma_0/dclk]
  connect_bd_net -net eth_ctrl_0_clken [get_bd_pins eth_ctrl_0/clken] [get_bd_pins xgmii_arp_resp_0/CE] [get_bd_pins xgmii_fcs_0/TX_CE] [get_bd_pins xgmii_udp_echo_0/CE]
  connect_bd_net -net eth_ctrl_0_mdc [get_bd_pins eth_ctrl_0/mdc] [get_bd_pins ten_gig_eth_pcs_pma_0/mdc]
  connect_bd_net -net eth_ctrl_0_mdio_w [get_bd_pins eth_ctrl_0/mdio_w] [get_bd_pins ten_gig_eth_pcs_pma_0/mdio_in]
  connect_bd_net -net eth_ctrl_0_pma_pmd_type [get_bd_pins eth_ctrl_0/pma_pmd_type] [get_bd_pins ten_gig_eth_pcs_pma_0/pma_pmd_type]
  connect_bd_net -net eth_ctrl_0_prtad [get_bd_pins eth_ctrl_0/prtad] [get_bd_pins ten_gig_eth_pcs_pma_0/prtad]
  connect_bd_net -net eth_ctrl_0_signal_detect [get_bd_pins eth_ctrl_0/signal_detect] [get_bd_pins ten_gig_eth_pcs_pma_0/signal_detect]
  connect_bd_net -net eth_ctrl_0_sim_speedup_control [get_bd_pins eth_ctrl_0/sim_speedup_control] [get_bd_pins ten_gig_eth_pcs_pma_0/sim_speedup_control]
  connect_bd_net -net eth_ctrl_0_tx_fault [get_bd_pins eth_ctrl_0/tx_fault] [get_bd_pins ten_gig_eth_pcs_pma_0/tx_fault]
  connect_bd_net -net peer_addr_expire_0_O_PEER_ADDR [get_bd_pins axis_udp_ip_ether_0/DST_IP_ADDR] [get_bd_pins peer_addr_expire_0/O_PEER_ADDR]
  connect_bd_net -net peer_addr_expire_0_O_PEER_ADDR_EN [get_bd_pins axis_udp_ip_ether_0/DST_ENABLE] [get_bd_pins peer_addr_expire_0/O_PEER_ADDR_EN] [get_bd_pins xlconcat_0/In1]
  connect_bd_net -net reset_1 [get_bd_pins CLK100_RESET] [get_bd_pins ten_gig_eth_pcs_pma_0/reset]
  connect_bd_net -net ten_gig_eth_pcs_pma_0_areset_datapathclk_out [get_bd_pins ten_gig_eth_pcs_pma_0/areset_datapathclk_out] [get_bd_pins util_vector_logic_0/Op1] [get_bd_pins xgmii_arp_resp_0/RESET] [get_bd_pins xgmii_fcs_0/RESET] [get_bd_pins xgmii_udp_echo_0/RESET]
  connect_bd_net -net ten_gig_eth_pcs_pma_0_drp_req [get_bd_pins ten_gig_eth_pcs_pma_0/drp_gnt] [get_bd_pins ten_gig_eth_pcs_pma_0/drp_req]
  connect_bd_net -net ten_gig_eth_pcs_pma_0_tx_disable [get_bd_pins SFP1_TX_DISABLE] [get_bd_pins ten_gig_eth_pcs_pma_0/tx_disable]
  connect_bd_net -net ten_gig_eth_pcs_pma_0_txn [get_bd_pins SFP1_TX_N] [get_bd_pins ten_gig_eth_pcs_pma_0/txn]
  connect_bd_net -net ten_gig_eth_pcs_pma_0_txp [get_bd_pins SFP1_TX_P] [get_bd_pins ten_gig_eth_pcs_pma_0/txp]
  connect_bd_net -net ten_gig_eth_pcs_pma_0_txusrclk2_out [get_bd_pins ETHERNET_CLK] [get_bd_pins axis_interconnect_0/ACLK] [get_bd_pins axis_interconnect_0/M00_AXIS_ACLK] [get_bd_pins axis_interconnect_0/S00_AXIS_ACLK] [get_bd_pins axis_interconnect_0/S01_AXIS_ACLK] [get_bd_pins axis_to_xgmii_0/CLK] [get_bd_pins axis_udp_ip_ether_0/CLK] [get_bd_pins fifo_packet/s_axis_aclk] [get_bd_pins peer_addr_expire_0/CLK] [get_bd_pins ten_gig_eth_pcs_pma_0/txusrclk2_out] [get_bd_pins xgmii_arp_resp_0/CLK] [get_bd_pins xgmii_fcs_0/CLK] [get_bd_pins xgmii_to_axis_0/CLK] [get_bd_pins xgmii_to_axis_1/CLK] [get_bd_pins xgmii_udp_echo_0/CLK] [get_bd_pins xlconcat_0/In0]
  connect_bd_net -net ten_gig_eth_pcs_pma_0_xgmii_rxc [get_bd_pins ten_gig_eth_pcs_pma_0/xgmii_rxc] [get_bd_pins xgmii_fcs_0/XGMII_RXC]
  connect_bd_net -net ten_gig_eth_pcs_pma_0_xgmii_rxd [get_bd_pins ten_gig_eth_pcs_pma_0/xgmii_rxd] [get_bd_pins xgmii_fcs_0/XGMII_RXD]
  connect_bd_net -net util_vector_logic_0_Res [get_bd_pins ETHERNET_RESETN] [get_bd_pins axis_interconnect_0/ARESETN] [get_bd_pins axis_interconnect_0/M00_AXIS_ARESETN] [get_bd_pins axis_interconnect_0/S00_AXIS_ARESETN] [get_bd_pins axis_interconnect_0/S01_AXIS_ARESETN] [get_bd_pins axis_to_xgmii_0/RESETN] [get_bd_pins axis_udp_ip_ether_0/RESETN] [get_bd_pins fifo_packet/s_axis_aresetn] [get_bd_pins peer_addr_expire_0/RESETN] [get_bd_pins util_vector_logic_0/Res]
  connect_bd_net -net xgmii_arbiter_exclus_0_D_OUT [get_bd_pins xgmii_arbiter_exclus_0/D_OUT] [get_bd_pins xgmii_to_axis_0/I_DATA]
  connect_bd_net -net xgmii_arbiter_exclus_0_LEN_OUT [get_bd_pins xgmii_arbiter_exclus_0/LEN_OUT] [get_bd_pins xgmii_to_axis_0/I_LEN]
  connect_bd_net -net xgmii_arp_resp_0_TXD_OUT [get_bd_pins xgmii_arbiter_exclus_0/D0_IN] [get_bd_pins xgmii_arp_resp_0/TXD_OUT]
  connect_bd_net -net xgmii_arp_resp_0_TXLEN_OUT [get_bd_pins xgmii_arbiter_exclus_0/LEN0_IN] [get_bd_pins xgmii_arp_resp_0/TXLEN_OUT]
  connect_bd_net -net xgmii_fcs_0_RXD_OUT [get_bd_pins xgmii_arp_resp_0/RXD_IN] [get_bd_pins xgmii_fcs_0/RXD_OUT] [get_bd_pins xgmii_udp_echo_0/RXD_IN]
  connect_bd_net -net xgmii_fcs_0_RXLEN_OUT [get_bd_pins xgmii_arp_resp_0/RXLEN_IN] [get_bd_pins xgmii_fcs_0/RXLEN_OUT] [get_bd_pins xgmii_udp_echo_0/RXLEN_IN]
  connect_bd_net -net xgmii_fcs_0_TXC_OUT [get_bd_pins ten_gig_eth_pcs_pma_0/xgmii_txc] [get_bd_pins xgmii_fcs_0/XGMII_TXC]
  connect_bd_net -net xgmii_fcs_0_XGMII_TXD [get_bd_pins ten_gig_eth_pcs_pma_0/xgmii_txd] [get_bd_pins xgmii_fcs_0/XGMII_TXD]
  connect_bd_net -net xgmii_to_axis_0_O_TDATA [get_bd_pins fifo_packet/s_axis_tdata] [get_bd_pins xgmii_to_axis_0/O_TDATA]
  connect_bd_net -net xgmii_to_axis_0_O_TLAST [get_bd_pins fifo_packet/s_axis_tlast] [get_bd_pins xgmii_to_axis_0/O_TLAST]
  connect_bd_net -net xgmii_to_axis_0_O_TUSER [get_bd_pins fifo_packet/s_axis_tuser] [get_bd_pins xgmii_to_axis_0/O_TUSER]
  connect_bd_net -net xgmii_to_axis_0_O_TVALID [get_bd_pins fifo_packet/s_axis_tvalid] [get_bd_pins xgmii_to_axis_0/O_TVALID]
  connect_bd_net -net xgmii_udp_echo_0_PEER_IP_ADDR [get_bd_pins peer_addr_expire_0/I_PEER_ADDR] [get_bd_pins xgmii_udp_echo_0/PEER_IP_ADDR]
  connect_bd_net -net xgmii_udp_echo_0_PEER_MAC_ADDR [get_bd_pins axis_udp_ip_ether_0/DST_MAC_ADDR] [get_bd_pins xgmii_udp_echo_0/PEER_MAC_ADDR]
  connect_bd_net -net xgmii_udp_echo_0_PEER_PORT [get_bd_pins axis_udp_ip_ether_0/DST_PORT] [get_bd_pins xgmii_udp_echo_0/PEER_PORT]
  connect_bd_net -net xgmii_udp_echo_0_TXD_OUT [get_bd_pins xgmii_arbiter_exclus_0/D1_IN] [get_bd_pins xgmii_udp_echo_0/TXD_OUT]
  connect_bd_net -net xgmii_udp_echo_0_TXLEN_OUT [get_bd_pins xgmii_arbiter_exclus_0/LEN1_IN] [get_bd_pins xgmii_udp_echo_0/TXLEN_OUT]
  connect_bd_net -net xgmii_udp_echo_0_UDP_D_OUT [get_bd_pins peer_addr_expire_0/I_UDP_D] [get_bd_pins xgmii_to_axis_1/I_DATA] [get_bd_pins xgmii_udp_echo_0/UDP_D_OUT]
  connect_bd_net -net xgmii_udp_echo_0_UDP_LEN_OUT [get_bd_pins peer_addr_expire_0/I_UDP_LEN] [get_bd_pins xgmii_to_axis_1/I_LEN] [get_bd_pins xgmii_udp_echo_0/UDP_LEN_OUT]
  connect_bd_net -net xlconcat_0_dout [get_bd_pins LED] [get_bd_pins xlconcat_0/dout]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: fmc_hdmi_adv7611_init
proc create_hier_cell_fmc_hdmi_adv7611_init { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_msg_id "BD_TCL-102" "ERROR" "create_hier_cell_fmc_hdmi_adv7611_init() - Empty argument(s)!"}
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_msg_id "BD_TCL-100" "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_msg_id "BD_TCL-101" "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 ADV7611_CMD

  # Create pins
  create_bd_pin -dir I -type clk CLK
  create_bd_pin -dir IO I2C_SCL
  create_bd_pin -dir IO I2C_SDA
  create_bd_pin -dir I -type rst RESET_N

  # Create instance: adv7611_init_0, and set properties
  set block_name adv7611_init
  set block_cell_name adv7611_init_0
  if { [catch {set adv7611_init_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $adv7611_init_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: i2c_master_0, and set properties
  set block_name i2c_master
  set block_cell_name i2c_master_0
  if { [catch {set i2c_master_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $i2c_master_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create interface connections
  connect_bd_intf_net -intf_net I_1 [get_bd_intf_pins ADV7611_CMD] [get_bd_intf_pins adv7611_init_0/I]

  # Create port connections
  connect_bd_net -net CLK_1 [get_bd_pins CLK] [get_bd_pins adv7611_init_0/CLK] [get_bd_pins i2c_master_0/CLK]
  connect_bd_net -net Net [get_bd_pins I2C_SDA] [get_bd_pins i2c_master_0/I2C_SDA]
  connect_bd_net -net Net1 [get_bd_pins I2C_SCL] [get_bd_pins i2c_master_0/I2C_SCL]
  connect_bd_net -net RESET_N_1 [get_bd_pins RESET_N] [get_bd_pins adv7611_init_0/RESET_N] [get_bd_pins i2c_master_0/RESET_N]
  connect_bd_net -net adv7611_init_0_I2CM_DEVADDR [get_bd_pins adv7611_init_0/I2CM_DEVADDR] [get_bd_pins i2c_master_0/DEVADDR]
  connect_bd_net -net adv7611_init_0_I2CM_RBYTES [get_bd_pins adv7611_init_0/I2CM_RBYTES] [get_bd_pins i2c_master_0/RBYTES]
  connect_bd_net -net adv7611_init_0_I2CM_REGADDR [get_bd_pins adv7611_init_0/I2CM_REGADDR] [get_bd_pins i2c_master_0/REGADDR]
  connect_bd_net -net adv7611_init_0_I2CM_VALID [get_bd_pins adv7611_init_0/I2CM_VALID] [get_bd_pins i2c_master_0/VALID]
  connect_bd_net -net adv7611_init_0_I2CM_WBYTES [get_bd_pins adv7611_init_0/I2CM_WBYTES] [get_bd_pins i2c_master_0/WBYTES]
  connect_bd_net -net adv7611_init_0_I2CM_WDATA [get_bd_pins adv7611_init_0/I2CM_WDATA] [get_bd_pins i2c_master_0/WDATA]
  connect_bd_net -net i2c_master_0_ERROR [get_bd_pins adv7611_init_0/I2CM_ERR] [get_bd_pins i2c_master_0/ERROR]
  connect_bd_net -net i2c_master_0_RDATA [get_bd_pins adv7611_init_0/I2CM_RDATA] [get_bd_pins i2c_master_0/RDATA]
  connect_bd_net -net i2c_master_0_READY [get_bd_pins adv7611_init_0/I2CM_READY] [get_bd_pins i2c_master_0/READY]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: fmc_hdmi
proc create_hier_cell_fmc_hdmi { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_msg_id "BD_TCL-102" "ERROR" "create_hier_cell_fmc_hdmi() - Empty argument(s)!"}
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_msg_id "BD_TCL-100" "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_msg_id "BD_TCL-101" "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 ETHERNET_SEND

  # Create pins
  create_bd_pin -dir I -type clk ETHERNET_CLK
  create_bd_pin -dir I -type rst ETHERNET_RESETN
  create_bd_pin -dir I HDMI1_AP
  create_bd_pin -dir I -type clk HDMI1_CLK
  create_bd_pin -dir I -from 0 -to 0 HDMI1_DE
  create_bd_pin -dir I -from 0 -to 0 HDMI1_HS
  create_bd_pin -dir I HDMI1_LRCLK
  create_bd_pin -dir I -from 23 -to 0 HDMI1_P
  create_bd_pin -dir I -type rst HDMI1_RESETN
  create_bd_pin -dir I HDMI1_SCLK
  create_bd_pin -dir I -from 0 -to 0 HDMI1_VS

  # Create instance: axis_audio_add_len_0, and set properties
  set block_name axis_audio_add_len
  set block_cell_name axis_audio_add_len_0
  if { [catch {set axis_audio_add_len_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $axis_audio_add_len_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: axis_data_fifo_0, and set properties
  set axis_data_fifo_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:1.1 axis_data_fifo_0 ]
  set_property -dict [ list \
   CONFIG.FIFO_MODE {2} \
   CONFIG.TDATA_NUM_BYTES {8} \
   CONFIG.TUSER_WIDTH {4} \
 ] $axis_data_fifo_0

  # Create instance: axis_data_fifo_1, and set properties
  set axis_data_fifo_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:1.1 axis_data_fifo_1 ]

  # Create instance: axis_dwidth_converter_0, and set properties
  set axis_dwidth_converter_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_dwidth_converter:1.1 axis_dwidth_converter_0 ]
  set_property -dict [ list \
   CONFIG.M_TDATA_NUM_BYTES {8} \
   CONFIG.S_TDATA_NUM_BYTES {4} \
 ] $axis_dwidth_converter_0

  # Create instance: axis_interconnect_0, and set properties
  set axis_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_interconnect:2.1 axis_interconnect_0 ]
  set_property -dict [ list \
   CONFIG.ARB_ON_MAX_XFERS {0} \
   CONFIG.ARB_ON_TLAST {1} \
   CONFIG.M00_FIFO_DEPTH {1024} \
   CONFIG.M00_FIFO_MODE {1} \
   CONFIG.NUM_MI {1} \
   CONFIG.NUM_SI {2} \
   CONFIG.S00_FIFO_DEPTH {1024} \
   CONFIG.S00_FIFO_MODE {1} \
   CONFIG.S01_FIFO_DEPTH {1024} \
   CONFIG.S01_FIFO_MODE {1} \
 ] $axis_interconnect_0

  # Create instance: const_zero, and set properties
  set const_zero [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 const_zero ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0} \
 ] $const_zero

  # Create instance: fifo_video_packet, and set properties
  set fifo_video_packet [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:1.1 fifo_video_packet ]
  set_property -dict [ list \
   CONFIG.FIFO_MODE {2} \
   CONFIG.TDATA_NUM_BYTES {8} \
   CONFIG.TUSER_WIDTH {4} \
 ] $fifo_video_packet

  # Create instance: hdmi_sig_count_0, and set properties
  set block_name hdmi_sig_count
  set block_cell_name hdmi_sig_count_0
  if { [catch {set hdmi_sig_count_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $hdmi_sig_count_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: i2s_axis_0, and set properties
  set block_name i2s_axis
  set block_cell_name i2s_axis_0
  if { [catch {set i2s_axis_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $i2s_axis_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: rgb_to_axis_0, and set properties
  set block_name rgb_to_axis
  set block_cell_name rgb_to_axis_0
  if { [catch {set rgb_to_axis_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_msg_id "BD_TCL-105" "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $rgb_to_axis_0 eq "" } {
     catch {common::send_msg_id "BD_TCL-106" "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins ETHERNET_SEND] [get_bd_intf_pins axis_data_fifo_0/M_AXIS]
  connect_bd_intf_net -intf_net S00_AXIS_1 [get_bd_intf_pins axis_interconnect_0/S00_AXIS] [get_bd_intf_pins fifo_video_packet/M_AXIS]
  connect_bd_intf_net -intf_net S01_AXIS_1 [get_bd_intf_pins axis_audio_add_len_0/O] [get_bd_intf_pins axis_interconnect_0/S01_AXIS]
  connect_bd_intf_net -intf_net axis_data_fifo_1_M_AXIS [get_bd_intf_pins axis_audio_add_len_0/I] [get_bd_intf_pins axis_data_fifo_1/M_AXIS]
  connect_bd_intf_net -intf_net axis_dwidth_converter_0_M_AXIS [get_bd_intf_pins axis_data_fifo_1/S_AXIS] [get_bd_intf_pins axis_dwidth_converter_0/M_AXIS]
  connect_bd_intf_net -intf_net axis_interconnect_0_M00_AXIS [get_bd_intf_pins axis_data_fifo_0/S_AXIS] [get_bd_intf_pins axis_interconnect_0/M00_AXIS]
  connect_bd_intf_net -intf_net i2s_axis_0_S [get_bd_intf_pins axis_dwidth_converter_0/S_AXIS] [get_bd_intf_pins i2s_axis_0/S]
  connect_bd_intf_net -intf_net rgb_to_axis_0_O [get_bd_intf_pins fifo_video_packet/S_AXIS] [get_bd_intf_pins rgb_to_axis_0/O]

  # Create port connections
  connect_bd_net -net HDMI1_AP_1 [get_bd_pins HDMI1_AP] [get_bd_pins i2s_axis_0/I2S_DAT]
  connect_bd_net -net HDMI1_DE_1 [get_bd_pins HDMI1_DE] [get_bd_pins hdmi_sig_count_0/DATA_EN] [get_bd_pins rgb_to_axis_0/DATA_EN]
  connect_bd_net -net HDMI1_HS_1 [get_bd_pins HDMI1_HS] [get_bd_pins hdmi_sig_count_0/HSYNC] [get_bd_pins rgb_to_axis_0/HSYNC]
  connect_bd_net -net HDMI1_LRCLK_1 [get_bd_pins HDMI1_LRCLK] [get_bd_pins i2s_axis_0/I2S_LR]
  connect_bd_net -net HDMI1_P_1 [get_bd_pins HDMI1_P] [get_bd_pins hdmi_sig_count_0/DATA] [get_bd_pins rgb_to_axis_0/DATA]
  connect_bd_net -net HDMI1_SCLK_1 [get_bd_pins HDMI1_SCLK] [get_bd_pins i2s_axis_0/I2S_BCLK]
  connect_bd_net -net HDMI1_VS_1 [get_bd_pins HDMI1_VS] [get_bd_pins hdmi_sig_count_0/VSYNC] [get_bd_pins rgb_to_axis_0/VSYNC]
  connect_bd_net -net clk_wiz_1_clk_out1 [get_bd_pins HDMI1_CLK] [get_bd_pins axis_interconnect_0/S00_AXIS_ACLK] [get_bd_pins fifo_video_packet/s_axis_aclk] [get_bd_pins hdmi_sig_count_0/CLK] [get_bd_pins rgb_to_axis_0/CLK]
  connect_bd_net -net clk_wiz_1_locked [get_bd_pins HDMI1_RESETN] [get_bd_pins axis_interconnect_0/S00_AXIS_ARESETN] [get_bd_pins fifo_video_packet/s_axis_aresetn] [get_bd_pins hdmi_sig_count_0/RESETN] [get_bd_pins rgb_to_axis_0/RESETN]
  connect_bd_net -net const_zero_dout [get_bd_pins axis_interconnect_0/S00_ARB_REQ_SUPPRESS] [get_bd_pins axis_interconnect_0/S01_ARB_REQ_SUPPRESS] [get_bd_pins const_zero/dout]
  connect_bd_net -net hdmi_sig_count_0_HCOUNT_SAVE [get_bd_pins hdmi_sig_count_0/HCOUNT_SAVE] [get_bd_pins rgb_to_axis_0/DISP_WIDTH]
  connect_bd_net -net hdmi_sig_count_0_INTERLACED [get_bd_pins hdmi_sig_count_0/INTERLACED] [get_bd_pins rgb_to_axis_0/INTERLACED]
  connect_bd_net -net hdmi_sig_count_0_ODD_FRAME [get_bd_pins hdmi_sig_count_0/ODD_FRAME] [get_bd_pins rgb_to_axis_0/ODD_FRAME]
  connect_bd_net -net hdmi_sig_count_0_VCOUNT_SAVE [get_bd_pins hdmi_sig_count_0/VCOUNT_SAVE] [get_bd_pins rgb_to_axis_0/DISP_HEIGHT]
  connect_bd_net -net m_axis_aclk_1 [get_bd_pins ETHERNET_CLK] [get_bd_pins axis_audio_add_len_0/CLK] [get_bd_pins axis_data_fifo_0/s_axis_aclk] [get_bd_pins axis_data_fifo_1/s_axis_aclk] [get_bd_pins axis_dwidth_converter_0/aclk] [get_bd_pins axis_interconnect_0/ACLK] [get_bd_pins axis_interconnect_0/M00_AXIS_ACLK] [get_bd_pins axis_interconnect_0/S01_AXIS_ACLK] [get_bd_pins i2s_axis_0/CLK]
  connect_bd_net -net m_axis_aresetn_1 [get_bd_pins ETHERNET_RESETN] [get_bd_pins axis_data_fifo_0/s_axis_aresetn] [get_bd_pins axis_data_fifo_1/s_axis_aresetn] [get_bd_pins axis_dwidth_converter_0/aresetn] [get_bd_pins axis_interconnect_0/ARESETN] [get_bd_pins axis_interconnect_0/M00_AXIS_ARESETN] [get_bd_pins axis_interconnect_0/S01_AXIS_ARESETN] [get_bd_pins i2s_axis_0/RESETN]

  # Restore current instance
  current_bd_instance $oldCurInst
}


# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  variable script_folder
  variable design_name

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_msg_id "BD_TCL-100" "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_msg_id "BD_TCL-101" "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj


  # Create interface ports
  set MGTCLK0 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 MGTCLK0 ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {156250000} \
   ] $MGTCLK0
  set default_sysclk_250 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 default_sysclk_250 ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {250000000} \
   ] $default_sysclk_250

  # Create ports
  set EXT_MCU_DOUT [ create_bd_port -dir O -from 4 -to 0 EXT_MCU_DOUT ]
  set EXT_MCU_SCL [ create_bd_port -dir IO EXT_MCU_SCL ]
  set EXT_MCU_SCLK [ create_bd_port -dir I -from 4 -to 0 EXT_MCU_SCLK ]
  set EXT_MCU_SDA [ create_bd_port -dir IO EXT_MCU_SDA ]
  set HDMI1_AP [ create_bd_port -dir I HDMI1_AP ]
  set HDMI1_DE [ create_bd_port -dir I HDMI1_DE ]
  set HDMI1_HS [ create_bd_port -dir I HDMI1_HS ]
  set HDMI1_LLC [ create_bd_port -dir I -type clk HDMI1_LLC ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {162500000} \
 ] $HDMI1_LLC
  set HDMI1_LRCLK [ create_bd_port -dir I HDMI1_LRCLK ]
  set HDMI1_P [ create_bd_port -dir I -from 23 -to 0 HDMI1_P ]
  set HDMI1_SCL [ create_bd_port -dir IO HDMI1_SCL ]
  set HDMI1_SCLK [ create_bd_port -dir I HDMI1_SCLK ]
  set HDMI1_SDA [ create_bd_port -dir IO HDMI1_SDA ]
  set HDMI1_VS [ create_bd_port -dir I HDMI1_VS ]
  set IR_OUT [ create_bd_port -dir O IR_OUT ]
  set LED [ create_bd_port -dir O -from 2 -to 0 LED ]
  set SFP1_RX_LOS [ create_bd_port -dir I SFP1_RX_LOS ]
  set SFP1_RX_N [ create_bd_port -dir I SFP1_RX_N ]
  set SFP1_RX_P [ create_bd_port -dir I SFP1_RX_P ]
  set SFP1_TX_DISABLE [ create_bd_port -dir O SFP1_TX_DISABLE ]
  set SFP1_TX_N [ create_bd_port -dir O SFP1_TX_N ]
  set SFP1_TX_P [ create_bd_port -dir O SFP1_TX_P ]
  set reset [ create_bd_port -dir I -type rst reset ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_HIGH} \
 ] $reset

  # Create instance: clk_wiz_hdmi, and set properties
  set clk_wiz_hdmi [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:6.0 clk_wiz_hdmi ]
  set_property -dict [ list \
   CONFIG.CLKOUT1_JITTER {105.122} \
   CONFIG.CLKOUT1_PHASE_ERROR {91.680} \
   CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {162.500} \
   CONFIG.CLKOUT1_REQUESTED_PHASE {180.000} \
   CONFIG.CLK_OUT1_PORT {clk_hdmi1} \
   CONFIG.MMCM_CLKFBOUT_MULT_F {40.625} \
   CONFIG.MMCM_CLKIN2_PERIOD {10.000} \
   CONFIG.MMCM_CLKOUT0_DIVIDE_F {6.250} \
   CONFIG.MMCM_CLKOUT0_PHASE {180.000} \
   CONFIG.MMCM_DIVCLK_DIVIDE {4} \
 ] $clk_wiz_hdmi

  # Create instance: clk_wiz_sys, and set properties
  set clk_wiz_sys [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:6.0 clk_wiz_sys ]
  set_property -dict [ list \
   CONFIG.CLKOUT1_JITTER {107.111} \
   CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {100.000} \
   CONFIG.CLKOUT2_JITTER {107.111} \
   CONFIG.CLKOUT2_PHASE_ERROR {85.928} \
   CONFIG.CLKOUT2_USED {false} \
   CONFIG.CLK_IN1_BOARD_INTERFACE {default_sysclk_250} \
   CONFIG.CLK_OUT1_PORT {clk_100} \
   CONFIG.CLK_OUT2_PORT {clk_out2} \
   CONFIG.MMCM_CLKOUT0_DIVIDE_F {10.000} \
   CONFIG.MMCM_CLKOUT1_DIVIDE {1} \
   CONFIG.MMCM_DIVCLK_DIVIDE {1} \
   CONFIG.NUM_OUT_CLKS {1} \
   CONFIG.RESET_BOARD_INTERFACE {reset} \
   CONFIG.USE_BOARD_FLOW {true} \
 ] $clk_wiz_sys

  # Create instance: fmc_hdmi
  create_hier_cell_fmc_hdmi [current_bd_instance .] fmc_hdmi

  # Create instance: fmc_hdmi_adv7611_init
  create_hier_cell_fmc_hdmi_adv7611_init [current_bd_instance .] fmc_hdmi_adv7611_init

  # Create instance: proc_sys_reset_100, and set properties
  set proc_sys_reset_100 [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_100 ]

  # Create instance: proc_sys_reset_hdmi1, and set properties
  set proc_sys_reset_hdmi1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_hdmi1 ]

  # Create instance: udp_server_10g
  create_hier_cell_udp_server_10g [current_bd_instance .] udp_server_10g

  # Create instance: vcnet_control
  create_hier_cell_vcnet_control [current_bd_instance .] vcnet_control

  # Create instance: xlconcat_0, and set properties
  set xlconcat_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0 ]

  # Create interface connections
  connect_bd_intf_net -intf_net ETHER_SEND_1 [get_bd_intf_pins fmc_hdmi/ETHERNET_SEND] [get_bd_intf_pins udp_server_10g/ETHERNET_SEND]
  connect_bd_intf_net -intf_net MGTCLK0_1 [get_bd_intf_ports MGTCLK0] [get_bd_intf_pins udp_server_10g/MGTCLK0]
  connect_bd_intf_net -intf_net default_sysclk_250_1 [get_bd_intf_ports default_sysclk_250] [get_bd_intf_pins clk_wiz_sys/CLK_IN1_D]
  connect_bd_intf_net -intf_net udp_server_10g_UDP_RECV [get_bd_intf_pins udp_server_10g/UDP_RECV] [get_bd_intf_pins vcnet_control/UDP_RECV]
  connect_bd_intf_net -intf_net vcnet_control_ADV7611 [get_bd_intf_pins fmc_hdmi_adv7611_init/ADV7611_CMD] [get_bd_intf_pins vcnet_control/ADV7611_CMD]

  # Create port connections
  connect_bd_net -net CLK100_RESETN_1 [get_bd_pins fmc_hdmi_adv7611_init/RESET_N] [get_bd_pins proc_sys_reset_100/peripheral_aresetn] [get_bd_pins vcnet_control/CLK100_RESETN]
  connect_bd_net -net EXT_MCU_SCLK_1 [get_bd_ports EXT_MCU_SCLK] [get_bd_pins vcnet_control/EXT_MCU_SCLK]
  connect_bd_net -net HDMI1_AP_1 [get_bd_ports HDMI1_AP] [get_bd_pins fmc_hdmi/HDMI1_AP]
  connect_bd_net -net HDMI1_CLK_1 [get_bd_pins clk_wiz_hdmi/clk_hdmi1] [get_bd_pins fmc_hdmi/HDMI1_CLK] [get_bd_pins proc_sys_reset_hdmi1/slowest_sync_clk]
  connect_bd_net -net HDMI1_DE_1 [get_bd_ports HDMI1_DE] [get_bd_pins fmc_hdmi/HDMI1_DE]
  connect_bd_net -net HDMI1_HS_1 [get_bd_ports HDMI1_HS] [get_bd_pins fmc_hdmi/HDMI1_HS]
  connect_bd_net -net HDMI1_LLC_1 [get_bd_ports HDMI1_LLC] [get_bd_pins clk_wiz_hdmi/clk_in1]
  connect_bd_net -net HDMI1_LRCLK_1 [get_bd_ports HDMI1_LRCLK] [get_bd_pins fmc_hdmi/HDMI1_LRCLK]
  connect_bd_net -net HDMI1_P_1 [get_bd_ports HDMI1_P] [get_bd_pins fmc_hdmi/HDMI1_P]
  connect_bd_net -net HDMI1_SCLK_1 [get_bd_ports HDMI1_SCLK] [get_bd_pins fmc_hdmi/HDMI1_SCLK]
  connect_bd_net -net HDMI1_VS_1 [get_bd_ports HDMI1_VS] [get_bd_pins fmc_hdmi/HDMI1_VS]
  connect_bd_net -net HDMI_RESETN_1 [get_bd_pins fmc_hdmi/HDMI1_RESETN] [get_bd_pins proc_sys_reset_hdmi1/peripheral_aresetn]
  connect_bd_net -net Net [get_bd_ports HDMI1_SDA] [get_bd_pins fmc_hdmi_adv7611_init/I2C_SDA]
  connect_bd_net -net Net1 [get_bd_ports HDMI1_SCL] [get_bd_pins fmc_hdmi_adv7611_init/I2C_SCL]
  connect_bd_net -net Net2 [get_bd_ports EXT_MCU_SCL] [get_bd_pins vcnet_control/EXT_MCU_SCL]
  connect_bd_net -net Net3 [get_bd_ports EXT_MCU_SDA] [get_bd_pins vcnet_control/EXT_MCU_SDA]
  connect_bd_net -net SFP1_RX_LOS_1 [get_bd_ports SFP1_RX_LOS] [get_bd_pins udp_server_10g/SFP1_RX_LOS]
  connect_bd_net -net SFP1_RX_N_1 [get_bd_ports SFP1_RX_N] [get_bd_pins udp_server_10g/SFP1_RX_N]
  connect_bd_net -net SFP1_RX_P_1 [get_bd_ports SFP1_RX_P] [get_bd_pins udp_server_10g/SFP1_RX_P]
  connect_bd_net -net clk_wiz_1_clk_100 [get_bd_pins clk_wiz_sys/clk_100] [get_bd_pins fmc_hdmi_adv7611_init/CLK] [get_bd_pins proc_sys_reset_100/slowest_sync_clk] [get_bd_pins udp_server_10g/CLK100] [get_bd_pins vcnet_control/CLK100]
  connect_bd_net -net clk_wiz_hdmi_locked [get_bd_pins clk_wiz_hdmi/locked] [get_bd_pins proc_sys_reset_hdmi1/dcm_locked]
  connect_bd_net -net clk_wiz_sys_locked [get_bd_pins clk_wiz_sys/locked] [get_bd_pins proc_sys_reset_100/dcm_locked]
  connect_bd_net -net reset_1 [get_bd_ports reset] [get_bd_pins clk_wiz_hdmi/reset] [get_bd_pins clk_wiz_sys/reset] [get_bd_pins proc_sys_reset_100/ext_reset_in] [get_bd_pins proc_sys_reset_hdmi1/ext_reset_in] [get_bd_pins udp_server_10g/CLK100_RESET]
  connect_bd_net -net udp_server_10g_LED [get_bd_pins udp_server_10g/LED] [get_bd_pins xlconcat_0/In0]
  connect_bd_net -net udp_server_10g_SFP1_TX_DISABLE [get_bd_ports SFP1_TX_DISABLE] [get_bd_pins udp_server_10g/SFP1_TX_DISABLE]
  connect_bd_net -net udp_server_10g_SFP1_TX_N [get_bd_ports SFP1_TX_N] [get_bd_pins udp_server_10g/SFP1_TX_N]
  connect_bd_net -net udp_server_10g_SFP1_TX_P [get_bd_ports SFP1_TX_P] [get_bd_pins udp_server_10g/SFP1_TX_P]
  connect_bd_net -net udp_server_10g_areset_datapathclk_out [get_bd_pins fmc_hdmi/ETHERNET_RESETN] [get_bd_pins udp_server_10g/ETHERNET_RESETN] [get_bd_pins vcnet_control/ETHERNET_RESETN]
  connect_bd_net -net udp_server_10g_txusrclk2_out [get_bd_pins fmc_hdmi/ETHERNET_CLK] [get_bd_pins udp_server_10g/ETHERNET_CLK] [get_bd_pins vcnet_control/ETERHENT_CLK]
  connect_bd_net -net vcnet_control_DOUT_0 [get_bd_ports EXT_MCU_DOUT] [get_bd_pins vcnet_control/EXT_MCU_DOUT]
  connect_bd_net -net vcnet_control_IR_OUT_0 [get_bd_ports IR_OUT] [get_bd_pins vcnet_control/IR_OUT]
  connect_bd_net -net vcnet_control_LED [get_bd_pins vcnet_control/LED] [get_bd_pins xlconcat_0/In1]
  connect_bd_net -net xlconcat_0_dout [get_bd_ports LED] [get_bd_pins xlconcat_0/dout]

  # Create address segments


  # Restore current instance
  current_bd_instance $oldCurInst

  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


common::send_msg_id "BD_TCL-1000" "WARNING" "This Tcl script was generated from a block design that has not been validated. It is possible that design <$design_name> may result in errors during validation."

