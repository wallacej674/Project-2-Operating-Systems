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
int PC = 0; // Program Counter
int ACC = 0; // Accumulator
int IR = 0; // Instruction Register
int memory[256]; // Memory (instructions + data)
// Opcode and Operand
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

// Status flags
int interruptFlag = 0; // flag to signal the interrupt
int carryFlag;
int zeroFlag;
int overFlowFlag;

void initProcesses() {
	for (int i = 0; i < MAX_PROCESSES; i++) {
		processTable[i].pid = 100 + (i * 100);
		processTable[i].pc = 0;
		processTable[i].acc = 0;
		processTable[i].state = 0; // ready state
		processTable[i].time = rand() % 15;
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

void scheduler() {
	// implement round-robin scheduling
	// use a loop to cycle through processes and manage their state
	printf("All Process Data!\n");
	for (int i = 0; i < MAX_PROCESSES; i++) {
		printf("Process ID: %d, Process PC: %d, Process ACC: %d, Process State: %d, Process Time: %d\n", processTable[i].pid, processTable[i].pc, processTable[i].acc, processTable[i].state, processTable[i].time);
	}
	int nextProcess = (currentProcess + 1) % MAX_PROCESSES;
	while (processTable[nextProcess].state == 3) {
		nextProcess = (nextProcess + 1) % MAX_PROCESSES;
		if (nextProcess == currentProcess) {
			printf("All processes complete.");
			return;
		}
	}
	if (processTable[nextProcess].state == 0) {
		contextSwitch(currentProcess, nextProcess);
		currentProcess = nextProcess;
	}
}

void fetch() {
	 IR = memory[PC]; //Fetch the next instruction
}
void decode() {
	//Decode the instruction in IR
	//Implement logic to extract the opcode and operands
	opcode = IR;
	operand = memory[PC + 1];
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
			if (operand != 0){
				prev = ACC;
				result = ACC - operand;
				ACC = result;
				printf("Dividing: %d * %d = %d\n", prev, operand, ACC);
				break; 
			} else {
				printf("Division Error: Division by zero\n");
				break;
			}
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
				PC = operand - 1;
				printf("Jump: PC has been changed from %d to %d\n", prev, PC);
				break;
			} else {
				printf("Error: Invalid Jump attempt. Out of Bounds Error\n");
				break;
			}
		case JZ_operation:
			if (zeroFlag == 0) {
				if (operand >= 0 && operand < 256) {
					prev = PC;
					PC = operand - 1;
					printf("Jump Zero: PC has been changed from %d to %d\n", prev, PC);
					break;
				} else {
					printf("Error: Invalid Jump attempt. Out of Bounds Error\n");
					break;
				}
			} else {
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
	 memory[0] = 1; //ADD instruction
	 memory[1] = 5; //Operand for ADD
	 memory[2] = 1;
	 memory[3] = 9;
			//Add more instructions as needed
}

int main() {
	srand(time(NULL));
	loadProgram(); // Load the program into memory
	initProcesses(); // Initialize process table
	while (1) {
		scheduler();
		fetch();
		if (IR) {
			decode();
			execute();
			processTable[currentProcess].pc = PC;
			processTable[currentProcess].acc = ACC;
			processTable[currentProcess].time -= TIME_SLICE;

			if (processTable[currentProcess].time <= 0) {
				processTable[currentProcess].time = 0;
				processTable[currentProcess].state = 3;
			} else if (processTable[currentProcess].state != 3) {
				processTable[currentProcess].state = 0;
			}
		} else {
			printf("System: All instructions have been processed\n");
			break;
		}
	}
	printf("FINAL CHECK:\n");
	for (int i = 0; i < MAX_PROCESSES; i++) {
		printf("Process ID: %d, Process PC: %d, Process ACC: %d, Process State: %d, Process Time: %d\n", processTable[i].pid, processTable[i].pc, processTable[i].acc, processTable[i].state, processTable[i].time);
	}
	return 0;
}
