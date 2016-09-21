#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>

#include "make_log.h"

#define FDFS_TEST_MUDULE "test"
#define FDFS_TEST_PROC   "fdfs_log_test"

#define LOG_LEN 128


int main(int argc,char **argv)
{
    int fd[2];
    pid_t pid = 0;
    char *file_name = NULL;
    char logStr[LOG_LEN ]={0};

    if(argc < 2) {
        printf("usage : ./fdfs_test file_anme\n");
        exit(0);
    }
    file_name = argv[1];

    if (0 > pipe(fd)) {
        LOG(FDFS_TEST_MUDULE,FDFS_TEST_PROC,"[error] : pipe error");
        exit(1);
    }
    pid = fork();

    if (pid == 0) {

        close(fd[0]);
        dup2(fd[1],STDOUT_FILENO);

        execlp("fdfs_upload_file", "fdfs_upload_file", "./conf/client.conf",file_name,NULL);
        LOG(FDFS_TEST_MUDULE,FDFS_TEST_PROC,"[error] : upload fail");
    }
    else {
        close(fd[1]);
        wait(NULL);

        read(fd[0],logStr,LOG_LEN);
        LOG(FDFS_TEST_MUDULE,FDFS_TEST_PROC," %s ",logStr);
    }


    return 0;
}

