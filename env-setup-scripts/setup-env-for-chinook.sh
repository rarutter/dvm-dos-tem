#!/bin/bash


export PATH="$HOME/custom_software/openmpi-4.1.5/bin:$PATH"

export LD_LIBRARY_PATH="$HOME/custom_software/openmpi-4.1.5/lib:$HOME/custom_software/boost_1_75_0/lib:$HOME/custom_software/jsoncpp/libs/linux-gcc-5.4.0:$HOME/custom_software/hdf5-1.8.19/hdf5/lib:$HOME/custom_software/netcdf-4.4.1.1/netcdf/lib:$HOME/custom_software/lapack-3.8.0:$LD_LIBRARY_PATH"

#export CPATH="$HOME/custom_software/jsoncpp/include:$HOME/custom_software/boost_1_75_0/include:$HOME/custom_software/netcdf-4.4.1.1/netcdf/include:$CPATH"

echo "NOTE: This file will NOT work if it is run as a script!"
echo "      Instead use the 'source' command like this:"
echo "      $ source env-setup-scripts/setup-env-for-atlas.sh"
echo ""


