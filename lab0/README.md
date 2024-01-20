# A Kernel Seedling
TODO: intro

## Building
```shell
cmd for build:
make
```

## Running
```shell
cmd for running binary:
sudo insmod proc_count.ko
cat /proc/count

```
Result: Terminal outputs the number of processes currently running.

## Cleaning Up
```shell
cmd for cleaning the built binary:
make clean
```

## Testing
```python
python -m unittest
```
Result: Terminal outputs whether or not the test cases fail or succeed. If it succeeds, it outputs the following (excluding some buffer lines):

Ran 3 tests in 7.859s

OK

Report which kernel release version you tested your module on
(hint: use `uname`, check for options with `man uname`).
It should match release numbers as seen on https://www.kernel.org/.

```shell
uname -r -s -v
```
Kernel Version: 5.14.8-arch-1
