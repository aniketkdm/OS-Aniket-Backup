#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>

#include			"structuresStacksQueues.c"
//#include			"globalVariables.c"

// this variable will define 
// whether to print the bulk printf statements or not
// 1 - print outputs; 0 - omit printing
int output = 0;


/**********************************************************************************
MESSAGE
To display only the intended output, Message function is used. 
Instead of deleting the printf statements a flagging mechanism is used,
so that printing and supressing the printfs can be easily switched
***********************************************************************************/
void Message(char toPrint[10000])
{
	if (output == 1)
	{
		printf("%s", toPrint);
	}
}

// PCB_COUNT: is global variable, that keeps track of the count of processes in PCB
int PCB_COUNT = 0;

// top_process would point to the last added process in PCB
PCB_stack *top_process = NULL;

// TIMER QUEUE Doubly-Linked List Global variables:
// FRONT_TIMER_QUEUE points to the 1st process in the timer queue
// REAR_TIMER_QUEUE points to the last process in the timer queue
// COUNT_TIMER_QUEUE keeps track of the number of processes in the timer queue
timer_queue *front_timer_queue = NULL;
timer_queue *rear_timer_queue = NULL;
int count_timer_queue = 0;


// READY QUEUE Doubly-Linked List Global variables:
// FRONT_READY_QUEUE points to the 1st process in the ready queue
// REAR_READY_QUEUE points to the last process in the ready queue
// COUNT_READY_QUEUE keeps track of the number of processes in the ready queue
ready_queue *front_ready_queue = NULL;
ready_queue *rear_ready_queue = NULL;
int count_ready_queue = 0;


/******************************************************************************
PRINT_READY_QUEUE
This function is used to print the ready queue.
variable tmp traverses the ready queue ,
starting from the front process in the ready queue till the rear process in the ready queue
*******************************************************************************/

void print_ready_queue()
{
	/*INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/


	ready_queue *tmp = front_ready_queue;
	int ready_position = 1;
	printf("initial ready position: %d\tcount_ready_queue: %d\n", ready_position, count_ready_queue);
	while (ready_position <= count_ready_queue)
	{
		printf("ready_position: %d\tprocess_name: %s\n", ready_position, tmp->current_ready_process_addr->process_name);
		if (count_ready_queue > 1)
		{
			tmp = tmp->next_ready_process;
		}
		
		ready_position++;
	}
	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/
}

/**************************************************************************************
SORT_READY_QUEUE
this will sort the ready queue based on the priorities
Simple Bubble Sort mechanism is implemented
***************************************************************************************/

void sort_ready_queue()
{
	ready_queue *temp; ready_queue *before, *after, *mover;
	int i, j;

	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));


	printf("in sort ready queue, count_ready_queue: %d\n", count_ready_queue);

	if (count_ready_queue > 1)
	{

		for (i = 1; i <= count_ready_queue; i++)
		{
			temp = front_ready_queue;
			for (j = 1; j <= count_ready_queue - i; j++)
			{
				if (temp->current_ready_process_addr->priority > temp->next_ready_process->current_ready_process_addr->priority)
				{
					printf("current priority higher than next processes priority\n");
					before = temp->prev_ready_process;
					after = temp->next_ready_process;
					if (before != NULL)
					{
						before->next_ready_process = after;
					}
					temp->next_ready_process = after->next_ready_process;
					temp->prev_ready_process = after;
					after->next_ready_process = temp;
					after->prev_ready_process = before;
				}
				else
				{
					if (temp->next_ready_process != NULL)
					{
						temp = temp->next_ready_process;
					}
				}
				mover = front_ready_queue;
				while (mover->next_ready_process != NULL)
				{
					mover = mover->next_ready_process;
				}
				rear_ready_queue = mover;

				while (mover->prev_ready_process != NULL)
				{
					mover = mover->prev_ready_process;
				}
				front_ready_queue = mover;
			}
		}
		printf("front: %s\trear: %s\n", front_ready_queue->current_ready_process_addr->process_name, rear_ready_queue->current_ready_process_addr->process_name);
		printf("sorting ready queue completed\n");

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));

		print_ready_queue();
	}
	else
	{
		printf("in sort ready queue: count of ready queue is %d; No sorting required. returning\n", count_ready_queue);
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));
	}
	

	return;
}

