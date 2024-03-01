Kevin Yuan, UID: 905933567

# Hash Hash Hash
This program will effectively implement a hash table with multithreading by utilizing mutexes to avoid race conditions.

## Building
To build the program and make the executable, run:
```shell
make
```

## Running
The program can be ran with the flags -t and/or -s. -t specifies the number of threads to use, and -s specifies the number of hash table entries to add per thread. For example, you can run:

```shell
$ ./hash-table tester -t 8 -s 50000
Generation: 69,315 usec
Hash table base: 1,498,348 usec
 - 0 missing
Hash table v1: 1,953,103 usec
 - 0 missing
Hash table v2: 403,573 usec
 - 0 missing
```

The program can also be ran without the flags -t and/or -s. The default number of threads is 4, and the default number of hash table entries per thread is 25,000. For example, you can run:
```shell
$ ./hash-table-tester
Generation: 17,711 usec
Hash table base: 49,904 usec
  - 0 missing
Hash table v1: 142,234 usec
  - 0 missing
Hash table v2: 21,599 usec
  - 0 missing
```

## First Implementation
In the first implementation, I declared a single `mutex` for the entire hash table before the `hash_table_v1_create()` function as a static global variable. I blocked all the code in the `hash_table_v1_add_entry()` function as a critical section, locking the mutex at the start of the function and unlocking it at the end of the function. This makes it so that while a thread is modifying the hash table, no other thread can tamper with it, thus preventing race conditions. I then destroyed it in the `hash_table_v1_destroy()` function to properly handle mutex cleanup. This guarantees correctness because every thread that adds an entry makes sure the entire has table is locked in the process of adding it so that no other thread can tamper with it or add the same entry.

### Performance
Test 1: v1 ran with just one thread.
```shell
$ ./hash-table-tester -t 1 -s 200000
Generation: 34,906 usec
Hash table base: 220,470 usec
  - 0 missing
Hash table v1: 241,418 usec
  - 0 missing
Hash table v2: 374,228 usec
  - 0 missing
```

Test 2: v1 ran with two threads.
```shell
$ ./hash-table-tester -t 2 -s 200000
Generation: 79,501 usec
Hash table base: 1,176,749 usec
  - 0 missing
Hash table v1: 1,867,935 usec
  - 0 missing
Hash table v2: 685,390 usec
  - 0 missing
```

Test 3: v1 ran with four threads.
```shell
$ ./hash-table-tester -t 4 -s 200000
Generation: 151,980 usec
Hash table base: 7,649,595 usec
  - 0 missing
Hash table v1: 10,530,915 usec
  - 0 missing
Hash table v2: 2,406,466 usec
  - 0 missing
```

Version 1 is slower than the base version. Locks make it so that only one thread executes a critical section at a time. In this case, the lock is used on the entire hash table (coarse-grained lock), which means all other threads must wait for the current thread to finish and unlock the mutex before executing the critical section. Only one thread operates on the hash table at a time. This raises the thread-related overhead as compared to the base version, which does not employ any mutexes. As a result, Version 1 is slower than the base version.

## Second Implementation
In the second implementation, I created a mutex for each entry in the hash table. I declared it in the `struct hash_table_entry`, and I initialized each one within the for loop in the `hash_table_v2_create()` function. I blocked the line in `hash_table_v2_add_entry()` that inserts a head as a critical section. This means the mutex for that entry is locked right before the head is inserted, and it is unlocked right after the head is inserted. This ensures that multiple threads do not try modifying the same index at the exact same time, which could cause a race condition.

### Performance
Test 1: v2 ran with just one thread.
```shell
$ ./hash-table-tester -t 1 -s 200000
Generation: 34,906 usec
Hash table base: 220,470 usec
  - 0 missing
Hash table v1: 241,418 usec
  - 0 missing
Hash table v2: 374,228 usec
  - 0 missing
```

Test 2: v2 ran with two threads.
```shell
$ ./hash-table-tester -t 2 -s 200000
Generation: 79,501 usec
Hash table base: 1,176,749 usec
  - 0 missing
Hash table v1: 1,867,935 usec
  - 0 missing
Hash table v2: 685,390 usec
  - 0 missing
```

Test 3: v2 ran with four threads.
```shell
$ ./hash-table-tester -t 4 -s 200000
Generation: 151,980 usec
Hash table base: 7,649,595 usec
  - 0 missing
Hash table v1: 10,530,915 usec
  - 0 missing
Hash table v2: 2,406,466 usec
  - 0 missing
```

Analysis:
- In Test 1, there is no multithreading because there is only one thread, so version 2 intuitively takes longer because of the thread overhead caused by the mutexes on each index.
- In Test 2, there are two cores. v2 takes 685,390 usec, and base takes 1,176,749 usec, so v2 = base / 1.717, and this satisfies v2 <= base / (num_cores - 1) = base / (2 - 1) = base.
- In Test 3, there are four cores. v2 takes 2,406,466 usec, and base takes 7,649,595 usec, so v2 = base / 3.179, and this satisfies v2 <= base / (num_cores - 1) = base / (4 - 1) = base / 3.
- This speed up occurs due to improving the balance between the number of threads used and the granularity of the locks. In version 1, there is one lock for the entire hash table, and the more threads we add, the slower it runs relative to the base. In version 2, there is a lock for each index in the hash table, and the more threads we add, the faster it runs relative to the base. Version 1 utilizes a coarse-grained lock with a larger critical section, while Version 2 utilizes a fine-grained lock with a smaller critical section. Version 2 has a higher grade of concurrency because while certain indices are locked, threads can still interact with the other indices.


## Cleaning up
To clean up the directory, run:
```shell
make clean
```