#include <getopt.h>
#include <string.h>
#include <stdio.h>

#include "vm.h"

#define version "v1.0"

char program_file[1024];

int main(int argc, char *argv[])
{
	int choice;
	char *file = NULL;

	while (1) {
		static struct option long_options[] = {
			{ "verbose", no_argument, 0, 'V' },
			{ "version", no_argument, 0, 'v' },
			{ "help", no_argument, 0, 'h' },
			{ "file", required_argument, 0, 'f' },

			{ 0, 0, 0, 0 }
		};

		int option_index = 0;

		choice = getopt_long(argc, argv, "Vvhf:", long_options,
				     &option_index);

		if (choice == -1)
			break;

		switch (choice) {
		case 'v':
			verbose_flag = 1;
			break;

		case 'V':
			printf("ASVM %s\n", version);
			return 0;

		case 'h':
			printf("Usage: asvm [-V] -f <file_name>\n"
			       "  -f, --file <file_name> \t\tselects the file to be parsed.\n"
			       "  -v, --verbose, \t\t\tverbose mode.\n"
			       "  -V, --version, \t\t\tshows the version.\n"
			       "  -h, --help, \t\t\t\tshows this message.\n");

			return 0;

		case 'f':
			file = argv[optind - 1];
			break;
		}
	}

	if(!file) {
		fprintf(stderr, "ASVM: A file is needed as argument.\n");
		return -1;
	}
		
	vm_load(file);
	vm_exec();

	if (verbose_flag == 1)
		vm_print();

	vm_free();

	return 0;
}
