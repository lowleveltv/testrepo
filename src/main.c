#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database_file>\n", argv[0]);
    printf("Options:\n");
    printf("\t -n          Create a new file\n");
    printf("\t -f <file>   Specify the file path\n");
	printf("\t -a <string> Add a new employee with the given string format 'name,address,hours'\n");
	printf("\t -l          List all employees in the database\n");
	printf("\t -d <name>    Delete employee with the given name\n");
    printf("\t -e <string>  Edit employee with format 'name,new_address,new_hours'\n");
    exit(0);
}

int main(int argc, char *argv[]) { 
	char *filepath = NULL;
	char *addstring = NULL;
	char *deletestring = NULL;
	char *editstring = NULL;
	char *portarg = NULL;
	unsigned short port = 0;
	bool newfile = false;
	bool list = false;
	int c;

	int dbfd = -1;
	struct dbheader_t *dbhdr = NULL;
	struct employee_t *employees = NULL;

	while ((c = getopt(argc, argv, "nf:a:ld:e:")) != -1) {
		switch (c) {
			case 'n':
				newfile = true;
				break;
			case 'f':
				filepath = optarg;
				break;
			case 'a':
				addstring = optarg;
				break;
			case 'l':
				list = true;
				break;
			case 'd':
				deletestring = optarg;
				break;
			case 'e':
				editstring = optarg;
				break;
			case '?':
				printf("Unknown option -%c\n", c);
				break;
			default:
				return -1;

		}
	}

    if (filepath == NULL) {
        fprintf(stderr, "Error: File path must be specified with -f option.\n");
        print_usage(argv);
    }

    if (newfile) {
		dbfd = create_db_file(filepath);
		if (dbfd == STATUS_ERROR) {
			fprintf(stderr, "Error creating database file: %s\n", filepath);
			return -1;
		}

		if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
			fprintf(stderr, "Error creating database header in file: %s\n", filepath);
			close(dbfd);
			return -1;
		}
	} else {
		dbfd = open_db_file(filepath);
		if (dbfd == STATUS_ERROR) {
			fprintf(stderr, "Error opening database file: %s\n", filepath);
			return -1;
		}

		if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
			fprintf(stderr, "Error validating database header in file: %s\n", filepath);
			close(dbfd);
			return -1;
		}
	}

	if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
		fprintf(stderr, "Error reading employees from database file: %s\n", filepath);
		close(dbfd);
		return -1;
	}

	if (addstring) {
		dbhdr->count++;
		employees = realloc(employees, dbhdr->count * sizeof(struct employee_t));
		if (employees == NULL) {
			fprintf(stderr, "Error reallocating memory for employees.\n");
			close(dbfd);
			return -1;
		}

		add_employee(dbhdr, employees, addstring);
	}

	if (list) {
		list_employees(dbhdr, employees);
	}

	if (deletestring) {
		if (delete_employee(dbhdr, &employees, deletestring) != STATUS_SUCCESS) {
			fprintf(stderr, "Error deleting employee: %s\n", deletestring);
			close(dbfd);
			return -1;
		}
	}

	if (editstring) {
		if (edit_employee(dbhdr, employees, editstring) != STATUS_SUCCESS) {
			fprintf(stderr, "Error edit employee: %s\n", editstring);
			close(dbfd);
			return -1;
		}
	}

	output_file(dbfd, dbhdr, employees);

    return 0;
}

