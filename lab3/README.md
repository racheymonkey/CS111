# Hash Hash Hash

In this lab, I aim to develop a concurrent hash table implementation by adding mutex locks to ensure thread safety. I'll be working with a serial hash table implementation provided as a skeleton, and I will enhance it with two different locking strategies, and comparing their performance against the base implementation.

## Building

To build the project, navigate to the lab3 directory and run make.

## Running

To execute the program, use the hash-table-tester executable with the desired number of threads and hash table entries per thread. For example:

```shell
./hash-table-tester -t 8 -s 50000
```

This command will run the tester with 8 threads and 50000 hash table entries per thread.

### Results

The tester will display the execution time for each hash table implementation, along with any missing entries. When I ran this code on my terminal, I got the following:

Generation: 72,722 usec
Hash table base: 1,701,029 usec
0 missing
Hash table v1: 2,470,028 usec
0 missing
Hash table v2: 1,142,474 usec
0 missing

## First Implementation: Version 1 (v1)

In this version, I'll focus on correctness by introducing a simple locking strategy using a single mutex.

### Implementation Details

I'll modify the hash_table_v1_add_entry function to add mutex locking, ensuring thread safety. This approach employs a single mutex to protect critical sections.

### Explanation of Mutex Usage

The mutex is initialized at the creation of the hash table and is locked before accessing or modifying any shared data structures. This ensures that only one thread can execute the critical sections at a time, preventing data corruption or race conditions.

### Performance

As shown ealier in the results of the command:

Generation: 72,722 usec
Hash table base: 1,701,029 usec
0 missing
Hash table v1: 2,470,028 usec
0 missing
Hash table v2: 1,142,474 usec
0 missing

This aligns with the expected program behavior. Version 1 might exhibit slightly slower performance compared to the base implementation due to the overhead of thread synchronization. This overhead primarily stems from the creation and management of mutex locks.

## Second Implementation: Version 2 (v2)

In this version, I'll focus on both correctness and performance by implementing a more sophisticated locking strategy using multiple mutexes.

### Implementation Details

I'll enhance the hash_table_v2_add_entry function with a more complex locking mechanism using multiple mutexes. This approach aims to reduce contention and improve concurrency by allowing different threads to access independent portions of the hash table simultaneously.

### Explanation of Mutex Usage

Multiple mutexes are created and associated with different sections of the hash table. Each mutex guards a specific portion of the data structure, enabling finer-grained concurrency control. This strategy enhances parallelism and reduces contention, leading to better performance.

### Performance

As shown ealier in the results of the command:

Generation: 72,722 usec
Hash table base: 1,701,029 usec
0 missing
Hash table v1: 2,470,028 usec
0 missing
Hash table v2: 1,142,474 usec
0 missing

Version 2 demonstrates significant performance improvement compared to the base implementation, achieving a notable speedup. This enhancement is attributed to the optimized locking strategy, which minimizes thread contention and maximizes parallelism.

## Cleaning up

To clean up the project directory, run make clean.
