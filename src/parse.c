#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

int find_employee(struct dbheader_t *dbhdr, struct employee_t *employees, const char *name) {
    int i = 0;
    for (; i < dbhdr->count; i++) {
        if (strcmp(employees[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int edit_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *editstring) {
    char *edit_copy = malloc(strlen(editstring) + 1);
    if (edit_copy == NULL) return STATUS_ERROR;
    strcpy(edit_copy, editstring);
    
    char *name = strtok(edit_copy, ",");
    char *addr = strtok(NULL, ",");
    char *hours = strtok(NULL, ",");

    int index = find_employee(dbhdr, employees, name);
    if (index < 0) {
        free(edit_copy);
        return STATUS_ERROR;
    }

    strncpy(employees[index].address, addr, sizeof(employees[index].address));
    employees[index].hours = atoi(hours);

    free(edit_copy);
    
    return STATUS_SUCCESS;
}

int delete_employee(struct dbheader_t *dbhdr, struct employee_t **employees, const char *name) {
    int index = find_employee(dbhdr, *employees, name);
    if (index < 0) return STATUS_ERROR;

    /* Determine how many employees need to be moved */
    int employees_to_move = dbhdr->count - index - 1;

    /* Shift all subsequent employees up one position
    // Source: Address of the employee after the one we're deleting
    // Destination: Address of the employee we're deleting
    // Size: Number of bytes for all remaining employees */
    memmove(&(*employees)[index], &(*employees)[index+1], 
            employees_to_move * sizeof(struct employee_t));
    
    dbhdr->count--;
    dbhdr->filesize = sizeof(struct dbheader_t) + sizeof(struct employee_t) * dbhdr->count;
    
    /* Resize the array to the new smaller size */
    struct employee_t *new_employees = realloc(*employees, dbhdr->count * sizeof(struct employee_t));
    if (new_employees == NULL && dbhdr->count > 0) {
        dbhdr->count++;
        dbhdr->filesize = sizeof(struct dbheader_t) + sizeof(struct employee_t) * dbhdr->count;
        printf("Failed to resize employee array\n");
        return STATUS_ERROR;
    }
    
    *employees = new_employees;
    
    return STATUS_SUCCESS;
    
    /* compact form
    memmove(&(*employees)[index], &(*employees)[index+1], 
            (dbhdr->count - index - 1) * sizeof(struct employee_t));
    
    dbhdr->count--;
    *employees = realloc(*employees, dbhdr->count * sizeof(struct employee_t));
    */
}

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
    int i = 0;
    for (; i < dbhdr->count; i++) {
        printf("Employee %d:\n", i);
        printf("\tName: %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours: %d\n", employees[i].hours);
        printf("\n");
    }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
    char *name = strtok(addstring, ",");
    char *addr = strtok(NULL, ",");
    char *hours = strtok(NULL, ",");
	
    dbhdr->count += 1;

    strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));
    strncpy(employees[dbhdr->count-1].address, addr, sizeof(employees[dbhdr->count-1].address));
    employees[dbhdr->count-1].hours = atoi(hours);


    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
    if (fd < 0) {
        perror("read_employees: Invalid file descriptor");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;
    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if (employees == NULL) {
        perror("read_employees: calloc failed");
        return STATUS_ERROR;
    }

    read(fd, employees, count * sizeof(struct employee_t));
    int i = 0;
    for (; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;
    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (fd < 0) {
        perror("output_file: Invalid file descriptor");
        return STATUS_ERROR;
    }

    int realcount = dbhdr->count;

    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->version = htons(dbhdr->version);
    dbhdr->count = htons(dbhdr->count);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + sizeof(struct employee_t) * realcount);

    /* Position file pointer at beginning of file to update the header */
    lseek(fd, 0, SEEK_SET);
    write(fd, dbhdr, sizeof(struct dbheader_t));

    int i = 0;
    for (; i < realcount; i++) {
        employees[i].hours = htonl(employees[i].hours);
        write(fd, &employees[i], sizeof(struct employee_t));
    }

    /* Ensures the file size always matches what's specified in the header */
    ftruncate(fd, sizeof(struct dbheader_t) + sizeof(struct employee_t) * realcount);
    return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if (fd < 0) {
        perror("validate_db_header: Invalid file descriptor");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        perror("validate_db_header: calloc failed");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("validate_db_header: read failed");
        free(header);
        return STATUS_ERROR;
    }

    header->magic = ntohl(header->magic);
    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->filesize = ntohl(header->filesize);

    if (header->magic != HEADER_MAGIC) {
        fprintf(stderr, "validate_db_header: Invalid magic number: 0x%x\n", header->magic);
        free(header);
        return STATUS_ERROR;
    }

    if (header->version != 1) {
        fprintf(stderr, "validate_db_header: Unsupported version: %u\n", header->version);
        free(header);
        return STATUS_ERROR;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if (header->filesize != dbstat.st_size) {
        fprintf(stderr, "validate_db_header: Corrupt database\n");
        free(header);
        return STATUS_ERROR;
    }

    *headerOut = header;
    return STATUS_SUCCESS;
}

int create_db_header(int fd, struct dbheader_t **headerOut) {
    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        perror("create_db_header: calloc failed");
        return STATUS_ERROR;
    }

    header->magic = HEADER_MAGIC;
    header->version = 0x1;
    header->count = 0;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;
    return STATUS_SUCCESS;	
}


