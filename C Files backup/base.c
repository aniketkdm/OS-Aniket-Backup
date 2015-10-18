/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Portability attempted.
 1.3 July     1992: More Portability enhancements.
 Add call to SampleCode.
 1.4 December 1992: Limit (temporarily) printout in
 interrupt handler.  More portability.
 2.0 January  2000: A number of small changes.
 2.1 May      2001: Bug fixes and clear STAT_VECTOR
 2.2 July     2002: Make code appropriate for undergrads.
 Default program start is in test0.
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 4.0  July    2013: Major portions rewritten to support multiple threads
 4.20 Jan     2015: Thread safe code - prepare for multiprocessors
 ************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>

//include the structures, global variables and functions
#include			"structuresStacksQueues.c"
//#include			"globalVariables.c"
//#include			"functions.c"

//  Allows the OS and the hardware to agree on where faults occur
extern void *TO_VECTOR[];

char *call_names[] = { "mem_read ", "mem_write", "read_mod ", "get_time ",
		"sleep    ", "get_pid  ", "create   ", "term_proc", "suspend  ",
		"resume   ", "ch_prior ", "send     ", "receive  ", "disk_read",
		"disk_wrt ", "def_sh_ar" };

/*typedef struct Create_Process_Struct {
	char process_name[20];
	long process_to_run;
	long priority;
	long *process_context;
	long *return_status;
} Create_Process_Struct; */


/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the OS.
 ************************************************************************/
void InterruptHandler(void) {
	INT32 DeviceID; long timeOfDay;
//	INT32 Status; 
	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	PCB_stack *tmp;
	int z;
	ready_queue *readyTmp;
	timer_queue *timerTmp;
	SP_INPUT_DATA SPData;    // Used to feed SchedulerPrinter


	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	static BOOL remove_this_in_your_code = TRUE; /** TEMP **/
	static INT32 how_many_interrupt_entries = 0; /** TEMP **/

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/

	READ_MODIFY(MEMORY_INTERLOCK_BASE + 30, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	Message("%s\n", &(Success[SPART * LockResult]));


	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	//Status = mmio.Field2;


	/** REMOVE THE NEXT SIX LINES **/
	how_many_interrupt_entries++; /** TEMP **/
	//if (remove_this_in_your_code && (how_many_interrupt_entries < 40)) {
		printf("Interrupt_handler: Found device ID %d with status %d\n",
				(int) mmio.Field1, (int) mmio.Field2);
	//}

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));*/

	// Clear out this device - we're done with it
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	mmio.Field2 = mmio.Field3 = 0;
	MEM_WRITE(Z502InterruptDevice, &mmio);

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/

	//popTimerQueue();
	GET_TIME_OF_DAY(&timeOfDay);

	// trying to get a lock
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	Message("%s\n", &(Success[SPART * LockResult]));

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	Message("%s\n", &(Success[SPART * LockResult]));

	updateTimerQueue(timeOfDay);

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));
*/
	popTimerQueue();

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));

	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));
*/
	sort_timer_queue();

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/
	//make_ready_to_run();

	READ_MODIFY(MEMORY_INTERLOCK_BASE + 30, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	Message("%s\n", &(Success[SPART * LockResult]));

	Message("interrupt handler completed\n");
}           // End of InterruptHandler


/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.

 Handling Privilege instructuions:
 DeviceID for Privileged_Instruction is changed to '9' in global.h
 So that Timer_interrupt(4) can be distinguished from Privilege_Instruction
 If Privilege Instruction is received, Terminate All routine is called
 ************************************************************************/