/***************************************************************************************************
GETCURRENTRUNNINGPROCESS
Returns the PCB of the process that is currently running
Uses the "GetCurrentContext" mechanism to get the address of the context that is running
using the context obtained, traverses through the PCB to find the Process on the PCB_STACK
****************************************************************************************************/

PCB_stack* GetCurrentRunningProcess()
{
	MEMORY_MAPPED_IO mmio;  
	long CurrentRunningProcessContext; PCB_stack *tmp = top_process; int i = PCB_COUNT;
	//short *PAGE_TBL_ADDR;

	mmio.Mode = Z502GetCurrentContext;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502Context, &mmio);
	SuccessExpected(mmio.Field4, "GetCurrentContext");

	CurrentRunningProcessContext = mmio.Field1;

	while (i != 0)
	{
		if (tmp->process_context == CurrentRunningProcessContext)
		{
			break;
		}
		tmp = tmp->prev_process; i--;
	}

	if (i != 0)
	{
		printf("Current Running Process is: %s\n", tmp->process_name);
		return tmp;
	}
	else
	{
		printf("Process not found with context value: %d", CurrentRunningProcessContext);	
		return NULL;
	}

}


/***********************************************************************************************
PRINT_TIMER_QUEUE
This function is used to print the timer queue.
variable tmp traverses the timer queue ,
starting from the front process in the timer queue till the rear process in the timer queue
************************************************************************************************/

void print_timer_queue()
{
	timer_queue *tmp; 
	int timer_position = 1;

	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";
	
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));

	tmp = front_timer_queue;


	printf("count of timer queue: %d\n", count_timer_queue);
	while (timer_position <= count_timer_queue)
	{
		printf("in print timer while\n");
		if (tmp != NULL && tmp->current_timer_process != NULL)
		{
			printf("timer_position: %d\tprocess_name: %s\tupdate_sleep_time: %d\n", timer_position, tmp->current_timer_process->process_name, tmp->current_timer_process->updated_sleep_time);
			tmp = tmp->next_process_context;
			printf("moved next timer element\n");
			timer_position++;
		}
		else
		{
			break;
		}
		
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));
}

/**************************************************************************************
SORT_timer_QUEUE
this will sort the timer queue based on the sleep time
Simple Bubble Sort mechanism is implemented
***************************************************************************************/

void sort_timer_queue()
{
	timer_queue *temp; timer_queue *before, *after, *mover;
	int i, j;

	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/


	printf("in sort timer queue, count_timer_queue: %d\n",count_timer_queue);

	if (count_timer_queue > 1)
	{	
		
		for (i = 1; i <= count_timer_queue; i++)
		{
			temp = front_timer_queue;
			for (j = 1; j <= count_timer_queue - i; j++)
			{
				if (temp->current_timer_process->updated_sleep_time > temp->next_process_context->current_timer_process->updated_sleep_time)
				{
					printf("current sleep time greater than next sleep time\n");
					before = temp->prev_process_context;
					after = temp->next_process_context;
					if (before != NULL)
					{
						before->next_process_context = after;
					}
					temp->next_process_context = after->next_process_context;
					temp->prev_process_context = after;
					after->next_process_context = temp;
					after->prev_process_context = before;
				}
				else
				{
					if (temp->next_process_context != NULL)
					{
						temp = temp->next_process_context;
					}
				}
				mover = front_timer_queue;
				while (mover->next_process_context != NULL)
				{
					mover = mover->next_process_context;
				}
				rear_timer_queue = mover;

				while (mover->prev_process_context != NULL)
				{
					mover = mover->prev_process_context;
				}
				front_timer_queue = mover;
			}
		}
		printf("front: %s\trear: %s\n", front_timer_queue->current_timer_process->process_name, rear_timer_queue->current_timer_process->process_name);
		printf("sorting timer queue completed\n");

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));*/
		return;

		print_timer_queue();
	}
	else
	{
		printf("in sort timer queue: count of timer queue is %d; No sorting required. returning\n",count_timer_queue);
	}
	
}

