#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 remaining_time;
  bool has_started;
  u32 start_time;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

void simulate_round_robin(struct process_list *list, struct process *processes, u32 size, u32 quantum) {
  u32 current_time = 0, completed_processes = 0;
  u32 total_waiting_time = 0, total_response_time = 0;
  struct process *proc, *temp_proc;
  TAILQ_INIT(list);

  while (completed_processes < size) {
    // Add processes to the queue based on arrival time.
    for (u32 i = 0; i < size; i++) {
      if (!processes[i].has_started && processes[i].arrival_time <= current_time) {
        TAILQ_INSERT_TAIL(list, &processes[i], pointers);
        processes[i].has_started = true;
        processes[i].start_time = current_time;
      }
    }

    // Schedule the first process in the queue.
    proc = TAILQ_FIRST(list);
    if (proc != NULL) {
      u32 execute_time = (proc->remaining_time > quantum) ? quantum : proc->remaining_time;
      proc->remaining_time -= execute_time;
      current_time += execute_time;

      // Calculate waiting time for other processes in the queue.
      TAILQ_FOREACH(temp_proc, list, pointers) {
        if (temp_proc != proc) {
          total_waiting_time += execute_time;
        }
      }

      if (proc->remaining_time == 0) {
        // Calculate response time.
        total_response_time += proc->start_time - proc->arrival_time;
        TAILQ_REMOVE(list, proc, pointers);
        completed_processes++;
      } else {
        // Re-add the process to the end of the queue if not finished.
        TAILQ_REMOVE(list, proc, pointers);
        TAILQ_INSERT_TAIL(list, proc, pointers);
      }
    } else {
      // Increment current time if no process is ready to execute.
      current_time++;
    }
  }

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / size);
  printf("Average response time: %.2f\n", (float)total_response_time / size);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  /* Your code here */
  init_processes(argv[1], &data, &size);
  simulate_round_robin(&list, data, size, quantum_length);
  /* End of "Your code here" */

  free(data);
  return 0;
}
