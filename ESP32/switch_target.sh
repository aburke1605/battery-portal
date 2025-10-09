#!/bin/bash

# check inputs
if [[ $# -ne 1 || ( $1 != "linux" && $1 != "esp32" ) ]]; then
    echo "Usage: $0 <target>"
    echo "<target> must be \"linux\" or \"esp32\""
    exit 1
fi


SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

rm -r $SCRIPT_DIR/build/

if [ $1 == "linux" ]; then
    cp $SCRIPT_DIR/main/idf_component.yml_linux $SCRIPT_DIR/main/idf_component.yml
    idf.py --preview set-target linux
    cp $SCRIPT_DIR/sdkconfig_linux $SCRIPT_DIR/sdkconfig
fi

if [ $1 == "esp32" ]; then
    cp $SCRIPT_DIR/main/idf_component.yml_esp32 $SCRIPT_DIR/main/idf_component.yml
    idf.py set-target esp32
    cp $SCRIPT_DIR/sdkconfig_esp32 $SCRIPT_DIR/sdkconfig
fi