/***********************************************************************************************
PUSH_READY_QUEUE
Adds the new process at the rear of the ready queue
And then calls the SORT_READY_QUEUE,
So that the ready queue is sorted w.r.t the priorities
************************************************************************************************/

void push_ready_queue(PCB_stack *P) 
{
	ready_queue *R = (ready_queue *)calloc(1, sizeof(ready_queue));
	R->current_ready_process_addr = P;
	R->next_ready_process = R->prev_ready_process = NULL;

	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));

	
	if (count_ready_queue == 0)
	{
		count_ready_queue++;
		front_ready_queue = rear_ready_queue = R;

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));
		//return;
	}
	else
	{
		count_ready_queue++;
		R->prev_ready_process = rear_ready_queue;
		rear_ready_queue->next_ready_process = R;
		rear_ready_queue = R;

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));

		sort_ready_queue();
		//return;
	}

	

	print_ready_queue();
	
}

/***********************************************************************************************
MAKE_READY_TO_RUN
whenever called, it calls the "GetCurrenRunningProcess" 
To get the context of the process
And adds it to the timer queue
************************************************************************************************/

void make_ready_to_run()
{
	PCB_stack *ProcessToReady;

	/*INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/


	ProcessToReady = GetCurrentRunningProcess();

	push_ready_queue(ProcessToReady);

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/
}

/***********************************************************************************************
REMOVE_PCB_READY_QUEUE
Removes the supplied process from the ready
This is called while popping the process from the PCB,
As whenever the process is popped from the PCB (test1b - Terminate_Process()),
it should also be removed from the ready queue
************************************************************************************************/

void remove_PCB_ready_queue(PCB_stack *temp)
{
	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));


	ready_queue *tmp = front_ready_queue;
	int cnt = count_ready_queue;

	while (cnt != 0)
	{
		if (tmp->current_ready_process_addr->process_context == temp->process_context)
		{
			if (tmp != rear_ready_queue)
			{
				tmp->next_ready_process->prev_ready_process = tmp->prev_ready_process;
			}
			else
			{
				rear_ready_queue = tmp->prev_ready_process;
			}
			
			if (tmp != front_ready_queue)
			{
				tmp->prev_ready_process->next_ready_process = tmp->next_ready_process;
			}
			else
			{
				front_ready_queue = tmp->next_ready_process;
			}
			

			count_ready_queue--;
			

			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
				&LockResult);
			printf("%s\n", &(Success[SPART * LockResult]));
			
			free(tmp);
			return;
		}
		cnt--;
		tmp = tmp->next_ready_process;
	}

	printf("%s not found on ready queue, hence not removed\n", temp->process_name);

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));
}

/***********************************************************************************************
POP_PROCESS
Pops the supplied process from the PCB
this is invoked when in test1b, terminate process is called
************************************************************************************************/

void pop_process(SYSTEM_CALL_DATA *pop_process_data)
{
	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));


	int i = PCB_COUNT; PCB_stack *temp = top_process; PCB_stack *move; PCB_stack *above;
	//printf("here");
	printf("process context to pop: %d\n", pop_process_data->Argument[0]);
	above = top_process;
	while (i != 0)
	{
		//printf("%d\t%s\t\n", i, top_process->process_name);
		//printf("%d\t%s\t\n\n", i,temp->process_name);
		if (temp->process_context == pop_process_data->Argument[0])
		{
			// if the process is being removed from the PCB,
			// it should also be removed from the ready_queue

			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
				&LockResult);
			printf("%s\n", &(Success[SPART * LockResult]));

			remove_PCB_ready_queue(temp);

			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
				&LockResult);
			printf("%s\n", &(Success[SPART * LockResult]));
			
			//printf("here");
			if (temp->prev_process == NULL)
			{
				//printf("here2");
				above->prev_process = NULL;
			}
			else
			{
				//printf("here3");
				above->prev_process = temp->prev_process;
			}

			printf("process context: %d popped out of PCB successfully\n", temp->process_context);
			*pop_process_data->Argument[1] = ERR_SUCCESS;
			
			READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
				&LockResult);
			printf("%s\n", &(Success[SPART * LockResult]));
			
			free(temp);
			PCB_COUNT--;

			

			return;
		}
		above = temp;
		temp = temp->prev_process;
		i--;

	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));
}


