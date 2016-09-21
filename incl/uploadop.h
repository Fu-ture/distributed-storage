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

#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include "make_log.h"
#include "redis_op.h"
#include "util_cgi.h"

#define UPLOAD_TEST_MUDULE  "upload"
#define UPLOAD_TEST_PROC    "upload_log"

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

/* 截取出fileName */
int getFilename(char *Str,char **fileName);


/*  describer:解释http返回数据段，将文件保存到本地，并且返回文件名字
 *
 * @return 
 *      succ 0
 *      fail -1
 */

int saveFile(char *getStr,int strLen,char **fName); 



/*打开一个子进程,存进storage,并且父进程读出xxxxx,unlink(fd)
 *
 * @param fileName  传入文件名
 * @param uploadMD5 传出uploadMD5,作为file_id
 * @success return 0 
 * @fail    return -1
 */
int upload_to_storage(char *fileName , char *uploadMD5);


/*
 * describe             得到文件的url地址
 *
 * @paramfileid         传进fileid
 * @paramfdfs_file_url  传出url
 */
int make_file_url(char *fileid, char *fdfs_file_url); 


/*
 * describe:传入uploadMD5
 * @param fileId 
 * @param fileName
 *
 * @return 
 *      success 0
 *      faile   -1
 */
int upload_FileInfo_redis(char *fileId,char *fileName,char *user);
