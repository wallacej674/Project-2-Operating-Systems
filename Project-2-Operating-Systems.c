#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Instruction Codes
#define ADD_operation 1
#define SUB_operation 2
#define LOAD_operation 3
#define STORE_operation 4

// Registers and Memory Arrays
int RAM[100];
int cacheLevel1[10], cacheLevel2[20];
int cacheTagsl1[10],cacheTagsl2[20];
int memory[] = {333, 122, 120, 215, 205};
int source[] = {1, 2, 3, 4, 5};
int destination[5] = {0};
int DMAsize = sizeof(source) / sizeof(source[0]);
int PC;
int ACC;
int IR;
int savedPC;
int savedACC;
int savedIR;
int interruptFlag = 0; // flag to signal the interrupt

typedef struct {
	int* source;
	int* destination;
	int size;
} parametersDMA;

// void -> void
// interrupt handler
void interruptHandler() {
	savedPC = PC;
	savedACC = ACC;
	savedIR = IR;
	printf("Saving CPU status\n");

	printf("Interrupt is being handled.\n");
	sleep(1);
	printf("Restoring CPU status\n");

	interruptFlag = 0;
	printf("Interrupt handling complete!\n");
}

// int -> void
// Check for Interrupt Signals.
void checkForInterrupt(int signal) {
	interruptFlag = 1;
	if (interruptFlag) {
		interruptHandler();
		alarm(3);
	}
}

// int -> void
// Simulate a DMA transfer
void* dmaTransfer(void* args) {
	parametersDMA* params = (parametersDMA*)args;
	int* source = params->source;
	int* destination = params->destination;
	int size = params->size;

	for (int i=0; i < size; i++) {
		printf("Destination index %d now contains %d\n", i, source[i]);
		destination[i] = source[i]; // move data from source to destination
		sleep(1);
		}
	free(params);
}
// int -> void
// Start DMA Transfer
void initiateDMA(int* source, int* destination, int size, pthread_t* dmaThread) {
	// CPU initiates DMA transfer
	printf("Initiating DMA transfer.\n");

	parametersDMA* params = (parametersDMA*)malloc(sizeof(parametersDMA));
    	params->source = source;
    	params->destination = destination;
    	params->size = DMAsize;

	pthread_create(dmaThread, NULL, dmaTransfer, params);
	
	// CPU can continue with other tasks while transfer happens.
}

// void -> int
// Fetch Instructions from memory
int fetchInstruction() {
	return memory[PC];
}

// int -> int
// Get the address of the instruction
int get_address(int n) {
	int temp = 0;
	int factor = 1;

	while (n / 10 != 0) {
		int m = n % 10;
		n /= 10;
		temp += factor * m;
		factor *= 10;
	}
	return temp;
}

// int -> int
// Get the opcode of the instruction
int get_opcode(int instruction) {
	int first = instruction;
	while (first >= 10) {
		first = first / 10;
	}
	return first;
}

//array -> ()
//initializes the Caches to have values of -1 symbolizing an empty slot and intializes RAM to have the value of the index.
void initialize_arrays(){
for (int i =0; i < 100; i++){
    RAM[i] = i;
}

for (int i = 0; i < 10; i++){
cacheLevel1[i] = -1;
cacheTagsl1[i] = -1;
}

for (int i = 0; i < 20; i++){
    cacheLevel2[i] = -1;
    cacheTagsl2[i] = -1;
}
}

//int -> boolean
//purpose: to check to see if an address exist in the L1 cache
bool inCacheL1(int address){
for(int i = 0; i < 10; i++){
if (cacheTagsl1[i] == address){
    return true;
}
}
return false;
}

//int -> boolean
//purpose: to check to see if an address exist in the L2 cache
bool inCacheL2(int address){
for(int i = 0; i < 20; i++){
    if(cacheTagsl2[i] == address){
        return true;
    }
}
return false;
}

//int -> int
//purpose: to fetch from cacheL1
int getFromCacheL1(int address){
    for (int i = 0; i < 10; i++){
        if (cacheTagsl1[i] == address)
        {
        return cacheLevel1[i];
        }
    }
}

//int -> int
//purpose: to fetch from CacheL2
int getFromCacheL2(int address){
        for (int i = 0; i < 20; i++){
        if (cacheTagsl2[i] == address)
        {
        return cacheLevel2[i];
        }
    }
	return -1;
}

