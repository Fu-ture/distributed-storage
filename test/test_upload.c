/*
 * echo.c --
 *
 *	Produce a page containing all FastCGI inputs
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */
#ifndef lint
static const char rcsid[] = "$Id: echo.c,v 1.5 1999/07/28 00:29:37 roberts Exp $";
#endif /* not lint */

#include "fcgi_config.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <sys/wait.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _WIN32
#include <process.h>
#else
extern char **environ;
#endif

#include "fcgi_stdio.h"
#include "make_log.h"
#include "redis_op.h"

#define FDFS_CLIENT_CONF    "./conf/client/client.conf"
#define UPLOAD_TEST_MUDULE  "test"
#define UPLOAD_TEST_PROC    "upload_test"

#define MD5_LEN 256

#define FILE_LEN 128
#define TIME_LEN 22
#define URL_LEN 1024
#define USER_LEN 22
#define TYPE_LEN 8

#define HASH_NAME "FILENAME_HASH"
#define HASH_URL  "URL_HASH"
#define HASH_USER "USER_HASH"
#define HASH_TIME "CREATETIME_HASH"
#define HASH_TYPE "TYPE_HASH"

typedef struct
{

    char filename[FILE_LEN];
    char url[URL_LEN];
    char user[USER_LEN];
    char type[TYPE_LEN];
    char create_time[TIME_LEN];

}FileInfo;



/*
   static void PrintEnv(char *label, char **envp)
   {
   printf("%s:<br>\n<pre>\n", label);
   for ( ; *envp != NULL; envp++) {
   printf("%s\n", *envp);
   }
   printf("</pre><p>\n");
   }
   */

/*内存块的字符串查找*/
char* memstr(char* full_data, int full_data_len, char* substr)  
{  
    if (full_data == NULL || full_data_len <= 0 || substr == NULL) {  
        return NULL;  
    }  

    if (*substr == '\0') {  
        return NULL;  
    }  

    int sublen = strlen(substr);  

    int i;  
    char* cur = full_data;  
    int last_possible = full_data_len - sublen + 1;  
    for (i = 0; i < last_possible; i++) {  
        if (*cur == *substr) {  
            assert(full_data_len - i >= sublen);  
            if (memcmp(cur, substr, sublen) == 0) {  
                //found  
                return cur;  
            }  
        }  
        cur++;  
    }  

    return NULL;  
}  



/* 截取出fileName */
int getFilename(char *Str,char **fileName)
{
    int ret = 0;
    int len = 0;
    char *p = NULL;
    char *delim = "\"";
    p = Str;
    len = strlen(Str);
    if(p[len-1] == '\r')
        p[len-1] = '\0';
    p = strstr(p,"filename=");
    if(p == NULL) {
        ret = -1;
        goto END;
    }
    strtok(p,delim);
    p = strtok(NULL,delim);
    len = strlen(p) + 1;
    *fileName = (char*)malloc(len);
    memcpy(*fileName,p,len);

END:
    return ret;
}

/*将文件保存到本地，并且返回文件名字*/
int saveFile(char *getStr,int strLen,char **fName) 
{
    int ret = 0;

    int i = 0;
    int len = 0;
    char p[1024] = { 0 };
    char *buf = (char *)malloc(strLen);
    char *delim = "\n";
    char *fileName = NULL;

    char *start = NULL;
    char *end = NULL;

    //将传回来的进行复制
    memcpy(buf,getStr,strLen);
    char *pStart = NULL;
    FILE *fp =NULL;

    strtok(buf,delim);

    pStart = strtok(NULL,delim);

    strcpy(p,pStart);
    if((getFilename(p,&fileName)) < 0) {
        ret = -1;
        LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"not sub fileName");
        goto END;
    }


    /**------*/
    *fName = (char *)calloc((strlen(fileName)+1),1);
    memcpy(*fName,fileName,strlen(fileName));
    /**------*/


    /**恢复strtok线程函数*/
    pStart = strchr(pStart,'\0');
    pStart++;
    /****/

    /**--------*/
    strtok(pStart,delim);
    pStart = strtok(NULL,delim);
    start = pStart+2;
    len = strlen(buf);

    if(buf[len - 1] == '\r') {
        buf[len - 1] = '\0';
    }

    end = memstr(start,strLen,buf);
    len = end - start;
    /**--------*/


    fp = fopen(fileName,"w");
    for(i = 0 ;i < len; i++) {
        fputc(start[i],fp);
    }
    fclose(fp);

