# A Kernel Seedling
The purpose of this lab is to allow students to set up their virtual machine to create their first kernel module. The lab specifically focuses on the implementation of the proc_count function in the proc_count.c file. This function prints the number of processes currently running onto the terminal. Some core topics of the lab include: kernel modules and process management.

## Building
```shell
Command for Build:
make
Explanation: This command executes the instruction within the Makefile to build the project accordingly.
```

## Running
```shell
Command for Running Binary:
sudo insmod proc_count.ko
cat /proc/count
Explanation: The first command loads the binary file 'proc_count.ko' that was created into the Linux kernel which interacts with the /proc filesystem to create the /proc/count virtual file. The second command will then display the contents of /proc/count, which I coded to output the number of processes that are currently running.

```
Result: Terminal outputs the number of processes currently running. I typically got an output of 137 or 136.

## Cleaning Up
```shell
Command for Cleaning the Built Binary:
make clean
Explanation: This command will remove all the files that were generated during the building of the project. In this case, it would include files such as proc_count.ko which were generated after the make command was sent to the terminal.

```

## Testing
```python
python -m unittest
```
Result: Terminal outputs whether or not the test cases fail or succeed. In my VM, I got the following (excluding separator lines):
Ran 3 tests in 7.859s

OK

Report which kernel release version you tested your module on
(hint: use `uname`, check for options with `man uname`).
It should match release numbers as seen on https://www.kernel.org/.

```shell
uname -r -s -v
```
Kernel Version: 5.14.8-arch-1
