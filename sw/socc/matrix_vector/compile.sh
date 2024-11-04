#! /bin/bash -e

declare -a test_folders=("matvec_mul_1_1" 
                        "matvec_mul_1_256" 
                        "matvec_mul_16_32"
                        "matvec_mul_32_16"
                        "matvec_mul_32_128"
                        "matvec_mul_64_64"
                        "matvec_mul_128_128"
                        "matvec_mul_256_128"
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