END:
    free(buf);
    free(fileName);
    LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"- %d -",ret);
    return ret;

}


/*打开一个子进程,存进storage,并且父进程读出xxxxx,unlink(fd)
 *
 * @param fileName  传入文件名
 * @param uploadMD5 传出uploadMD5,作为file_id
 * @success return 0 
 * @fail    return -1
 */
int upload_to_storage(char *fileName , char *uploadMD5) {
    int ret = 0;
    int len = 0;
    pid_t pid = 0;
    int fd[2];
    if (pipe(fd) < 0) {
        LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"[error] : pipe() err");
        ret = -1;
        goto END;
    }

    if((pid = fork()) < 0) {
        LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"[error] : fork() err");
        ret = -1;
        goto END;
    }

    if(pid == 0) {
        //child 
        close(fd[0]); 
        dup2(fd[1],STDOUT_FILENO); 
        execlp("fdfs_upload_file" , "fdfs_upload_file" , FDFS_CLIENT_CONF , fileName ,NULL);
        LOG(UPLOAD_TEST_MUDULE ,UPLOAD_TEST_PROC,"[error] : child process upload fail");
        exit(-1);

    } 
    close(fd[1]);
    wait(NULL);
    while((read(fd[0],uploadMD5,MD5_LEN)) > 0);
    len = strlen(uploadMD5);
    if(uploadMD5[len-1] == '\n')
        uploadMD5[len -1] = '\0';
    LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"- %s -,upload success",uploadMD5);
    close(fd[0]);
END:
    return ret;
}

/*
 * describe             得到文件的url地址
 *
 * @paramfileid         传进fileid
 * @paramfdfs_file_url  传出url
 */
int make_file_url(char *fileid, char *fdfs_file_url)
{
    int ret = 0;

    char *p = NULL;
    char *q = NULL;
    char *k = NULL;

    char fdfs_file_stat_buf[1024] = {0};
    char fdfs_file_host_name[URL_LEN] = {0};

    int pfd[2];

    pid_t pid;

    pipe(pfd);

    pid = fork();

    if (pid == 0) {
        close(pfd[0]);

        dup2(pfd[1], STDOUT_FILENO);
        execlp("fdfs_file_info", "fdfs_file_info", FDFS_CLIENT_CONF, fileid, NULL);
    }

    close(pfd[1]);

    read(pfd[0], fdfs_file_stat_buf, 1024);
    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "get file_ip - %s - succ\n", fdfs_file_stat_buf);
    wait(NULL);
    close(pfd[0]);


    //拼接上传文件的完整url地址--->http://host_name/group1/M00/00/00/D12313123232312.png
    p = strstr(fdfs_file_stat_buf, "source ip address: ");

    //q              k
    q = p + strlen("source ip address: ");
    k = strstr(q, "\n");
    strncpy(fdfs_file_host_name, q, k-q);

    fdfs_file_host_name[k-q] = '\0';

    //printf("host_name:[%s]\n", fdfs_file_host_name);
    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "get file_ip - %s - succ\n", fdfs_file_stat_buf);

    strcat(fdfs_file_url, "http://");
    strcat(fdfs_file_url, fdfs_file_host_name);
    strcat(fdfs_file_url, "/");
    strcat(fdfs_file_url, fileid);

    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "get file_ip - %s - succ\n", fdfs_file_url);

    return ret;

}
int get_file_suffix(const char *file_name, char *suffix)
{
    const char *p = file_name;
    int len = 0;
    const char *q=NULL;
    const char *k= NULL;

    if (p == NULL) {
        return -1; 
    }   

    q = p;

    while (*q != '\0') {
        q++;
    }   

    k = q;
    while (*k != '.' && k != p) {
        k--;
    }   

    if (*k == '.') {
        k++;
        len = q - k;

        if (len != 0) {
            strncpy(suffix, k, len);
            suffix[len] = '\0';
        }   
        else {
            strncpy(suffix, "null", 5); 
        }   
    }   
    else {
        strncpy(suffix, "null", 5);
    }

    return 0;
}

/*
 * describe:传入uploadMD5
 * @param fileId 
 * @param fileName
 *
 * @return 
 * success 0
 * faile   -1
 */
