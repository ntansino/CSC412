#!/bin/bash

# Error Handling
if [[ $# != 2 ]]
	then
		echo usage:$'\t'$0 "<path>" "<extension>"

else

	# navigate to directory
	command cd $1

	echo Looking for files with extension .$2 in folder
	echo $'\t'$1:

	
	# create array and count (length of array)
	files=()
	count=0

	# assess every file in given directory
	for fname in *
	do
		# add desired files to array
		if [[ $fname == *.$2 ]]
			then
				files+=( $fname )
				((count++))
		fi
	done

	# no files found
	if [[ $count == 0 ]]
		then
			echo No file found.

	# one file found
	elif [[ $count == 1 ]]
		then
			echo 1 file found:
			echo $'\t'${files[0]}

	# multiple files found
	else
		
		echo $count files found:

		# print all files in array
		for fname in ${files[@]}
		do
			echo $'\t'$fname
		done
	fi
fi