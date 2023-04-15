#!/bin/sh

for test_folder in "$@"
do
    echo "$test_folder"
    echo "$test_folder" >> result.txt
    ~/Git/private/AALTITOAD/Debug/verifier -f $test_folder -q $test_folder/Queries.json -i ".*ignore.*" -m -v 4 -t /dev/null >> result.txt
    echo "done"
done