int upload_FileInfo_redis(char *fileId,char *fileName,char *user)
{

    int ret = 0;
    time_t now;

    redisContext * redis_conn = NULL;

    RFIELDS InfoKey = (RFIELDS)malloc(FIELD_ID_SIZE); 
    RVALUES InfoValue = (RVALUES)malloc(VALUES_ID_SIZE);


    FileInfo *fileInfo=(FileInfo *)malloc(sizeof(FileInfo));
    bzero(fileInfo,sizeof(FileInfo));



    //------ 打造FILEINFO -------------/

    memcpy(fileInfo->filename,fileName,strlen(fileName) + 1);

    //------获取到httpurl------/
    make_file_url(fileId,fileInfo->url);

    memcpy(fileInfo->user,user,strlen(user) + 1);

    now = time(NULL);
    strftime(fileInfo->create_time, TIME_LEN-1 , "%Y-%m-%d %H:%M:%S", localtime(&now)); 

    get_file_suffix(fileName, fileInfo->type);

    //------ 打造结束 -------------/


    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "get url + \n %s \n %s \n %s \n %s \n %s+ succ",fileInfo->filename,fileInfo->url,fileInfo->user,fileInfo->create_time,fileInfo->type);

    /*
       strcpy(InfoKeys[0] , FILEINFO);
       strcpy(InfoKeys[1] , URLINFO);
       strcpy(InfoKeys[2] , USERINFO);
       strcpy(InfoKeys[3] , TIMEINFO);
       strcpy(InfoKeys[4] , TYPEINFO);

       LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "get InfoKeys + \n %s \n %s \n %s \n %s \n %s+ succ",InfoKeys[0],InfoKeys[1],InfoKeys[2],InfoKeys[3],InfoKeys[4]);

    //将打造好的RVALUES传进
    strcpy(InfoValues[0] , fileInfo->filename);
    strcpy(InfoValues[1] , fileInfo->url);
    strcpy(InfoValues[2] , fileInfo->user);
    strcpy(InfoValues[3] , fileInfo->create_time);
    strcpy(InfoValues[4] , fileInfo->type);
    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "get InfoKeys + \n %s \n %s \n %s \n %s \n %s+ succ",InfoKeys[0],InfoKeys[1],InfoKeys[2],InfoKeys[3],InfoKeys[4]);
    //等有时间去修改一下rop_create_or_replace_hash
    */


    //在redis_key.h中定义的宏
    redis_conn = rop_connectdb_nopwd(REDIS_SERVER_IP, REDIS_SERVER_PORT);
    if (redis_conn == NULL) {
        ret = -1;
        LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "[error] : connect redis error");
        goto END;
    }
    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "connect redis success");


    /*------将fileId存进List--------*/
    if ((rop_list_push(redis_conn , FILE_LIST , fileId)) <  0) {
        ret = -1;
        LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "[error] : insert redis list  %s error", FILE_LIST);
        goto END;
    }
    /*-------- end -----------------*/


    /*---------将info存进哈希表*/
    //rop_create_or_replace_hash_table(redis_conn,fileId,5, InfoKeys,InfoValues);

    bzero(InfoKey,FIELD_ID_SIZE);
    memcpy(InfoKey,fileId,strlen(fileId) + 1);


    bzero(InfoValue,VALUES_ID_SIZE);
    memcpy(InfoValue,fileInfo->filename,strlen(fileInfo->filename)+1);
    if((rop_hash_set_append(redis_conn,HASH_NAME,InfoKey,InfoValue,1)) < 0) {

        LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s err",InfoValue,HASH_NAME);
        goto END; 
    }
    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s succ",InfoValue,HASH_NAME);


    bzero(InfoValue,VALUES_ID_SIZE);
    memcpy(InfoValue,fileInfo->url,strlen(fileInfo->url)+1);
    if((rop_hash_set_append(redis_conn,HASH_URL,InfoKey,InfoValue,1)) < 0) {

        LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s err",InfoValue,HASH_URL);
        goto END; 
    }
    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s succ",InfoValue,HASH_URL);


    bzero(InfoValue,VALUES_ID_SIZE);
    memcpy(InfoValue,fileInfo->user,strlen(fileInfo->user)+1);
    if((rop_hash_set_append(redis_conn,HASH_USER,InfoKey,InfoValue,1)) < 0) {

        LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s err",InfoValue,HASH_USER);
        goto END; 
    }
    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s succ",InfoValue,HASH_USER);


    bzero(InfoValue,VALUES_ID_SIZE);
    memcpy(InfoValue,fileInfo->create_time,strlen(fileInfo->create_time)+1);
    if((rop_hash_set_append(redis_conn,HASH_TIME,InfoKey,InfoValue,1)) < 0) {

        LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s err",InfoValue,HASH_TIME);
        goto END; 
    }
    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s succ",InfoValue,HASH_TIME);


    bzero(InfoValue,VALUES_ID_SIZE);
    memcpy(InfoValue,fileInfo->type,strlen(fileInfo->type)+1);
    if((rop_hash_set_append(redis_conn,HASH_TYPE,InfoKey,InfoValue,1)) < 0) {

        LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s err",InfoValue,HASH_TYPE);
        goto END; 
    }
    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "set %s to %s succ",InfoValue,HASH_TYPE);


    /*-------- end ------------*/



    /*--有序集合--*/
    rop_zset_increment(redis_conn, FILE_HOT_ZSET, fileId);
    //LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "inser sort set  %s err",FILE_HOT_ZSET);
    /*-- end --*/


    LOG(UPLOAD_TEST_MUDULE, UPLOAD_TEST_PROC, "redis save cache success");

