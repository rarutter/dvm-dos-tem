#!/bin/bash

#parameters: node_x, node_y, input_dir

#Dimensions of the rectangle sent to each node (10?)
node_x=$1
node_y=$2

#Directory of input files
# e.g. input_dir=/home/vagrant/dvm-dos-tem/DATA/Toolik_10x10_allyrs
input_dir=$3

#Dimensions of the input files are read from run-mask.nc 
# As recommended at http://nco.sourceforge.net/nco.html#ncdmnsz
input_x="$(ncks -m ${input_dir}/run-mask.nc | grep -E 'run dimension 1' | cut -f 7 -d ' ')"
input_y="$(ncks -m ${input_dir}/run-mask.nc | grep -E 'run dimension 0' | cut -f 7 -d ' ')"


#Number of node chunks in each dimension (x and y, not NetCDF dimensions)
chunks_x=$(($input_x/$node_x))
x_remainder=$(($input_x%$node_x)) #Possibly unneeded

chunks_y=$(($input_y/$node_y))
y_remainder=$(($input_y%$node_y)) #Possibly unneeded


#Sliced data is currently isolated in a subdirectory for easy deletion
# during testing. Check for the directory, remove if needed, then create.
if [ -d "sliced_input" ]; then
  rm -r sliced_input
fi
mkdir sliced_input
cd sliced_input

#Check if the input directory path is relative or absolute
if [[ "$input_dir" != /* ]]; then #is relative
  echo "relative path, modifying to adjust for directory change"
  #prepend '../' to balance the change in directory
  prefix="../"
  input_dir=$prefix$input_dir
fi

#Cycle through x,y
for (( curr_y=0; curr_y<$input_y; curr_y+=$node_y )); do
  for (( curr_x=0; curr_x<$input_x; curr_x+=$node_x )); do

    #Check for directory, if none then create one for this
    # specific node chunk
    echo "${curr_x}, ${curr_y}"
    if [ -d "${curr_x}_${curr_y}" ]; then
      rm -r "${curr_x}_${curr_y}"
    fi
    mkdir "${curr_x}_${curr_y}"

    #Calculate the limits of the hyperslab for ncks
    x_slice_end=$(($curr_x+$node_x-1))
    y_slice_end=$(($curr_y+$node_y-1))

    #The input files are not required to be an even multiple of the node
    # chunk dimensions. The following ensures that ncks is not called with
    # an invalid dimension limit.
    if (( $x_slice_end >= $input_x )); then
      x_slice_end=$(($input_x-1))
      echo "x slice end = ${x_slice_end}"
    fi
    if (( $y_slice_end >= $input_y )); then
      y_slice_end=$(($input_y-1))
      echo "y slice end = ${y_slice_end}"
    fi

    #Slice each input file
    for file in $input_dir/*.nc; do

      #remove path from input filename
      filename=${file##*/}
      echo "Slicing ${filename}, chunk ${curr_x},${curr_y}"

      ncks -d X,$curr_x,$x_slice_end -d Y,$curr_y,$y_slice_end $file "${curr_x}_${curr_y}/${filename}"

    done #for each file

    #tarball for transfer?
    tar -czf "${curr_x}_${curr_y}.tar.gz" "./${curr_x}_${curr_y}/" 

  done #x dimension
done #y dimension