void FaultHandler(void) {
	INT32 DeviceID;
	INT32 Status; 
	long ErrorReturned;
	SP_INPUT_DATA SPData;    // Used to feed SchedulerPrinter
	PCB_stack *tmp;
	ready_queue* tmpReady;

	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	Status = mmio.Field2;
	int i;

	printf("Fault_handler: Found vector type %d with value %d\n", DeviceID,
			Status);

	// Clear out this device - we're done with it
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	MEM_WRITE(Z502InterruptDevice, &mmio);

	if (DeviceID == PRIVILEGED_INSTRUCTION)
	{
		Message("Privilege Instruction called in User Mode..Exiting\n"); // Message

		memset(&SPData, 0, sizeof(SP_INPUT_DATA));
		strcpy(SPData.TargetAction, "Fault");

		tmp = GetCurrentRunningProcess();

		SPData.CurrentlyRunningPID = tmp->PID;
		SPData.TargetPID = 0;
		// The NumberOfRunningProcesses as used here is for a future implementation
		// when we are running multiple processors.  For right now, set this to 0
		// so it won't be printed out.
		SPData.NumberOfRunningProcesses = 0;

		SPData.NumberOfReadyProcesses = returnReadyQueueCount();   // Processes ready to run

		tmpReady = returnFrontReadyQueue();

		for (i = 0; i < SPData.NumberOfReadyProcesses; i++) {
			if (i == 0)
			{
				//tmpReady = front_ready_queue;
			}
			else
			{
				tmpReady = tmpReady->next_ready_process;
			}
			SPData.ReadyProcessPIDs[i] = tmpReady->current_ready_process_addr->PID;
		}

		CALL(SPPrintLine(&SPData));

		TERMINATE_PROCESS(-2, &ErrorReturned);
	}

} // End of FaultHandler

/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 The variable do_print is designed to print out the data for the
 incoming calls, but does so only for the first ten calls.  This
 allows the user to see what's happening, but doesn't overwhelm
 with the amount of data.
 ************************************************************************/

