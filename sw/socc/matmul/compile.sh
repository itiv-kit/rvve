#! /bin/bash

declare -a test_folders=("matmul_1_1_1" 
                        "matmul_1_64_64"
                        "matmul_1_256_256"
                        "matmul_64_64_64"
                        "matmul_72_128_128"
                        "matmul_128_128_128"
                        "matmul_256_128_16"
                        "matmul_256_256_256"
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