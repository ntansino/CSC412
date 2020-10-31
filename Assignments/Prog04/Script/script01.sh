#!/bin/bash

# Error Handling
if [[ $# != 2 ]]
	then
		echo usage:$'\t'$0 "<data_path>" "<out_path>"

else

	# navigate to given directory
	command cd $1

	# create array of indices
	indices=()
	numFiles=0

	# assess every file in given directory
	for fname in *
	do
		echo "$fname"
		# access line in txt file
		while read fname;
		do
			echo "$fname"
			# access first character of txt file
			indices+=(${line:0:1})
			((numFiles++))
		done
	done

	# print all indicies in array
	for index in ${indices[@]}
	do
		echo $index
	done

	echo "Number of Files = "$numFiles
	
fi