void svc(SYSTEM_CALL_DATA *SystemCallData) {
	short call_type;
	static short do_print = 10;
	short i;
	SP_INPUT_DATA SPData;    SP_INPUT_DATA SPData2; // Used to feed SchedulerPrinter
	PCB_stack *tmp;
	//int i;
	ready_queue *readyTmp;
	timer_queue *timerTmp;


	MEMORY_MAPPED_IO mmio;


	Message("in SVC before call type: do_print: %d\n", do_print); // Message

	call_type = (short)SystemCallData->SystemCallNumber;
	if (do_print > 0) {
		printf("SVC handler: %s\n", call_names[call_type]);
		for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++) {
			//Value = (long)*SystemCallData->Argument[i];
			printf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
				(unsigned long)SystemCallData->Argument[i],
				(unsigned long)SystemCallData->Argument[i]);
		}
		do_print--;

		Message("in SVC above switch case\n"); // should be changed to Message

		switch (call_type) {
			// Get time service call
		case SYSNUM_GET_TIME_OF_DAY:   // This value is found in syscalls.h
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
			MEM_READ(Z502Clock, &mmio);
			*SystemCallData->Argument[0] = mmio.Field1;
			Message("Time of the day: %d\n", *SystemCallData->Argument[0]); //Message
			break;

			// SLEEP system call
		case SYSNUM_SLEEP:
			CustomStartTimer(0, SystemCallData->Argument[0]);
			break;

		case SYSNUM_CREATE_PROCESS:
			Message("Came in SVC Create Process Block\n"); //Message
														   /*Validate_Process_Data(SystemCallData);*/

			only_create_process(SystemCallData);
			Message("ErrorReturned in SVC: %d\n\n\n", *SystemCallData->Argument[4]); //Message

			if (*SystemCallData->Argument[4] != ERR_SUCCESS)
			{
				Message("%d\n", *SystemCallData->Argument[4]);
				Message("Exiting!!! As error has occurred\n");
				return;
			}

			*SystemCallData->Argument[4] = (long)ERR_SUCCESS;
			break;

			// terminate system call
			// based on the value passed
			// either terminate all (for negative values)
			// or terminate process will be called (positive values)
		case SYSNUM_TERMINATE_PROCESS:

			Message("entered terminate process correctly\n");
			//getch();
			Message("in Terminate switch case\n");
			Message("system argument 0: %ld, %d\n", SystemCallData->Argument[0], SystemCallData->Argument[0]);

			if ((int)SystemCallData->Argument[0] < 0) {
				Message("entered Terminate all mode\n");
				*SystemCallData->Argument[1] = ERR_SUCCESS;
				mmio.Mode = Z502Action;
				mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;

				//getch();

				MEM_WRITE(Z502Halt, 0);
				break;
			}
			else
			{
				Message("entered pop\n");
				pop_process(SystemCallData);
				break;
			}

		case SYSNUM_SUSPEND_PROCESS:

			Message("Suspending %d\n", SystemCallData->Argument[0]);
			CustomSuspendProcess(SystemCallData);

			break;

		case SYSNUM_RESUME_PROCESS:
			Message("Resuming %d\n", SystemCallData->Argument[0]);
			CustomResumeProcess(SystemCallData);

			break;

		case SYSNUM_GET_PROCESS_ID:
			get_process_id(SystemCallData);
			break;

		case SYSNUM_CHANGE_PRIORITY:
			Message("Changing priority for %d\n", SystemCallData->Argument[0]);
			CustomChangePriority(SystemCallData);
			break;

		default:
			Message("ERROR!  call_type not recognized!\n");
			Message("Call_type is - %i\n", call_type);
			break;
		}                                           // End of switch

	}
	else
	{
		switch (call_type) {
			// Get time service call
		case SYSNUM_GET_TIME_OF_DAY:   // This value is found in syscalls.h
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
			MEM_READ(Z502Clock, &mmio);
			*SystemCallData->Argument[0] = mmio.Field1;
			Message("Time of the day: %d\n", *SystemCallData->Argument[0]); //Message
			break;

			// SLEEP system call
		case SYSNUM_SLEEP:
			//SetMode(KERNEL_MODE);
			CustomStartTimer(0, SystemCallData->Argument[0]);
			break;

		case SYSNUM_CREATE_PROCESS:
			Message("Came in SVC Create Process Block\n"); //Message
														   /*Validate_Process_Data(SystemCallData);*/

			only_create_process(SystemCallData);
			Message("ErrorReturned in SVC: %d\n\n\n", *SystemCallData->Argument[4]); //Message

			if (*SystemCallData->Argument[4] != ERR_SUCCESS)
			{
				Message("%d\n", *SystemCallData->Argument[4]);
				Message("Exiting!!! As error has occurred\n");
				return;
			}

			*SystemCallData->Argument[4] = (long)ERR_SUCCESS;
			break;

			// terminate system call
			// based on the value passed
			// either terminate all (for negative values)
			// or terminate process will be called (positive values)
		case SYSNUM_TERMINATE_PROCESS:

			Message("entered terminate process correctly\n");
			//getch();
			Message("in Terminate switch case\n");
			Message("system argument 0: %ld, %d\n", SystemCallData->Argument[0], SystemCallData->Argument[0]);

			if ((int)SystemCallData->Argument[0] < 0) {
				Message("entered Terminate all mode\n");
				*SystemCallData->Argument[1] = ERR_SUCCESS;
				mmio.Mode = Z502Action;
				mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
				MEM_WRITE(Z502Halt, 0);
				break;
			}
			else
			{
				Message("entered pop\n");
				pop_process(SystemCallData);
				break;
			}

		case SYSNUM_SUSPEND_PROCESS:

			Message("Suspending %d\n", SystemCallData->Argument[0]);
			CustomSuspendProcess(SystemCallData);

			break;

		case SYSNUM_RESUME_PROCESS:
			Message("Resuming %d\n", SystemCallData->Argument[0]);
			CustomResumeProcess(SystemCallData);

			break;

		case SYSNUM_GET_PROCESS_ID:
			get_process_id(SystemCallData);
			break;

		case SYSNUM_CHANGE_PRIORITY:
			Message("Changing priority for %d\n", SystemCallData->Argument[0]);
			CustomChangePriority(SystemCallData);
			break;

		default:
			Message("ERROR!  call_type not recognized!\n");
			Message("Call_type is - %i\n", call_type);
			break;
		}                                           // End of switch

	}                                           // End of switch
}

// End of svc


