#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// the global variables and registers.
int PC = 0; //Program Counter
int ACC = 0; //Accumulator
int IR = 0; //Instruction register
int memory[256]; //memory (instructions + data)
// the opcode and operand
int opcode;
int operand;

// the different instruction types:
#define ADD_operation 1
#define SUB_operation 2
#define LOAD_operation 3
#define STORE_operation 4
#define MUL_operation 5
#define DIV_operation 6
#define AND_operation 7
#define OR_operation 8
#define JMP_operation 9
#define JZ_operation 10

//Status flag
int interruptFlag = 0; // flag to signal the interrupt
int carryFlag;
int zeroFlag;
int overFlowFlag;

// Memory System
#define RAM_SIZE 1024 //Main memory (RAM)
#define L1_SIZE 64 //L1 Cache
#define L2_SIZE 128 //L2 Cache
int RAM[RAM_SIZE];
int L1Cache[L1_SIZE];
int L2Cache[L2_SIZE];
int L1Tags[L1_SIZE];
int L2Tags[L2_SIZE];
int MEMORY_TABLE_SIZE = RAM_SIZE / 100;


// Process Variables and Structs (M3)
#define MAX_PROCESSES 3
#define TIME_SLICE 5

int currentProcess = 0;

struct PCB {
	int pid;
	int pc;
	int acc;
	int state; // Process state (0 = ready, 1 = running, 2 = blocked, 3 = terminated)
	int time; // Process time
};

struct PCB processTable[MAX_PROCESSES];

// Interrupts (M4)
void (*IVT[3])(); // Interrupt Vector Table

void timerInterrupt() {
	// Handle timer interrupt, call dispatcher if needed
	printf("TIMER INTERRUPT! 2 seconds.\n");
	sleep(2);
	printf("Timer done. Setting another alarm in 8 seconds.\n");
	alarm(8);
}

void ioInterrupt() {
	// Handle I/O interrupt
	printf("I/O INTERRUPT! 2 seconds.\n");
	sleep(2);
	printf("I/O done.\n");
}

void systemCallInterrupt() {
	// Handle system call interrupt
	printf("SYSTEM CALL INTERRUPT! 2 seconds.\n");
	sleep(2);
	printf("System call done.\n");
}


void initProcesses() {
	for (int i = 0; i < MAX_PROCESSES; i++) {
		processTable[i].pid = 100 + (i * 100);
		processTable[i].pc = 0;
		processTable[i].acc = 0;
		processTable[i].state = 0; // ready state
		processTable[i].time = rand() % 25;
	}
}

void contextSwitch(int currProcess, int nextProcess) {
	processTable[currProcess].pc = PC;
	processTable[currProcess].acc = ACC;
	if (processTable[currProcess].state != 3) {
		processTable[currProcess].state = 0;
	}

	PC = processTable[nextProcess].pc;
	ACC = processTable[nextProcess].acc;
	processTable[nextProcess].state = 1;

	printf("Context switch complete! Process %d to Process %d\n", processTable[currProcess].pid, processTable[nextProcess].pid);
}

void dispatcher(int currentProcess, int nextProcess) {
	contextSwitch(currentProcess, nextProcess);
}

void scheduler() {
	// implement round-robin scheduling
	// use a loop to cycle through processes and manage their state
	int nextProcess = (currentProcess + 1) % MAX_PROCESSES;
	printf("Current Process ID: %d, State: %d, Time: %d\n", processTable[nextProcess].pid, processTable[nextProcess].state, processTable[nextProcess].time);
	while (processTable[nextProcess].state == 3) {
		nextProcess = (nextProcess + 1) % MAX_PROCESSES;
		printf("Current Process ID: %d, State: %d, Time: %d\n", processTable[nextProcess].pid, processTable[nextProcess].state, processTable[nextProcess].time);
		if (nextProcess == currentProcess) {
			if (processTable[nextProcess].state != 3) {
				contextSwitch(currentProcess, nextProcess);
				currentProcess = nextProcess;
			} else {
			printf("All processes complete.");
			}
			return;
		}
	}
	if (processTable[nextProcess].state == 0) {
		contextSwitch(currentProcess, nextProcess);
		currentProcess = nextProcess;
	}
}

struct MemoryBlock {
    int processID;
    int memoryStart;
    int memoryEnd;
    int isFree; //1 = Free, 0 = Allocated
    };
struct MemoryBlock memoryTable[RAM_SIZE / 100]; //Example: divide RAM into blocks

void initCache() {
    for (int i = 0; i < L1_SIZE; i++) {
        L1Cache[i] = -1;
    }
    for (int i = 0; i < L2_SIZE; i++) {
        L2Cache[i] = -1;
    }
    for(int i = 0; i < L1_SIZE; i++){
        L1Tags[i] = -1;
    }
    for(int i = 0; i < L2_SIZE; i++){
        L2Tags[i] = -1;
    }
}

//int -> boolean
//purpose: to check to see if an address exist in the L1 cache
bool inCacheL1(int address){
for(int i = 0; i < L1_SIZE; i++){
if (L1Tags[i] == address){
    return true;
}
}
return false;
}