/***********************************************************************************************
GET_PROCESS_ID
Returns Success or failure w.r.t whether the process is present or not in the PCB
************************************************************************************************/

void get_process_id(SYSTEM_CALL_DATA *ReturnProcessData)
{
	PCB_stack *temp = top_process; int i = PCB_COUNT;

	if (strcmp(ReturnProcessData->Argument[0], "") == 0)
	{

		*ReturnProcessData->Argument[1] = temp->process_context;
		if (temp->processing_status == 3)
		{
			*ReturnProcessData->Argument[2] = ERR_PROCESSED;
		}
		else
		{
			*ReturnProcessData->Argument[2] = ERR_SUCCESS;
		}
		
		return;
	}

	while (strcmp(temp->process_name, ReturnProcessData->Argument[0]) != 0 && i != 0)
	{
		temp = temp->prev_process;
		i--;
	}
	if (strcmp(temp->process_name, ReturnProcessData->Argument[0]) == 0)
	{
		*ReturnProcessData->Argument[1] = temp->process_context;
		if (temp->processing_status == 3)
		{
			*ReturnProcessData->Argument[2] = ERR_PROCESSED;
		}
		else
		{
			*ReturnProcessData->Argument[2] = ERR_SUCCESS;
		}
	}
	else
	{
		*ReturnProcessData->Argument[2] = ERR_PROCESS_NOT_FOUND;
	}
	return;
}

/***********************************************************************************************
VALIDATE_PROCESS_DATA
This function validates the process data before creating it
Details of each validation are mentioned in-line
************************************************************************************************/

void Validate_Process_Data(SYSTEM_CALL_DATA *create_process_data)
{
	int i = PCB_COUNT; PCB_stack *temp = top_process;
	
	//validating priority
	if ((int)create_process_data->Argument[2] > 0)
	{
		*create_process_data->Argument[4] = ERR_SUCCESS;
		printf("%s priority validated\n", create_process_data->Argument[0]);
	}
	else
	{
		*create_process_data->Argument[4] = ERR_INCORRECT_PRIORITY;
		printf("Priority assigned is incorrect. Error Number: %d\n", *create_process_data->Argument[4]);
		return;
	}
	

	//validating the process_name
	printf("validating process_names\n");
	while (i != 0)
	{

		if (stricmp(temp->process_name, create_process_data->Argument[0]) == 0)
		{
			*create_process_data->Argument[4] = ERR_INCORRECT_PROCESS_NAME;
			return;
		}
		temp = temp->prev_process;
		i--;
	}

	//validating the number of processes
	printf("Number of processes already created: %d\n", PCB_COUNT);
	if (PCB_COUNT >= 18) // Limiting the number of processes that can be created
	{
		//printf("Max number of processes allowed: 2\n");
		*create_process_data->Argument[4] = ERR_MAX_PROCESSES_REACHED;
		printf("validating count %d\n", *create_process_data->Argument[4]);
		return;
	}
}

/***********************************************************************************************
ONLY_CREATE_PROCESS
This method will only create a context but will not start it
After creating the process, it is added to the PCB
Also, the process is pushed into the ready_queue,
Which then calls the sort_ready_queue to sort the ready_queue after adding the new process
************************************************************************************************/

