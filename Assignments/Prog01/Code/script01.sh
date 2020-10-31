#!/bin/bash

# compile C program w/ new executable name
command gcc -Wall prog01.c -o $1

# run new executable 
command ./$1						#FAIL (UNDER)
command ./$1 one					#FAIL (UNDER)
command ./$1 one two				#PASS (EQUAL)
command ./$1 one two three			#FAIL (OVER)
command ./$1 one two three four		#FAIL (OVER)