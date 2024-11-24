#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "exec.h"
#include "vm.h"

vm_t vm;

/* This structure is used to access the registers values when the program instruction require it */
#define CPU_REG_N 5
struct {
	char *name;
	int *reg;
} cpu_reg[] = {
	{ "$RAX", &vm.cpu.rax }, { "$RBX", &vm.cpu.rbx },
	{ "$RCX", &vm.cpu.rcx }, { "$RDX", &vm.cpu.rdx },
	{ "$DS", &vm.cpu.ds },	 { NULL, NULL },
};

struct inst_set_set inst_set[] = {
	/* Memory */
	{ "MOVE", 2, &exec_move },
	{ "PUSH", 1, &exec_push },
	{ "POP", 1, &exec_pop },
	{ "PUSHA", 0, &exec_pusha },
	{ "POPA", 0, &exec_popa },

	/* Math */
	{ "ADD", 2, &exec_math },
	{ "SUB", 2, &exec_math },
	{ "MUL", 2, &exec_math },
	{ "DIV", 2, &exec_math },
	{ "INC", 1, &exec_inc },
	{ "DEC", 1, &exec_dec },

	/* Logic */
	{ "CMP", 2, &exec_cmp },
	{ "JMP", 1, &exec_jmp_prefix },
	{ "JEQ", 1, &exec_jmp_prefix },
	{ "JNE", 1, &exec_jmp_prefix },
	{ "JLT", 1, &exec_jmp_prefix },
	{ "JLE", 1, &exec_jmp_prefix },
	{ "JGT", 1, &exec_jmp_prefix },
	{ "JGE", 1, &exec_jmp_prefix },
	{ "CALL", 1, &exec_call },
	{ "RET", 0, &exec_ret },

	/* IO */
	{ "PRINTD", 1, &exec_print },
	{ "PRINTC", 1, &exec_print },
	{ "PRINTS", 1, &exec_print },

	{ "OPEN", 2, &exec_open },
	{ "READ", 2, &exec_read },
	{ "WRITE", 2, &exec_write },
	{ "CLOSE", 1, &exec_close },
	{ NULL, 0, NULL },
};

/* Checks if the argv is a register or a var, and return it's adress*/
static int *get_addrs(char *argv)
{
	switch (argv[0]) {
	case '$':
		for (int i = 0; cpu_reg[i].name; ++i) {
			if (strcmp(cpu_reg[i].name, argv) == 0) {
				return cpu_reg[i].reg;
			}
		}

		return NULL;
	case '#':

		for (int i = 0; i < vm.memory.offset; i += 2) {
			if (strcmp(argv, vm.memory.heap[i]) == 0)
				return vm.memory.heap[i + 1];
		}

		vm.memory.heap[vm.memory.offset++] = strdup(argv);
		vm.memory.heap[vm.memory.offset] = malloc(sizeof(int));

		return vm.memory.heap[vm.memory.offset++];
	}

	return NULL;
}

static int get_data(char *argv)
{
	if (argv[0] == '$' || argv[0] == '#')
		return *get_addrs(argv);

	for (int i = 0; vm.label[i].text != NULL; ++i) {
		if (strcmp(vm.label[i].text, argv) == 0)
			return vm.label[i].rip;
	}

	return atoi(argv);
}

// Data Functions
void exec_move(char **argv)
{
	int *a = get_addrs(argv[1]);
	int b = get_data(argv[2]);

	if (!a)
		return;

	*a = b;
}

void exec_push(char **argv)
{
	int a = get_data(argv[1]);
	memcpy(&vm.cpu.stack[vm.cpu.rsp++], &a, sizeof(int));
}

void exec_pop(char **argv)
{
	int *a = get_addrs(argv[1]);

	if (!a)
		return;

	/* RSP points to where the next value will be pushed,
	   not the value it is in, so we need to decrement first */
	memcpy(a, &vm.cpu.stack[--vm.cpu.rsp], sizeof(int));
}

