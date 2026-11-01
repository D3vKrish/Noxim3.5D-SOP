#!/bin/bash

CURR_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

export SYSTEMC_HOME=$CURR_DIR/systemc/2.3.4/
export YAML_HOME=$CURR_DIR/yaml-cpp/
SYSTEMC_LIB=$SYSTEMC_HOME/lib64/
YAML_LIB=$YAML_HOME/lib/
export LD_LIBRARY_PATH=$SYSTEMC_LIB:$LD_LIBRARY_PATH
export LIBRARY_PATH=$SYSTEMC_LIB:$YAML_LIB:$LIBRARY_PATH