END:

    free(InfoValue);
    free(InfoKey);
    free(fileInfo);
    rop_disconnect(redis_conn);
    return ret;

}

int main (void)
{
    //char **initialEnv = environ;

    int count = 0;
    char *fileStr =NULL;
    char *user = "zou";
    char *fileName=NULL;
    char uploadMD5[MD5_LEN] = { 0 };

    while (FCGI_Accept() >= 0) {
        //得到客户端传回的长度
        char *contentLength = getenv("CONTENT_LENGTH");
        int len;

        printf("Content-type: text/html\r\n"
                "\r\n"
                "<title>FastCGI echo</title>"
                "<h1>FastCGI echo</h1>\n"
                "Request number %d,  Process ID: %d<p>\n", ++count, getpid());

        if (contentLength != NULL) {
            //转换为10进制的long类型的
            len = strtol(contentLength, NULL, 10);
        }
        else {
            len = 0;
        }

        if (len <= 0) {
            printf("No data from standard input.<p>\n");
        }
        else {

            fileStr = (char *)malloc(len);
            bzero(fileStr,len);

            int i, ch;

            printf("Standard input:<br>\n<pre>\n");

            //在传输协议中，不是全部都是标准的字符串，应该进行字符复制，或者内存复制
            for (i = 0; i < len; i++) {
                if ((ch = getchar()) < 0) {
                    printf("Error: Not enough bytes received on standard input<p>\n");
                    break;
                }
                //putchar(ch);
                fileStr[i]=ch;
            }

            /*----------保存文件-------------*/
            if((saveFile(fileStr,len,&fileName)) < 0) {
                free(fileStr);
                if(fileName != NULL) {
                    unlink(fileName);
                    free(fileName);
                }
                LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"[error] : file save to local err");
                //continue;
                exit(-1);
            }

            LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"[error] : file save to local success");
            free(fileStr);
            /*----------end-------------*/



            /*----------保存文件到storage-------------*/
            if( (upload_to_storage(fileName,uploadMD5)) < 0 ) {

                unlink(fileName);
                free(fileName);
                LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"[error] : file save to storage err");

                //continue;
                exit(-1);
            }

            LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"[error] : file save to storage succcess");
            /*----------end-------------*/


            /*将uploadMD5作为fileId,存到redis链表*/
            if((upload_FileInfo_redis(uploadMD5,fileName,user)) < 0) {

                unlink(fileName);
                free(fileName);
                LOG(UPLOAD_TEST_MUDULE,UPLOAD_TEST_PROC,"[error] : file save cahe to redis err");

                //continue;
                exit(-1);
            }
            /*----------end-------------*/

            unlink(fileName);
            free(fileName);

            printf("File upload success<p>\r\n");
            printf("\n</pre><p>\n");
        }



        /*
           PrintEnv("Request environment", environ);
           PrintEnv("Initial environment", initialEnv);
           */
    } /* while */

    return 0;
}
