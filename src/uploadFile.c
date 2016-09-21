#include "uploadop.h"

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
            
            int i, ch;

            fileStr = (char *)malloc(len);
            bzero(fileStr,len);


            printf("Standard input:<br>\n<pre>\n");

            //在传输协议中，不是全部都是标准的字符串，应该进行字符复制，或者内存复制
            for (i = 0; i < len; i++) {
                if ((ch = getchar()) < 0) {
                    printf("Error: Not enough bytes received on standard input<p>\n");
                    break;
                }
                //putchar(ch);
                //将http传回来的截存，然后处理
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

            printf("<h3>File upload success</h3><p>\r\n");
            printf("\n</pre><p>\n");
        }

        /*
           PrintEnv("Request environment", environ);
           PrintEnv("Initial environment", initialEnv);
        */
    } /* while */

    return 0;
}
