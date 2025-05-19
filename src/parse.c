#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_employees( struct dbheader_t *head, struct employee_t employees[] ){
	
	if ( NULL == head ){
		fprintf( stderr, "Invalid database header\n" );
		return;
	}

	if ( NULL == employees ){
		fprintf( stderr, "Invalid employee data\n" );
		return;
	}

	for ( int i = 0; i < head->count; i++ ){
		printf("Employee: %d\n\tName: %s\n\tAddress: %s\n\tHours: %d\n", i, employees[i].name, employees[i].address, employees[i].hours );
	}
}

int search_employee( struct dbheader_t *head, struct employee_t employees[], char *searchstring ){
	bool found = false;

	for( int i = 0; i < head->count; i++ ){
		if ( NULL != strcasestr( employees[i].name, searchstring ) ){
			found = true;
			printf("Employee: %d\n\tName: %s\n\tAddress: %s\n\tHours: %d\n", i, employees[i].name, employees[i].address, employees[i].hours );
		}
	}

	if ( found ){
		return STATUS_SUCCESS;
	} else {
		fprintf( stderr, "%s not found\n", searchstring );
		return STATUS_ERROR;
	}
}

int add_employee( struct dbheader_t *head, struct employee_t **employeesPtr, char *addstring ){
	
	if ( NULL == head ){
		fprintf( stderr, "Invalid database header\n" );
		return STATUS_ERROR;
	}

	if ( head->count >= 100 ){
		fprintf( stderr, "Maximum number of employees reached\n" );
		return STATUS_ERROR;
	}

	if ( NULL == addstring ){
		fprintf( stderr, "Invalid employee data\n" );
		return STATUS_ERROR;
	}

	if ( NULL == employeesPtr ){
		fprintf( stderr, "Invalid employee data\n" );
		return STATUS_ERROR;
	}

	if ( NULL == *employeesPtr){
		// First employee - allocate new memory
		*employeesPtr = calloc( 1, sizeof(struct employee_t) );
	} 

	char *name = strtok( addstring, "," );
	if ( NULL == name ){
		fprintf( stderr, "Invalid employee data\n" );
		return STATUS_ERROR;
	}
	
	char *address = strtok( NULL, "," );
	if ( NULL == address ){
		fprintf( stderr, "Invalid employee data\n" );
		return STATUS_ERROR;
	}

	char *hours = strtok( NULL, "," );
	if ( NULL == hours ){
		fprintf( stderr, "Invalid employee data\n" );
		return STATUS_ERROR;
	}	
	
	struct employee_t *employees = *employeesPtr;

	strncpy( employees[head->count].name, name, sizeof(employees[head->count].name) );
	strncpy( employees[head->count].address, address, sizeof(employees[head->count].address) );
	employees[head->count].hours = atoi(hours);

	head->count++;
	*employeesPtr = employees;

	return STATUS_SUCCESS;
}

int delete_employee( struct dbheader_t *head, struct employee_t *employeesPtr[], int emp_id ){

	for ( int i = emp_id + 1; i < head->count; i++ ){
		(*employeesPtr)[i-1] = (*employeesPtr)[i];
	}

	*employeesPtr = reallocarray( *employeesPtr, head->count - 1, sizeof(struct employee_t) );
	if ( NULL == *employeesPtr && head->count > 1 ) {
		fprintf( stderr, "Failed to reallocate memory for employee entries\n" );
		return STATUS_ERROR;
	}

	head->count--;
	return STATUS_SUCCESS;
}

void update_hours( struct employee_t employees[], int emp_id, int hours ){
	employees[emp_id].hours = (unsigned int) hours;
}

