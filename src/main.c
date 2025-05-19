#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
	printf( "Usage: %s -f database file [-n] [-s name] [-a employee_data] [-i employee_id -d -h hours]\n", argv[0] );
	printf("\t -f - (Required) Path to database file\n");
	printf("\t -n - Create new database\n");
	printf("\t -l - List all employees\n");
	printf("\t -s - Search for employees by name\n");
	printf("\t -a - Add new employee to database\n");
	printf("\t -i - Employee ID to update or delete\n");
	printf("\t -d - Delete employee\n");
	printf("\t -h - Update Employee hours\n");
	return;
}

int main(int argc, char *argv[]) {
	char *filepath  = NULL;
	char *addstr    = NULL;
	char *searchstr = NULL;
	bool newfile    = false;
	bool list       = false;
	bool delete		= false;
	int emp_id      = -1;
	int hours       = -1;

	int db_fd = -1;
	struct dbheader_t *db_head   = NULL;
	struct employee_t *employees = NULL;

	int c;
	opterr = 0;
	while ( ( c = getopt(argc, argv, "nldf:a:s:i:h:" ) ) != -1 ){
		switch(c){
			case 'n':
				newfile = true;
				break;
			case 'f':
				filepath = optarg;
				break;
			case 'a':
				addstr = optarg;
				break;
			case 'l':
				list = true;
				break;
			case 's':
				searchstr = optarg;
				break;
			case 'i':
				emp_id = atoi(optarg);
				break;
			case 'd':
				delete = true;
				break;
			case 'h':
				hours = atoi(optarg);
				break;
			case '?':
				fprintf( stderr, "Unknown option -%c\n", optopt );
				break;
			default:
				return STATUS_ERROR;
		}
	}

	if ( NULL == filepath ){
		fprintf( stderr, "Database file is a required argument\n");
		print_usage(argv);
		return STATUS_ERROR;
	}

	if ( newfile ){
		db_fd = create_db_file(filepath);
		if ( STATUS_ERROR == db_fd ){
			fprintf( stderr, "Unable to create new database at %s\n", filepath );
			return STATUS_ERROR;
		}
		if ( STATUS_ERROR == create_db_header( &db_head ) ){
			fprintf( stderr, "Unable to initilise database\n" );
			close_db_file(db_fd);
			clean_up( &db_head, &employees );
			return STATUS_ERROR;
		}
	} else {
		db_fd = open_db_file(filepath);
		if ( STATUS_ERROR == db_fd ){
			fprintf( stderr, "Unable to load database at %s\n", filepath );
			return STATUS_ERROR;
		}

		if ( STATUS_ERROR == validate_db_header( db_fd, &db_head) ){
			fprintf( stderr, "Failed to validate database\n" );
			close_db_file(db_fd);
			clean_up( &db_head, &employees );
			return STATUS_ERROR;
		}
	}

	if ( STATUS_ERROR == read_employees( db_fd, db_head, &employees ) ){
		fprintf( stderr, "Failed to load employee entries\n" );
		close_db_file(db_fd);
		clean_up( &db_head, &employees );
		return STATUS_ERROR;
	}

	if ( NULL != addstr ){
		add_employee( db_head, &employees, addstr );
	}

	if ( list ){
		list_employees( db_head, employees );
	}

	if ( NULL != searchstr ){
		search_employee( db_head, employees, searchstr );
	}

	if ( delete ){
		if ( emp_id < 0 ){
			fprintf( stderr, "Require employee ID to delete\n" );
			close_db_file(db_fd);
			clean_up( &db_head, &employees );
			return STATUS_ERROR;
		} else if ( emp_id >= db_head->count ) {
			fprintf( stderr, "Invalid employee ID\n" );
			close_db_file(db_fd);
			clean_up( &db_head, &employees );
			return STATUS_ERROR;
		}
		delete_employee( db_head, &employees, emp_id );
	}

	if ( hours >= 0 ){
		if ( emp_id < 0 ){
			fprintf( stderr, "Require employee ID to update\n" );
			close_db_file(db_fd);
			clean_up( &db_head, &employees );
			return STATUS_ERROR;
		} else if ( emp_id >= db_head->count ) {
			fprintf( stderr, "Invalid employee ID\n" );
			close_db_file(db_fd);
			clean_up( &db_head, &employees );
			return STATUS_ERROR;
		}
		update_hours( employees,  emp_id,  hours );
	}

	// Write changes if any to disk
	if ( newfile || delete || hours >= 0 || NULL != addstr ){
		output_file( db_fd, db_head, employees );
	}

	close_db_file(db_fd);
	clean_up( &db_head, &employees );
	return STATUS_SUCCESS;
}
