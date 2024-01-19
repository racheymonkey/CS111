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
TODO: results?

Report which kernel release version you tested your module on
(hint: use `uname`, check for options with `man uname`).
It should match release numbers as seen on https://www.kernel.org/.

```shell
uname -r -s -v
```
TODO: kernel ver?
