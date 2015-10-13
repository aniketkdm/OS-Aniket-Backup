#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>

typedef struct PCB_stack {
	int PID;
	char process_name[20];
	long process_to_run;
	int priority;
	long process_context;
	int processing_status;
	long updated_sleep_time;
	struct PCB_stack *prev_process;
	long pageTable;
} PCB_stack;

typedef struct timer_queue
{
	PCB_stack *current_timer_process;
	long process_context;
	long sleep_time;
	long sleep_start_time;
	struct timer_queue *next_process_context;
	struct timer_queue *prev_process_context;
} timer_queue;

typedef struct ready_queue
{
	PCB_stack *current_ready_process_addr;
	struct ready_queue *next_ready_process;
	struct ready_queue *prev_ready_process;
} ready_queue;