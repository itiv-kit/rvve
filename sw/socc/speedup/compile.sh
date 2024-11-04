#! /bin/bash -e

declare -a test_folders=("load" 
                        "mac"
                        "inner_product"
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
