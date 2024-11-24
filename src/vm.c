#include <alloca.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "exec.h"
#include "vm.h"

int verbose_flag = 0;

static char *
vm_read(char path[PATH_MAX]); /* Read the program files contents in to memory */

/* (RE)Allocates memory for the vm text section.
   - __SIZE__ is the final size of the array.
   - __N__ is the initial position from which we allocate memory for the array elements.
   - __RETURNS:__ the new size of the vm text section.
*/
static int vm_realloc(size_t size, size_t n);

static char *vm_read(char path[PATH_MAX])
{
	char *result = NULL;
	/* Reading the file size */
	FILE *f = fopen(path, "rb");
	if (!f) {
		perror("ASVM");
		exit(-1);
	}

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET); /* same as rewind(f); */

	result = calloc(fsize + 1, 1);
	fread(result, 1, fsize, f);
	fclose(f);

	return result;
}

int vm_load(char path[PATH_MAX])
{
	char *buf = vm_read(path);
	size_t allocd = vm_realloc(PROGRAM_S_SIZE, 0);

	/* This is used to access the values of the program text adding to the
	 * index the number of the var that you want to access. For example when
	 * line "MOVE $RAX, 10" is parsed, you will access it using "index+N", n 
	 * "N" being the asset wanted from this line.
	 */
	vm.cpu.rip = 0;
	size_t label_offset = 0;

	// Remove Comments
	char *cmt_start;
	while((cmt_start = strchr(buf, ';'))) {
		char *cmt_end = strchr(cmt_start+1, ';');

		for(int i = 0; i <= cmt_end-cmt_start; ++i)
			cmt_start[i] = ' ';
	}

	for (char *cmd = strtok(buf, VM_TOKEN); cmd;
	     cmd = strtok(NULL, VM_TOKEN), vm.cpu.rip++) {
		if (IS_LABEL(cmd)) {
			vm.label[label_offset] = (struct label){
				.text = strndup(cmd, strlen(cmd) - 1),
				.rip = vm.cpu.rip - 1,
			};

			vm.cpu.rip--;
			label_offset++;

			vm.label[label_offset + 1].text = NULL;
		} else
			strcpy(vm.memory.text[vm.cpu.rip], cmd);

		if (vm.cpu.rip >= allocd - 1)
			allocd = vm_realloc(allocd + PROGRAM_S_SIZE, allocd);
	}

	vm_realloc(vm.cpu.rip, allocd);
	free(buf);

	return 0;
}

int vm_exec(void)
{
	int count = vm.cpu.rip;
	vm.cpu.rip = 0;

	while (vm.cpu.rip < count) {
		// This vars are used for error handling and verbose output
		char **ptr = NULL;
		int i;

		/* Where the program is executed */
		for (i = 0; inst_set[i].str; ++i) {
			if (strcmp(vm.memory.text[vm.cpu.rip],
				   inst_set[i].str) == 0) {
				ptr = vm.memory.text + vm.cpu.rip;

				inst_set[i].exec(&vm.memory.text[vm.cpu.rip]);
				vm.cpu.rip += inst_set[i].argc + 1;

				break;
			}
		}

		if (!ptr) {
			fprintf(stderr, "ASVM: Syntax Error\n");
			return -1;
		}

		else if (verbose_flag == 1) {
			fprintf(stderr, "%.3ld->%s:", vm.cpu.rip, ptr[0]);

			if (inst_set[i].argc > 0)
				fprintf(stderr, "\t%s", ptr[1]);

			if (inst_set[i].argc > 1)
				fprintf(stderr, ", %s", ptr[2]);

			fprintf(stderr, "\n");
		}
	}


	return 0;
}

int vm_realloc(size_t size, size_t n)
{
	if (sizeof_arr(vm.memory.text) > size + 1) {
		for (int i = size; i < n; ++i)
			free(vm.memory.text[i]);
	}

	vm.memory.text = realloc(vm.memory.text, (size * sizeof(char **)));

	if (vm.memory.text == NULL)
		fprintf(stderr, "ASVM: Failed to allocate memory\n");

	for (int i = n; i < size; i++)
		vm.memory.text[i] = malloc(PROGRAM_T_SIZE);

	return size;
}

int vm_free(void)
{
	for (int i = 0; i < vm.cpu.rip; i++)
		free(vm.memory.text[i]);

	free(vm.memory.text);

	for (int i = 0; vm.label[i].text != NULL; i++)
		free(vm.label[i].text);

	for (int i = 0; i < vm.memory.offset; i++)
		free(vm.memory.heap[i]);

	return 0;
}

void vm_print(void)
{
	fprintf(stderr,
		"CPU:\n"
		"\tRAX: %d\n"
		"\tRBX: %d\n"
		"\tRCX: %d\n"
		"\tRDX: %d\n"
		"\tRBP: %ld\n"
		"\tRSP: %ld\n"
		"\tRIP: %ld\n"
		"\tDS: %d\n",
		vm.cpu.rax, vm.cpu.rbx, vm.cpu.rcx, vm.cpu.rcx, vm.cpu.rbp,
		vm.cpu.rsp, vm.cpu.rip, vm.cpu.ds);

	if (vm.label[0].text) {
		int i = -1;

		fprintf(stderr, "Labels:\n");
		while (vm.label[++i].text)
			fprintf(stderr, "\t%s: %d\n", vm.label[i].text,
				vm.label[i].rip);
	}

	if (vm.cpu.rsp > 0) {
		fprintf(stderr,
			"+-------------------------------------------+\n"
			"|		   Stack		    |\n"
			"+-------------------------------------------+\n"
			"| Address |  Int  |  Hex   | Char |   Bin   |\n"
			"+-------------------------------------------+\n");

		char buf[9];

		for (int i = 0; i < vm.cpu.rsp; ++i)
			fprintf(stderr,
				"| 0x%.4x  | %.5d | 0x%.4x |  %c	  | %s |\n",
				i, vm.cpu.stack[i], vm.cpu.stack[i],
				vm.cpu.stack[i],
				vm_p_bin(vm.cpu.stack[i], buf));
		fprintf(stderr,
			"+-------------------------------------------+\n");
	}

	if (vm.memory.offset > 0) {
		fprintf(stderr,
			"\n+-----------------------------------------------------+\n"
			"|		         Heap	               	      |\n"
			"+-----------------------------------------------------+\n"
			"|  Name  | Address  |  Int  |  Hex   | Char |   Bin   |\n"
			"+-----------------------------------------------------+\n");

		char buf[9];

		for (int i = 0; i < vm.memory.offset; i += 2)
			fprintf(stderr,
				"| %s\t |  0x%.4x  | %.5d | 0x%.4x |  %c   | %s |\n",
				(char *)vm.memory.heap[i], i,
				*(int *)vm.memory.heap[i + 1],
				*(int *)vm.memory.heap[i + 1],
				*(int *)vm.memory.heap[i + 1],
				vm_p_bin(*(int *)vm.memory.heap[i + 1], buf));
		fprintf(stderr,
			"+-----------------------------------------------------+\n");
	}
}

char *vm_p_bin(int n, char buf[8])
{
	strncpy(buf, "0000000", 8);

	if (n <= 0)
		return buf;

	int i = 0;

	while (n) {
		if (i > 8)
			break;
		if (n & 1)
			buf[i++] = '1';
		else
			buf[i++] = '0';

		n >>= 1;
	}
	buf[7] = '\0';

	return buf;
}
