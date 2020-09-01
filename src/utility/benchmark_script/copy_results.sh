#!/bin/bash

result_dir=$1
target_dir=$2

mv $result_dir/*.csv $target_dir
mv $result_dir/*.dmp $target_dir
