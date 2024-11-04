#! /bin/bash -e

declare -a test_folders=(
                        "vadd_128" 
                        "vadd_256" 
                        "vadd_512" 
                        "vadd_654" 
                        "vadd_756" 
                        "vadd_1024" 
                        "vadd_1280"
                        "vadd_1536"
                        "vmul_128" 
                        "vmul_256" 
                        "vmul_512" 
                        "vmul_654" 
                        "vmul_756" 
                        "vmul_1024" 
                        "vmul_1280"
                        "vmul_1536"
            )

## now loop through the above array
for i in "${test_folders[@]}"
do
    cd $i
    make clean
    make all
    cd ..
done