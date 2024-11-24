#ifndef VM_H
#define VM_H

#include <stddef.h>
#include <dirent.h>
#include <stdio.h>

#define STACK_SIZE 124
#define HEAP_SIZE 1024

#define PROGRAM_S_SIZE 30 // ammount of lines is "N/3"
#define PROGRAM_T_SIZE 100

#define VM_TOKEN ", \t\n\0"

#define IS_LABEL(t) (t[strlen(t)-1] == ':')
#define sizeof_arr(ptr) (*(&ptr + 1) - ptr)

/* Virtualized x86_64 machine, this struct shall hold all the information
 * about the hardware state, like the CPU registers, the program text and
 * memory.
*/
struct vm_machine {

	struct cpu {
		int    stack[STACK_SIZE];

		size_t rip; /* Program instruction pointer */

		size_t rbp; /* Stack base pointer */
		size_t rsp; /* Stack pointer */

		int rax;
		int rbx;
		int rcx;
		int rdx;

		int cmp; /* The cpu compare flag */
		int ds; /* Data Section */
	} cpu; /* CPU Registers and Stack */

	struct memory {
		size_t offset;
		void  *heap[HEAP_SIZE]; /* The program vars section */

		char **text; /* The program text section*/
	} memory; /* VM heap memory */

	struct label {
		char *text;
		int  rip;
	} label[50];

};

typedef struct vm_machine vm_t;

extern int verbose_flag;
extern vm_t vm;

int vm_load(char path[1024]); /* Loads to the memory the contents of the program file */
int vm_exec(void); /* Executes the program */
int vm_free(void);

void vm_print(void); /* Used to print debuggin information(Verbose Mode) */
char *vm_p_bin(int n, char buf[8]); /* Print the binnary representation of a int */

#endif /* VM_H */
