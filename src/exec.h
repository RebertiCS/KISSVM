#ifndef EXEC_H
#define EXEC_H

/* The CPU instruction set structure */
struct inst_set_set {
	char *str; /* The name of the instruction itself */
	int argc; /* How many arguments it takes */

	/* A function pointer that holds the code of the instruction code to be executed*/
	void (*exec)(char **);
};

extern struct inst_set_set inst_set[];

void exec_move(char **argv);
void exec_push(char **argv);
void exec_pop(char **argv);
void exec_pusha(char **argv);
void exec_popa(char **argv);

void exec_math(char **argv); /* Basic Math is done here */

void exec_inc(char **argv);
void exec_dec(char **argv);

void exec_cmp(char **argv); /* Sets the compare register */
void exec_jmp_prefix(char **argv);
void exec_call(char **argv);
void exec_ret(char **argv);

void exec_print(char **argv);

void exec_open(char **argv);
void exec_read(char **argv);
void exec_write(char **argv);
void exec_close(char **argv);
#endif /* EXEC_H */
