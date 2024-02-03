## UID: 705993595

## Pipe Up

A custom program that aims to replicate the Unix shell pipe (|) mechanism, allowing users to chain commands where the output of one command serves as the input to the next.

## Building

To build my program, use the GCC compiler in your terminal. Navigate to the directory containing pipe.c, then compile the program with the following command:

gcc pipe.c -o pipe

This will create an executable command called "pipe" in which you can call by entering "./pipe [arguments]" into the terminal.

## Running

./pipe ls sort uniq

What to expect:
ls --> lists the contents of the current directory
sort --> sorts the list alphabetically
uniq --> filters out duplicate entries in the list

I will expect, based on the current directory, to get the following output:
Makefile
pipe
pipe.c
README.md
test_lab1.py

I got these results when I ran the program. I also double checked by using the shell pipe command that also produced the same results.

## Cleaning up

To clean up all binary files, simply just type the following command into the terminal:

rm pipe

This will remove the pipe binary file; this is the only binary file produced during the lab. However, a more general command for removing all binary files is:

make clean