int read_employees( int fd, struct dbheader_t *head, struct employee_t *employeesPtr[] ){

	if ( NULL == head ){
		fprintf( stderr, "Invalid database header\n" );
		return STATUS_ERROR;
	}

	if ( NULL == employeesPtr ){
		fprintf( stderr, "Invalid employee data\n" );
		return STATUS_ERROR;
	}

	if ( NULL == fd ){
		fprintf( stderr, "Invalid file descriptor\n" );
		return STATUS_ERROR;
	}

	if ( NULL == *employeesPtr ){
		fprintf( stderr, "Invalid employee data\n" );
		return STATUS_ERROR;
	}

	int count = head->count;

	if ( count > 100 ){
		fprintf( stderr, "Maximum number of employees reached\n" );
		return STATUS_ERROR;
	}

	struct employee_t *employees = calloc( count, sizeof(struct employee_t) );
	if ( NULL == employees ){
		fprintf( stderr, "Failed to allocate memory for employee entries\n" );
		return STATUS_ERROR;
	}

	read( fd, employees, count * sizeof(struct employee_t) );

	for ( int i = 0; i < count; i++ ){
		employees[i].hours = ntohl(employees[i].hours);
	}

	*employeesPtr = employees;
	return STATUS_SUCCESS;
}

int output_file( int fd, struct dbheader_t *head, struct employee_t employees[] ){
	short count = head->count;
	int filesize = sizeof( struct dbheader_t ) + ( count * sizeof( struct employee_t ) );

	#ifdef DEBUG
	printf( "Output:\n\tCount: %d\n\tFilesize: %d\n", count, filesize );
	#endif

	head->version  = htons(head->version);
	head->count    = htons(head->count);
	head->magic    = htonl(head->magic);
	head->filesize = htonl(filesize);

	lseek( fd, 0, SEEK_SET );
	write( fd, head, sizeof(struct dbheader_t) );

	for ( int i = 0; i < count; i++ ){
		employees[i].hours = htonl(employees[i].hours);
	}

	write( fd, employees, count * sizeof(struct employee_t) );
	ftruncate( fd, filesize );

	return STATUS_SUCCESS;
}

int validate_db_header( int fd, struct dbheader_t **headerOut ){
	struct dbheader_t *head = calloc( 1, sizeof(struct dbheader_t) );
	if ( NULL == head ){
		fprintf( stderr, "Failed to allocate memory for header\n" );
		return STATUS_ERROR;
	}

	if ( sizeof(struct dbheader_t) != read( fd, head, sizeof(struct dbheader_t) ) ){
		perror("read");
		free(head);
		return STATUS_ERROR;
	}

	head->version  = ntohs(head->version);
	head->count    = ntohs(head->count);
	head->magic    = ntohl(head->magic);
	head->filesize = ntohl(head->filesize);

	if ( HEADER_MAGIC != head->magic ){
		fprintf( stderr, "Not a valid database file\n" );
		free(head);
		return STATUS_ERROR;
	}

	if ( HEADER_VERSION != head->version ){
		fprintf( stderr, "Incorrect header version\n" );
		free(head);
		return STATUS_ERROR;
	}

	struct stat db_stat = {0};
	fstat( fd, &db_stat );

	if ( head->filesize != db_stat.st_size ){
		fprintf( stderr, "Database corruption detected\n" );
		free(head);
		return STATUS_ERROR;
	}

	*headerOut = head;
	return STATUS_SUCCESS;
}

int create_db_header( struct dbheader_t **headerOut ){
	if (headerOut == NULL) {
		return STATUS_ERROR;
	}

	struct dbheader_t *head = calloc( 1, sizeof(struct dbheader_t) );
	if ( NULL == head ){
		fprintf( stderr, "Failed to allocate memory for header\n" );
		return STATUS_ERROR;
	}
	head->version = HEADER_VERSION;
	head->count = 0;
	head->magic = HEADER_MAGIC;
	head->filesize = sizeof( struct dbheader_t );

	*headerOut = head;

	return STATUS_SUCCESS;
}

void clean_up(struct dbheader_t **head, struct employee_t *employeesPtr[]){
	free(*head);
	free(*employeesPtr);
	*head = NULL;
	*employeesPtr = NULL;
}