// address, value --> ()
// adds the value and address to the cache memory.
void updateCache(int address, int value) {
	if (inCacheL1(address)) {
		return;
	}
	else if (inCacheL2(address)) {
		return;
	}
	// Space in cache 1
	for (int i = 0; i < 10; i++) {
		if (cacheTagsl1[i] == -1) {
			cacheTagsl1[i] = address;
			cacheLevel1[i] = value;
			printf("Value stored in cache level 1: address %d value: %d\n", address, value);
			return;
		}
	}
	// space in cache 2
	for (int i = 0; i < 20; i++) {
		if (cacheTagsl2[i] == -1) {
			cacheTagsl2[i] = address;
			cacheLevel2[i] = value;
			printf("value stored in cache level 2: address: %d value: %d \n", address, value);
			return;
		}
	}
	// if no space then this should occur
	printf("No space in either cache level, removing values to make space...\n");
	int temp_address = cacheTagsl1[0];
	int temp_value = cacheLevel1[0];


	//this will bump the values in l1 up 1 slot
	for(int i = 1; i < 10; i++){
		int bump_value = cacheLevel1[i];
		int bump_address = cacheTagsl1[i];
		

		cacheTagsl1[i - 1] = bump_address;
		cacheLevel1[i - 1] = bump_value;
		
	}
	//removes these elements from the queue
	cacheLevel1[9] = -1;
	cacheTagsl1[9] = -1;
	//bumps all the elements in l2
	for (int i = 1; i < 20; i++){
		int bump_value = cacheLevel1[i];
		int bump_address = cacheTagsl1[i];
		

		cacheTagsl1[i - 1] = bump_address;
		cacheLevel1[i - 1] = bump_value;
	}
	// moves this element to the end of l2
	cacheLevel2[19] = temp_value;
	cacheTagsl2[19] = temp_address;

	//adds the new value to cache L1
	cacheTagsl1[9] = address;
	cacheLevel1[9] = value;
	printf("value stored in cache level 1: address: %d value: %d \n", address, value);
}

// int int --> void
//Write to memory
void writeMemory(int address, int value) {
    RAM[address] = value;

    // Check if the address exists in L1 or L2 and update
    if (inCacheL1(address)) {
        for (int i = 0; i < 10; i++) {
            if (cacheTagsl1[i] == address) {
                cacheLevel1[i] = value; // Update L1 cache
            }
        }
    } else if (inCacheL2(address)) {
        for (int i = 0; i < 20; i++) {
            if (cacheTagsl2[i] == address) {
                cacheLevel2[i] = value; // Update L2 cache
            }
        }
    }
    updateCache(address, value);
}

// int --> int
//read from memory
int readMemory(int address) {
 if (inCacheL1(address)) {
return getFromCacheL1(address); //Cache hit – if in cache
 }
else if(inCacheL2(address)) {
return getFromCacheL2(address);
 }
else{
	updateCache(address, RAM[address]);
return RAM[address]; //cache miss – if not, get from RAM
}
}

// execute based on operation code
void executeInstruction(int instruction) {
	int opcode = get_opcode(instruction);
	int address = get_address(instruction);
	int prev;

	switch(opcode) {
		case ADD_operation:
			// perform addition
			prev = ACC;
			ACC = prev + readMemory(address);
			printf("Adding: %d + %d = %d\n", prev, readMemory(address), ACC);
			break;
		case SUB_operation:
			// perform substraction
			prev = ACC;
			ACC = prev - readMemory(address);
			printf("Subtracting: %d - %d = %d\n", prev, readMemory(address), ACC);
			break;
		case LOAD_operation:
			// perform loading
			ACC = readMemory(address);
			printf("Loading data in ACC: %d -> %d\n", readMemory(address), ACC);
			break;
		case STORE_operation:
			// perform storing
			writeMemory(address, ACC);
			printf("Storing data into memory from ACC: %d -> %d\n", ACC, readMemory(address));
			break;
		default:
			// Handle undefined/invalid opcodes
			printf("Invalid opcode given. Please try again.\n");
			break;
		}
}

int main() {
	initialize_arrays();
	PC = 0;

	signal(SIGALRM, checkForInterrupt);
	alarm(1);

	pthread_t dmaThread;
	initiateDMA(source, destination, DMAsize, &dmaThread);

	while (PC < sizeof(memory) / sizeof(memory[0])) {
		if (!interruptFlag) {
			IR = fetchInstruction();
			PC++;
			executeInstruction(IR);
			sleep(1);
		}
	}

	pthread_join(dmaThread, NULL);
	printf("DMA Completed.\n");
	return 0;
}
