#=============================================================================
# run-opt.tcl 
#=============================================================================
# @brief: A Tcl script for the optimization experiments from lab 1 section 4.3
#
# @desc: Fix the design configuration to use 20 iterations and 32-bit signed
# fixed-point type with 8 integer bits. The goal is to maximize the throughput
# of this design using optimization directives provided by Vivado HLS. 
#

# Your Vivado HLS project script goes here
# HINT: You can use run_fixed.tcl as a template. Then, run this script with
# `vivado_hls -f run_opt.tcl`

#------------------------------------------------------
# Set result filename and clean old data
#------------------------------------------------------
set filename "fixed_result_opt.csv"
file delete -force "./result/${filename}"

#-----------------------------------------------------
# You can specify a set of bitwidth configurations to 
# explore in the batch experiments. 
# Each configuration (line) is defined by a pair in  
# total bitwidth and integer bitwidth
# Examples are shown below. 
#-----------------------------------------------------
set bitwidth_pair { \
    32    8   \
}
#-----------------------------------
# run batch experiments
#-----------------------------------
foreach { TOT_W INT_W } $bitwidth_pair {

# Define the bitwidth macros from CFLAGs
set CFLAGS "-DFIXED_TYPE -DTOT_WIDTH=${TOT_W} -DINT_WIDTH=${INT_W}"

# Project name
set hls_prj "fixed_${TOT_W}_${INT_W}_20.prj"

# Open/reset the project
open_project ${hls_prj} -reset
# Top function of the design is "cordic"
set_top cordic

# Add design and testbench files
add_files cordic.cpp -cflags $CFLAGS
add_files -tb cordic_test.cpp -cflags $CFLAGS

open_solution "solution1"
# Use Zynq device
set_part {xc7z020clg484-1}

#set_directive_pipeline cordic
#pragma HLS pipeline

set_directive_unroll cordic/FIXED_STEP_LOOP
#pragma HLS unroll

# Target clock period is 10ns
create_clock -period 10


# Simulate the C++ design
csim_design
# Synthesize the design
csynth_design



# We will skip C-RTL cosimulation for now
#cosim_design

#---------------------------------------------
# Collect & dump out results from HLS reports
#---------------------------------------------
set argv [list $filename $hls_prj]
set argc 2
source "./script/collect_result.tcl"
}

quit