long only_create_process(SYSTEM_CALL_DATA *create_process_data)
{
	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";


	PCB_stack *P = (PCB_stack *)calloc(1, sizeof(PCB_stack));
	MEMORY_MAPPED_IO mmio;
	void *PageTable = (void *)calloc(2, VIRTUAL_MEM_PAGES);

	Validate_Process_Data(create_process_data);
	if (*create_process_data->Argument[4] != ERR_SUCCESS)
	{
		printf("%d\n", *create_process_data->Argument[4]);
		printf("Exiting!!! As error has occurred\n");
		return;
	}


	//Initialize Context
	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = create_process_data->Argument[1];
	mmio.Field3 = (long)PageTable;

	//printf("%d\t%d\n", mmio.Field2,(long)test1a);

	MEM_WRITE(Z502Context, &mmio);   // Start of Make Context Sequence

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));


	//Here we enter the process context details in the PCB
	P->PID = ++PCB_COUNT;
	strcpy(P->process_name, create_process_data->Argument[0]);
	//printf("%s\n", P->process_name);
	P->process_to_run = create_process_data->Argument[1];
	P->priority = create_process_data->Argument[2];
	P->process_context = mmio.Field1;
	//Processing_status of the newly created process should be 'Initialized: To be Picked: 0'
	P->processing_status = PROCESS_INITIALIZED;
	//process page table
	P->pageTable = PageTable;
	P->updated_sleep_time = 0;
	// New process created should point to the previous process in the PCB
	P->prev_process = top_process;

	
	top_process = P;
	//printf("%d\t%s\t%d\t%d\t%d\n", P->PID, P->process_name, P->process_to_run, P->priority, P->process_context);
	printf("PID: %d, process_context: %d initialized successfully with %d\n", P->PID, P->process_context, create_process_data->Argument[3]);

	*create_process_data->Argument[3] = mmio.Field1;
	printf("%d\t", *create_process_data->Argument[3]);
	*create_process_data->Argument[4] = ERR_SUCCESS;

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));

	push_ready_queue(P);
	P->processing_status = ON_READY_QUEUE;

	

	return mmio.Field1;

}

/***********************************************************************************************
OS_CREATE_PROCESS
Starts the context of the process whose detail are passed to this function
************************************************************************************************/

void os_create_process(SYSTEM_CALL_DATA *create_process_data)
{
	MEMORY_MAPPED_IO mmio;
	printf("starting context for: %s\n", create_process_data->Argument[0]);
	// below statement will call a method to create a process and 
	// the called method returns the context created
	//mmio.Field1 = only_create_process(create_process_data);
	mmio.Field1 = *create_process_data->Argument[3];
	//Start Context
	mmio.Mode = Z502StartContext;
	
	// Field1 contains the value of the context returned in the last call
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	//mmio.Field3 = mmio.Field4 = 0;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
}

/***********************************************************************************************
POP_READY_QUEUE
Pops the top priority process, the process at the front of the ready queue
************************************************************************************************/

void pop_ready_queue()
{
	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));


	ready_queue *tmp = front_ready_queue;
	printf("in pop, count_ready_queue: %d\n", count_ready_queue);
	if (count_ready_queue > 1)
	{
		printf("next ready queue process: %s\n", front_ready_queue->next_ready_process->current_ready_process_addr->process_name);

		front_ready_queue = front_ready_queue->next_ready_process;

		front_ready_queue->prev_ready_process = NULL;

		printf("after poping front ready queue points to: %s\n", front_ready_queue->current_ready_process_addr->process_name);
	}
	free(tmp);
	count_ready_queue--;
	printf("Count of ready queue after popping: %d\n", count_ready_queue);

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));

	return;
}

/***********************************************************************************************
WASTE_TIME
Called in Dispatcher,
To waste time, until the some process is ready in the ready queue
************************************************************************************************/

void waste_time() {};

/***********************************************************************************************
POPTIMERQUEUE
Pops the process from the timer queue, 
whose timer has processed
************************************************************************************************/

void popTimerQueue()
{
	INT32 LockResult; timer_queue *temp;
	char Success[] = "      Action Failed\0        Action Succeeded";

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));


	temp = front_timer_queue;
	

	int cnt = count_timer_queue;

	while (cnt != 0)
	{
		if (temp->current_timer_process->updated_sleep_time <= 0)
		{
			// updating the process status
			// to indicate that it is processed
			temp->current_timer_process->processing_status = PROCESSED;
			printf("%s is being popped out of the timer queue as it's timer has expired\n", temp->current_timer_process->process_name);
			count_timer_queue--;
			// entering this if condition means 
			// that the process has finished sleeping for required time
			// it needs to be popped out of the timer queue

			if (temp == front_timer_queue)
			{
				if (temp != rear_timer_queue)
				{
					front_timer_queue = temp->next_process_context;
					front_timer_queue->prev_process_context = NULL;
				}
				else
				{	
					front_timer_queue = rear_timer_queue = NULL;
					free(temp);
					printf("Timer queue empty after popping\n");

					READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult);
					printf("%s\n", &(Success[SPART * LockResult]));

					return;
				}

			}

		}
		cnt--; temp = temp->next_process_context;
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));
}