/************************************************************************
 osInit
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/

void osInit(int argc, char *argv[]) {
	void *PageTable = (void *)calloc(2, VIRTUAL_MEM_PAGES);
	INT32 i; char test_no_chr[10]; long status; long context;

	MEMORY_MAPPED_IO mmio;
	SYSTEM_CALL_DATA *Create_Process_Data = (SYSTEM_CALL_DATA *)calloc(1, sizeof(SYSTEM_CALL_DATA));


	// Demonstrates how calling arguments are passed thru to here       
	//printf( "inside osInit\n" );

	printf("Program called with %d arguments:", argc);
	for (i = 0; i < argc; i++)
		printf(" %s", argv[i]);
	printf("\n");
	printf("Calling with argument 'sample' executes the sample program.\n");
	// Here we check if a second argument is present on the command line.
	// If so, run in multiprocessor mode
	if (argc > 2) {
		printf("Simulation is running as a MultProcessor\n\n");
		mmio.Mode = Z502SetProcessorNumber;
		Message("Z502SetProcessorNumber done successfully\n");
		mmio.Field1 = MAX_NUMBER_OF_PROCESSORS;
		Message("MAX_NUMBER_OF_PROCESSORS done successfully\n");
		mmio.Field2 = (long)0;
		mmio.Field3 = (long)0;
		mmio.Field4 = (long)0;

		MEM_WRITE(Z502Processor, &mmio);   // Set the number of processors
	}
	else {
		printf("Simulation is running as a UniProcessor\n");
		printf("Add an 'M' to the command line to invoke multiprocessor operation.\n\n");
	}

	//          Setup so handlers will come to code in base.c   
	//printf("outside multiprocessor if block\n");

	TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR] = (void *)InterruptHandler;
	TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *)FaultHandler;
	TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR] = (void *)svc;

	//  Determine if the switch was set, and if so go to demo routine. 

	PageTable = (void *)calloc(2, VIRTUAL_MEM_PAGES);

	//Below code does the following:
	//1. checks if the command line arguments were passed
	//2. calls the context for appropriate test in the switch case

	if ((argc > 1) && (strcmp(argv[1], "") != 0)) {
		strcpy(test_no_chr, argv[1]);

		if (test_no_chr[4] == '1')
		{
			switch (test_no_chr[5])
			{
			case 'a':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1a";
				Create_Process_Data->Argument[1] = (long *)test1a;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				Message("process_name: %s\n process_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//			Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");
				//if (Create_Process_Data->Argument[4] != ERR_SUCCESS)

				//Message("status:%d", status);
				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					Message("%d Exiting!!! As error has occurred", status);
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'b':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1b";
				Create_Process_Data->Argument[1] = (long *)test1b;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'c':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1c";
				Create_Process_Data->Argument[1] = (long *)test1c;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'd':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1d";
				Create_Process_Data->Argument[1] = (long *)test1d;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'e':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1e";
				Create_Process_Data->Argument[1] = (long *)test1e;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'f':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1f";
				Create_Process_Data->Argument[1] = (long *)test1f;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'g':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1g";
				Create_Process_Data->Argument[1] = (long *)test1g;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'k':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1k";
				Create_Process_Data->Argument[1] = (long *)test1k;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			}

		}


		/*		mmio.Mode = Z502InitializeContext;
		mmio.Field1 = 0;
		mmio.Field2 = (long) test1a;
		mmio.Field3 = (long) PageTable;

		MEM_WRITE(Z502Context, &mmio);   // Start of Make Context Sequence
		mmio.Mode = Z502StartContext;
		// Field1 contains the value of the context returned in the last call
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context
		*/

	} // End of handler for sample code - This routine should never return here

	  //  By default test0 runs if no arguments are given on the command line
	  //  Creation and Switching of contexts should be done in a separate routine.
	  //  This should be done by a "OsMakeProcess" routine, so that
	  //  test0 runs on a process recognized by the operating system.


	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)test0;
	mmio.Field3 = (long)PageTable;

	MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence
	mmio.Mode = Z502StartContext;
	// Field1 contains the value of the context returned in the last call
	// Suspends this current thread
	Message("before START_NEW_CONTEXT_AND_SUSPEND\n");
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	Message("Outside START_NEW_CONTEXT_AND_SUSPEND\n");
	//printf("calling MEM_WRITE\n");
	//printf("%d %x %d", &mmio, &mmio, Z502Context);
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
	Message("after MEM_WRITE\n");
	//printf("osInit completed successfully\n");

}                                               // End of osInit
