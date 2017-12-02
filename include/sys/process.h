#ifndef _PROCESS_H
#define _PROCESS_H

#include <sys/defs.h>

#define MAX_PROCESSES 1024
#define STACK_SIZE 4096
extern int PID;


typedef struct vm_area_struct {
        struct mm_struct *vm_mm;
        uint64_t vm_start;
        uint64_t vm_end;
        uint64_t vm_file;
        uint64_t vm_pgoff;
        uint64_t vm_flags;
        uint64_t vm_mmsz;
        struct vm_area_struct *vm_next;
} vm_area_struct;

typedef struct mm_struct {
        uint32_t count;
        struct vm_area_struct *mmap;
} mm_struct;

typedef struct task_struct {
	uint64_t *rsp;
	uint64_t kstack[STACK_SIZE];
	uint64_t *ustack;
	uint64_t pid;
	uint64_t ppid;
	uint64_t *cr3;
	char tname[100];
	mm_struct *mm;
	uint64_t pml4;
	int exit_status;
	uint64_t entry;
} task_struct;

typedef struct ready_task {
	task_struct *process;
	struct ready_task *next;
	struct ready_task *prev;

}ready_task;


//task_struct *ready_queue;
//uint64_t num_processes = 0;
void schedule();
void add_to_task_list(task_struct *process);
task_struct *get_running_task();
uint64_t fork();
#endif