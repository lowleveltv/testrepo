#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "file.h"
#include "common.h"


int create_db_file(char *filename){
	int fd = open( filename, O_RDWR | O_CREAT | O_EXCL, 0644 );
	if ( -1 == fd ){
		perror("open");
		return STATUS_ERROR;
	}

	return fd;
}

int open_db_file(char *filename){
	int fd = open( filename, O_RDWR );
	if ( -1 == fd ){
		perror("open");
		return STATUS_ERROR;
	}

	return fd;
}

void close_db_file(int fd){
	close(fd);
}
