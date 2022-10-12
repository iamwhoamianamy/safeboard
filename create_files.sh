#!/bin/bash

dir=/home/arthur/many_dirs

rm -r $dir
mkdir $dir

for i in 1 2
do
   ./file_generator 5000 $dir/many_files_$i
   ./file_contaminator $dir/many_files_$i > $dir/true_report_$i.txt
done