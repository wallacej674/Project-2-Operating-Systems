#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// the global vairables and registers.
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
            if(operand != 0){
                prev = ACC;
                result = ACC - operand;
                ACC = result;
                printf("Dividing: %d * %d = %d\n", prev, operand, ACC);
                break;
            }
            else{
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
            }
            else{
                printf("Error: Invalid Jump attempt. Out of Bounds Error\n");
                break;
            }
        case JZ_operation:
            if(zeroFlag == 0){
                if(operand >= 0 && operand < 256){
                    prev = PC;
                    PC = operand - 1;
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
 memory[0] = 1; //ADD instruction
 memory[1] = 5; //Operand for ADD
 //Add more instructions as needed
}

int main() {
 loadProgram(); //Load the program into memory
 while (1) {
 fetch();
 if(IR){
 decode();
 execute();
 }
 else{
    printf("System: All instructions have been processed\n");
    break;
 }
 //Add logic to break when program completes or an error occurs
 }
 return 0;
}