/***********************************************************************************************
UPDATETIMERQUEUE
Updates the processes in the timer queue with the remaining time
Interrupt Handler calls updateTimerQueue to update the times in the timer_queue after Interrupt
************************************************************************************************/

void updateTimerQueue(long timeOfDay)
{
	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));


	timer_queue *tmp = front_timer_queue; long timeElapsed;
	int t = count_timer_queue;
	printf("count timer queue in update timer: %d\n", count_timer_queue);
	while (t != 0)
	{
		printf("T=%d\n",t);
		printf("before updating %s the update_sleep_time is: %d\n", tmp->current_timer_process->process_name, tmp->current_timer_process->updated_sleep_time);
		timeElapsed = timeOfDay - tmp->sleep_start_time;
		printf("timeElapsed: %d\n", timeElapsed);
		// if updated sleep time = 0, then that means sleep for that Process has not started
		// if so, then the the sleep time required by it should not be updated
		if (tmp->current_timer_process->updated_sleep_time != 0)
		{
			printf("updating sleep time for %s\n", tmp->current_timer_process->process_name);
			tmp->current_timer_process->updated_sleep_time = tmp->current_timer_process->updated_sleep_time - timeElapsed;
		}
		
		t--; tmp=tmp->next_process_context;
	}
	printf("printing timer queue after updating sleep times\n");
	print_timer_queue();

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));

	popTimerQueue();

}

/***********************************************************************************************
DISPATCHER
Dispatches the process with highest priority on the ready queue
i.e. the front process in the ready queue
it calls the wasteTime() to waste time if the ready is temporarily empty, 
Until the ready queue has a process that can be dispatched
************************************************************************************************/

PCB_stack* dispatcher()
{
	PCB_stack *process_to_dispatch;// = (PCB_stack *)malloc(1 * sizeof(PCB_stack));
	//SYSTEM_CALL_DATA *dispatcher_create_process = (SYSTEM_CALL_DATA *)malloc(1*sizeof(SYSTEM_CALL_DATA));
	MEMORY_MAPPED_IO mmio;
	int wasteTimeInt = 0;
	long timeOfDay;

	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/


	while (count_ready_queue == 0 && count_timer_queue != 0) // && wasteTimeInt < 5)
	{
		CALL(waste_time());
		wasteTimeInt++;

		if (wasteTimeInt == 200 || wasteTimeInt == 400)
		{
			printf("wasting time in dispatcher\n");
			GET_TIME_OF_DAY(&timeOfDay);
		}
		
	}
	
	if (count_ready_queue != 0)
	{
		process_to_dispatch = front_ready_queue->current_ready_process_addr;
		printf("%s will be dispatched to the timer queue from the dispatcher\n", process_to_dispatch->process_name);
		//pop_ready_queue();

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));*/

		return process_to_dispatch;
	}
	else
	{
		printf("There is no process on the ready to be dispatched\n");

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));*/

		return NULL;
	}
	
	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/
		
}

/***********************************************************************************************
ADDTOTIMERQUEUE
Adds the process to the rear of the timer queue,
Then calls the sort_timer_queue
To sort the timer queue w.r.t the required time to sleep
************************************************************************************************/

void AddToTimerQueue(long sleep_time)
{
	PCB_stack *current_process; // = (PCB_stack *)malloc(1 * sizeof(PCB_stack));
	timer_queue *new_timer_process = (timer_queue *)malloc(1 * sizeof(timer_queue));
	long timeOfDay;
	printf("****");
	current_process = dispatcher();
	printf("****");
	if (current_process == NULL)
	{
		make_ready_to_run();
		current_process = dispatcher();
	}

	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/


	printf("process dispatched: %s\n", current_process->process_name);
	current_process->updated_sleep_time = sleep_time;
	new_timer_process->current_timer_process = current_process;
	new_timer_process->process_context = current_process->process_context;
	new_timer_process->next_process_context = new_timer_process->prev_process_context = NULL;
	GET_TIME_OF_DAY(&timeOfDay);
	new_timer_process->sleep_start_time = timeOfDay;
	if (front_timer_queue == NULL && rear_timer_queue == NULL)
	{	
		printf("inserting 1st element in timer queue\n");
		count_timer_queue++;
		front_timer_queue = rear_timer_queue = new_timer_process;

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));*/

		print_timer_queue();
	}
	else
	{
		count_timer_queue++;
		printf("before adding and sorting the timer queue\n");
		rear_timer_queue->next_process_context = new_timer_process;
		new_timer_process->prev_process_context = rear_timer_queue;
		rear_timer_queue = new_timer_process;
		printf("**Printing timer_queue before sorting\n");
		printf("%s\n", rear_timer_queue->current_timer_process->process_name);
		print_timer_queue();

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));*/

		sort_timer_queue();
	}
	
	return;
}


