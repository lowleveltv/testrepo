#ifndef PARSE_H
#define PARSE_H

#define HEADER_MAGIC 0x4c4c4144
#define HEADER_VERSION 0x1

struct dbheader_t {
	unsigned int magic;
	unsigned short version;
	unsigned short count;
	unsigned int filesize;
};

struct employee_t {
	char name[64];
	char address[256];
	unsigned int hours;
};

int create_db_header( struct dbheader_t **headerOut);
int validate_db_header(int fd, struct dbheader_t **headerOut);
int read_employees(int fd, struct dbheader_t *head, struct employee_t *employeesPtr[]);
int output_file(int fd, struct dbheader_t *head, struct employee_t employees[]);
void list_employees(struct dbheader_t *head, struct employee_t employees[]);
int add_employee(struct dbheader_t *head, struct employee_t *employeesPtr[], char *addstring);
int delete_employee( struct dbheader_t *head, struct employee_t *employeesPtr[], int emp_id );
void update_hours( struct employee_t employees[], int emp_id, int hours );
int search_employee(struct dbheader_t *head, struct employee_t employees[], char *searchstring);
void clean_up(struct dbheader_t **head, struct employee_t *employeesPtr[]);

#endif