void exec_pusha(char **argv)
{
	for (int i = 0; i < CPU_REG_N; ++i)
		vm.cpu.stack[vm.cpu.rsp++] = *cpu_reg[i].reg;
}

void exec_popa(char **argv)
{
	for (int i = CPU_REG_N - 1; i >= 0; --i)
		*cpu_reg[i].reg = vm.cpu.stack[--vm.cpu.rsp];
}

// Mathematical Functions

void exec_math(char **argv)
{
	int *reg = get_addrs(argv[1]);
	switch (argv[0][0]) {
	case 'A': /* ADD */
		*reg += get_data(argv[2]);
		break;
	case 'S': /* SUB */
		*reg -= get_data(argv[2]);
		break;
	case 'D': /* Divide */
		*reg /= get_data(argv[2]);
		break;
	case 'M': /* Multply */
		*reg *= get_data(argv[2]);
		break;
	}
}

void exec_inc(char **argv)
{
	int *reg = get_addrs(argv[1]);
	(*reg)++;
}

void exec_dec(char **argv)
{
	int *reg = get_addrs(argv[1]);
	*reg -= 1;
}

// Logical Functions
void exec_cmp(char **argv)
{
	int a = get_data(argv[2]);
	int b = get_data(argv[1]);

	vm.cpu.cmp = a - b;
}

void exec_jmp_prefix(char **argv)
{
	if (strncmp(argv[0], "JMP", 3) == 0)
		vm.cpu.rip = get_data(argv[1]) - 1;

	else if (vm.cpu.cmp == 0 && (strcmp(argv[0], "JEQ") == 0))
		vm.cpu.rip = get_data(argv[1]) - 1;

	else if (vm.cpu.cmp != 0 && (strcmp(argv[0], "JNE") == 0))
		vm.cpu.rip = get_data(argv[1]) - 1;

	else if (vm.cpu.cmp < 0 && (strcmp(argv[0], "JLT") == 0))
		vm.cpu.rip = get_data(argv[1]) - 1;

	else if (vm.cpu.cmp > 0 && (strcmp(argv[0], "JGT") == 0))
		vm.cpu.rip = get_data(argv[1]) - 1;

	else if (vm.cpu.cmp >= 1 && (strcmp(argv[0], "JGE") == 0))
		vm.cpu.rip = get_data(argv[1]) - 1;

	else if (vm.cpu.cmp <= -1 && (strcmp(argv[0], "JLE") == 0))
		vm.cpu.rip = get_data(argv[1]) - 1;

	vm.cpu.cmp = 0; /* Reseting the compare flag */
}

void exec_call(char **argv)
{
	vm.cpu.ds = vm.cpu.rip + 1;
	vm.cpu.rip = get_data(argv[1]) - 1;
}

void exec_ret(char **argv)
{
	vm.cpu.rip = vm.cpu.ds;
}

void exec_print(char **argv)
{
	switch (argv[0][5]) {
	case 'D':
		printf("%d", get_data(argv[1]));
		break;
	case 'C':
		printf("%c", get_data(argv[1]));
		break;
	case 'S':
		printf("%s", argv[1]);
	}
}

void exec_open(char **argv)
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	int *var = get_addrs(argv[1]);
	*var = open(argv[2], O_RDWR | O_CREAT | O_APPEND, mode);
}

void exec_read(char **argv)
{
	int *data = get_addrs(argv[2]);
	if(read(get_data(argv[1]), data, sizeof(char)) == 0)
		*data = 0;
}

void exec_write(char **argv)
{
	void *data;

	if (argv[2][0] == '$' || argv[2][0] == '#') {
		data = get_addrs(argv[2]);
		write(get_data(argv[1]), data, sizeof(char));
	}

	else {
		data = strdup(argv[2]);
		write(get_data(argv[1]), data, strlen(data));
		free(data);
	}
}

void exec_close(char **argv)
{
	close(get_data(argv[1]));
}