/************************************************************************
CUSTOMSTARTTIMER
Adds the sleep time to the timer queue
starts the timer
Calls the dispatcher
Switches the context if required
*************************************************************************/
void CustomStartTimer(long start_time, long sleep_time)
{
	MEMORY_MAPPED_IO mmio; long context; long TimerStartTime; int wasteTimeInt = 0;
	long Status;
	PCB_stack *ready_process_start = (PCB_stack *)calloc(1,sizeof(PCB_stack));
	SYSTEM_CALL_DATA *create_process_data = (SYSTEM_CALL_DATA *)calloc(1,sizeof(SYSTEM_CALL_DATA));

	printf("in start timer\n");

	//make_ready_to_run();

	AddToTimerQueue(sleep_time);
	pop_ready_queue();
	/*while (wasteTimeInt < 100)
	{
		CALL(waste_time());
		wasteTimeInt++;
	}*/	

	//printf("Starting Timer at: ");
	//GET_TIME_OF_DAY(&TimerStartTime);

	printf("sleeping for: %d\n", sleep_time);

	printf("checking the status before starting the timer\n");

	SetMode(KERNEL_MODE);

	/*mmio.Mode = Z502Status;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502Timer, &mmio);
	Status = mmio.Field1;

	if (Status == DEVICE_IN_USE)
		printf("Got expected result (DEVICE_IN_USE) for Status of Timer\n");
	else
		printf("Got erroneous result for Status of Timer\n");*/

	do
	{
		mmio.Mode = Z502Start;
		mmio.Field1 = sleep_time;   // You pick the time units
		mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
		MEM_WRITE(Z502Timer, &mmio);

		//Check whether timer is running or not
		mmio.Mode = Z502Status;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
		MEM_READ(Z502Timer, &mmio);
		Status = mmio.Field1;
	} while (Status != DEVICE_IN_USE);
	
	
	//printf("Status=%d\n", Status);
	if (Status == DEVICE_IN_USE)
		printf("Got expected result (DEVICE_IN_USE) for Status of Timer\n");
	else
		printf("Got erroneous result for Status of Timer\n");

	printf(
		"\nThe next output from the Interrupt Handler should report that \n");
	printf("   interrupt of device 4 has occurred with no error.\n");

	if (count_timer_queue != 0 && count_ready_queue == 0)
	{
		make_ready_to_run();
	}

	printf("calling dispatcher after start timer\n");

	ready_process_start = dispatcher();

	if (ready_process_start != NULL)
	{
		context = ready_process_start->process_context;
		printf("process returned by dispatcher: %s\n", ready_process_start->process_name);

		create_process_data->NumberOfArguments = 5;
		create_process_data->Argument[0] = (long *)ready_process_start->process_name;
		create_process_data->Argument[1] = NULL;
		create_process_data->Argument[2] = NULL;
		create_process_data->Argument[3] = &context;
		create_process_data->Argument[4] = (long *)ERR_SUCCESS;
		//printf("process_name: %s\n process_to_run: %d \n", create_process_data->Argument[0], Create_Process_Data->Argument[1]);

		os_create_process(create_process_data);
	}
	

	
	//sleep-idle code
	/*mmio.Mode = Z502Action;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	printf("halt started\n");
	MEM_WRITE(Z502Idle, &mmio);       //  Let the interrupt for this timer occur
									  //DoSleep(sleep_time);*/
	printf("halt ended\n");
}
