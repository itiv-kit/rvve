#! /bin/bash

declare -a test_folders=("conv_1_1_1_1_1_1_1_0_1" 
                        "conv_1_8_8_64_4_4_32_0_1"
                        "conv_1_8_8_128_4_4_32_0_1"
                        "conv_1_26_26_32_3_3_64_0_1"
                        "conv_1_28_28_1_3_3_32_0_1"
                        "conv_1_56_56_64_2_2_16_1_1"
                        "conv_2_28_28_3_3_3_64_1_1"
                        "conv_2_56_56_16_3_3_32_1_2"
            )

## now loop through the above array
for i in "${test_folders[@]}"
do
    cd $i
    echo "$i"
    make clean
    make all
    cd ..
done