//int -> boolean
//purpose: to check to see if an address exist in the L2 cache
bool inCacheL2(int address){
for(int i = 0; i < L2_SIZE; i++){
    if(L2Tags[i] == address){
        return true;
    }
}
return false;
}

void updateCache(int address){
    int value = RAM[address];
    // if space in cache L1
    for (int i = 0; i < L1_SIZE; i++){
        if(L1Cache[i] == -1){
            L1Tags[i] = address;
            L1Cache[i] = value;
            return;
        }
    }

    // if space in cache L2
    for(int i = 0; i < L2_SIZE; i++){
        if(L2Cache[i] == -1){
            L2Tags[i] = address;
            L2Cache[i] = value;
            return;
        }
    }

    // holds the values so that they do not get lost.
    int temp_value = L1Cache[0];
    int temp_address = L1Tags[0];

    //this will bump the values in l1 up 1 slot
	for(int i = 1; i < L1_SIZE; i++){
		int bump_address = L1Cache[i];
        int bump_tag = L1Tags[i];
		L1Cache[i - 1] = bump_address;
        L1Tags[i - 1] = bump_tag;
	}

    //removes these elements from the queue
	L1Cache[L1_SIZE - 1] = -1;
    L1Tags[L1_SIZE - 1] = -1;
    
    // does the bump for L2
    for(int i = 1; i < L2_SIZE; i++){
        int bump_address = L2Cache[i];
        int bump_tag = L2Tags[i];
        L2Cache[i - 1] = bump_address;
        L2Tags[i - 1] = bump_tag;
    }

	// moves this element to the end of L2
    L2Cache[L2_SIZE - 1] = temp_value;
    L2Tags[L2_SIZE - 1] = temp_address;
	
	//adds the new value to cache L1
	L1Cache[L1_SIZE - 1] = value;
    L1Tags[L1_SIZE - 1] = address;
}

int accessMemory(int address){
    
    //if in cache 1
    for(int i = 0; i < L1_SIZE; i++){
        if(address == L1Tags[i]){
            printf("L1 Cache hit\n");
            return L1Cache[i];
        }
    }
    
    //if in cache 2
    for (int i = 0; i < L2_SIZE; i++){
        if(address == L2Tags[i]){
            printf("L2 Cache hit\n");
            return L2Cache[i];
        }
    }

    //if in neither then update the cache and pull from the RAM.
    printf("Cache miss pulled from RAM\n");
    updateCache(address);
    return RAM[address];
}

//Write to memory
void writeMemory(int address, int value) {
    // Check if the address exists in L1 or L2 and update
    if (inCacheL1(address)) {
        for (int i = 0; i < L1_SIZE; i++) {
            if (L1Tags[i] == address) {
                L1Cache[i] = value; 
            }
        }
    } else if (inCacheL2(address)) {
        for (int i = 0; i < L2_SIZE; i++) {
            if (L2Tags[i] == address) {
                L2Cache[i] = value; 
            }
        }
    }
	updateCache(address);
}


void initMemoryTable() {
    for (int i = 0; i < MEMORY_TABLE_SIZE; i++) {
    memoryTable[i].processID = -1; //Mark all blocks as free
    memoryTable[i].isFree = 1;
    memoryTable[i].memoryStart = i * (RAM_SIZE / MEMORY_TABLE_SIZE); // Initialize memoryStart
    memoryTable[i].memoryEnd = (i + 1) * (RAM_SIZE / MEMORY_TABLE_SIZE) - 1; // Initialize memoryEnd
    }

}

void allocateMemory(int processID, int size) {
 //Implement First-Fit or Best-Fit allocation strategy
 //first fit being used.
 bool successful_alloc = false;
 for(int i = 0; i < RAM_SIZE / 100; i ++){
 if(memoryTable[i].isFree && (memoryTable[i].memoryEnd - memoryTable[i].memoryStart + 1) >= size){
    memoryTable[i].processID = processID;
    memoryTable[i].isFree = 0;
    successful_alloc = true;
    printf("Memory Allocation: ProcessID: %d Memory Start: %d, Memory End: %d \n", processID, memoryTable[i].memoryStart, memoryTable[i].memoryEnd);
    break;
 }

 }
 if(!successful_alloc){
    printf("Memory Allocation Error: Not enough space for Process %d \n", processID);
 }
}

void deallocateMemory(int processID) {
 //Implement logic to free allocated memory
 bool successful_dealloc = false;
 for (int i = 0; i < RAM_SIZE / 100; i++){
    if (memoryTable[i].processID == processID){
        memoryTable[i].processID = -1;
        memoryTable[i].isFree = 1;
        printf("Memory Deallocation: ProcessID: %d Memory Start: %d Memory End: %d \n", processID, memoryTable[i].memoryStart, memoryTable[i].memoryEnd);
        successful_dealloc = true;
        break;
    }
 }
 if(!successful_dealloc){
    printf("Memory Deallocation Error: ProcessID not found\n");
 }
}

// Insturction System below
void fetch() {
 IR = accessMemory(PC); //Fetch the next instruction
}

