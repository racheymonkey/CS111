# You Spin Me Round Robin

This lab focused on the implenentation of a Round Robin scheduling algorithm, utilizing the 'sys/queue.h' library and its 'TAILQ' macros for efficient process queue management. This task required a detailed setup of process scheduling to ensure equal CPU time distribution givan an input of a predefined quantum length. In this lab, I focused on managing process arrival, execution, and completion times through the maniplation of a doubly linked list.

## Building

```shell
In order to compile the rr.c source file and build the executable,
1) Navigate to the directory containing the rr.c file
2) Paste and enter the following command in the terminal:
'gcc rr.c -o rr'
This command will compile the rr.c source code into an executable named 'rr'.
```

## Running

cmd for running the Round Robin Simulation
```shell
In order to run the Round Robin simulation,

Paste and enter the following command in the terminal:
'./rr [path to text file containing processes information] [quantum length]'

In our lab, the text file containing process information was given to us in the same directory as the simulation source code. Thus, I inputted the following command into the terminal to run the simulation:
'./rr processes.txt 3'
This command will run the scheduler with a quantum length of 3 times unit.
```

results TODO
```shell
The results of the scheduler would generally follow this format:
'Average waiting time: [float number]
Average response time: [float number]'

If the command './rr processes.txt 3' is inputted into the terminal, I get the following result:
'Average waiting time: 7.00
Average response time: 2.75'
```

## Cleaning up

```shell
In order to clean and remove the binary files created during the build process, paste and enter the following command in the terminal:
'make clean'
```
