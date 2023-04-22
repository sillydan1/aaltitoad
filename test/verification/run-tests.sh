#!/bin/sh

for test_folder in "$@"
do
    echo "$test_folder"
    echo "$test_folder" >> result.txt
    ~/Downloads/aaltitoad/Debug/aaltitoad -i $test_folder -q $test_folder/Queries.json -v 4 >> result.txt
    echo "done"
done