void decode() {
 //Decode the instruction in IR
 //Implement logic to extract the opcode and operands
 opcode = IR;
 operand = accessMemory(PC + 1);
}

void execute() {
    int result;
    int prev;

    switch (IR) {
        case ADD_operation: 
            prev = ACC;
            result = ACC + operand;
            ACC = result;
            printf("Adding: %d + %d = %d\n", prev, operand, ACC);
            break;
        case SUB_operation:
            prev = ACC;
            result = ACC - operand;
            ACC = result;
            printf("Subtracting: %d - %d = %d\n", prev, operand, ACC);
            break;
        case MUL_operation:
            prev = ACC;
            result = ACC * operand;
            ACC = result;
            printf("Multiplying: %d * %d = %d\n", prev, operand, ACC);
            break;
        case DIV_operation:
            if(operand != 0){
                prev = ACC;
                result = ACC / operand;
                ACC = result;
                printf("Dividing: %d * %d = %d\n", prev, operand, ACC);
                break;
            }
            else{
                printf("Division Error: Division by zero\n");
                break;
            }
        case LOAD_operation:
            prev = ACC;
            ACC = operand;
			printf("Loading data in ACC: %d -> %d\n", prev, ACC);
			break;
        case STORE_operation:
            writeMemory(operand, ACC);
            printf("Storing data into memory from ACC to Adress: %d -> %d\n", ACC, operand);
			break;
        case AND_operation:
            prev = ACC;
            result = ACC & operand;
            ACC = result;
			printf("AND operation: %d & %d = %d \n", prev, operand, ACC);
            break;
        case OR_operation:
            prev = ACC;
            result = ACC | operand;
            ACC = result;
            printf("OR operation: %d | %d = %d \n", prev, operand, ACC);
            break;
        case JMP_operation:
            if (operand >= 0 && operand < 256){
                prev = PC;
                PC = operand - 2;
                printf("Jump: PC has been changed from %d to %d\n", prev, PC);
                break;
            }
            else{
                printf("Error: Invalid Jump attempt. Out of Bounds Error\n");
                break;
            }
        case JZ_operation:
            if(zeroFlag == 1){
                if(operand >= 0 && operand < 256){
                    prev = PC;
                    PC = operand - 2;
                    printf("Jump Zero: PC has been changed from %d to %d\n", prev, PC);
                    break;
                    }
                    else{
                        printf("Error: Invalid Jump attempt. Out of Bounds Error\n");
                        break;
                    }
                
            }
            else{
                printf("Jump Zero: Unsuccessful jump, no zero flag present.\n");
                break;
            }
		default:
			// Handle undefined/invalid opcodes
			printf("Invalid opcode given. Please try again.\n");
			break;
        }
    
    PC += 2; //Move to the next instruction

}

void loadProgram() {
    RAM[0] = LOAD_operation; // load a value into ACC
    RAM[1] = 7;  // Operand for ADD
    RAM[2] = SUB_operation; // SUB
    RAM[3] = 2;  // Operand for SUB
    RAM[4] = ADD_operation;
    RAM[5] = 7;
    RAM[6] = MUL_operation;
    RAM[7] = 2;
    RAM[8] = DIV_operation;
    RAM[9] = 6;
}

int main() {
    srand(time(NULL));
    loadProgram(); // Load the program into memory
    initProcesses(); // Initialize process table
    initCache();
    IVT[0] = timerInterrupt;
    IVT[1] = ioInterrupt;
    IVT[2] = systemCallInterrupt;

    signal(SIGALRM, IVT[0]);
    alarm(1);

    int inputchar;

    while (1) {
	if (interruptFlag) {
		interruptFlag = 0;
		scheduler();
	}

	char inputchar = getchar();

	if (inputchar == 'k') {
		interruptFlag = 1;
		IVT[1]();
	} else if (inputchar == 's') {
		interruptFlag = 1;
		IVT[2]();
	}

        fetch();
        if (IR) {
		bool processesComplete = true;
		for (int i = 0; i < MAX_PROCESSES; i++) {
			if (processTable[i].state != 3) {
				processesComplete = false;
			}
		}

       		if (processesComplete) {
			break;
		}
		decode();
        	execute();
        	processTable[currentProcess].pc = PC;
		processTable[currentProcess].acc = ACC;
		processTable[currentProcess].time -= TIME_SLICE;

		if (processTable[currentProcess].time <= 0) {
			processTable[currentProcess].time = 0;
			processTable[currentProcess].state = 3;
            } 
            else if (processTable[currentProcess].state != 3) {
		    processTable[currentProcess].state = 0;
	    }
        }
        else {
            printf("System: All instructions have been processed\n");
            break;
	}
	scheduler();
 }
 // PROCESSES COMPLETE
 for (int i = 0; i < MAX_PROCESSES; i++) {
	 printf("Process ID: %d, Process PC: %d, Process ACC: %d, Process State: %d, Process Time: %d\n", processTable[i].pid, processTable[i].pc, processTable[i].acc, processTable[i].state, processTable[i].time);
 }
 return 0;
}

