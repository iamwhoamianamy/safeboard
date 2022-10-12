#!/bin/bash

dir=/home/arthur/many_dirs

rm -r $dir
mkdir $dir
./file_generator 1000 $dir/many_files
touch $dir/true_report.txt
./file_contaminator $dir/many_files > $dir/true_report.txt