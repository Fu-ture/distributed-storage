#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "make_log.h"
#include "fdfs_client.h"
#include "shared_func.h"
#include "fcgi_stdio.h"
#include "fcgi_config.h"
#include "cJSON.h"
#include "util_cgi.h"
#include "redis_op.h"

#define DATA_LOG_MODULE          "data"
#define DATA_LOG_PROC            "data_log"

extern char host_name[HOST_NAME_LEN] = {0};

/*
 * describe 文件下载量加 1 
 *
 * @param file_id
 */
void increase_file_pv(char *file_id)
{
    redisContext *redis_conn = NULL;

    redis_conn = rop_connectdb_nopwd(REDIS_SERVER_IP, REDIS_SERVER_PORT);
    if (redis_conn == NULL) {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis connected error");
        return;
    }


    rop_zset_increment(redis_conn, FILE_HOT_ZSET, file_id);


    rop_disconnect(redis_conn);
}

/*
 * describe 动态获取到下载的url
 * 
 * @param 传进file_id
 * @param 传出file_url
 *
 * @return 
 *      succ 0
 *      fail -1
 */
int get_file_url_dynamic(char *file_id, char *file_url)
{
    int result;
    FDFSFileInfo file_info;

	if ((result=fdfs_client_init(FDFS_CLIENT_CONF)) != 0)
	{
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "fdfs_client_init error");
		return result;
	}

    memset(&file_info, 0, sizeof(file_info));

	result = fdfs_get_file_info_ex1(file_id, true, &file_info);
	if (result != 0)
	{
		LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "query file info fail, " \
			"error no: %d, error info: %s\n", \
			result, STRERROR(result));
	}
    else {
        memset(file_url, 0, URL_LEN);
        strcat(file_url, "http://");
        strcat(file_url, file_info.source_ip_addr);
        strcat(file_url, "/");
        strcat(file_url, file_id);

        //LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "file_url[%s]", file_url);
    }

	tracker_close_all_connections();
	fdfs_client_destroy();

    return result;
}

/*
 * describe 取出key哈希表里的field的value值
 */
int get_value_by_fileId(redisContext *conn, char *key, char* fileid , char *value)
{
    int ret = 0;
    int len = 0;
    RFIELDS file_id = (RFIELDS)malloc(FIELD_ID_SIZE);
    memcpy(file_id,fileid,strlen(fileid)+1);
    RVALUES buf = (RVALUES)malloc(VALUES_ID_SIZE);
    if((rop_hash_get(conn, key, file_id , buf)) < 0 ) {
        ret = -1;
        goto END;
    }
    len = strlen(buf) + 1;
    memcpy(value,buf,len);
END:
    free(file_id);
    free(buf);
    return ret;
}

void print_file_list_json(int fromId, int count, char *cmd, char *kind)
{

    int ret = 0;
    int i = 0;


    cJSON *root = NULL; 
    cJSON *array =NULL;
    char *out;

    char file_id[MD5_LEN] = {0};
    char file_url[URL_LEN] = {0};
    char filename[FILE_LEN] = {0};
    char user[USER_LEN] = {0};
    char create_time[TIME_LEN] ={0};
    char suffix[TYPE_LEN] = {0};

    char picurl[URL_LEN] = {0};
    char pic_name[TYPE_LEN] = {0};
    
    //也就是说每次加载，从0到endId,这样好吗
    int endId = fromId + count - 1;
    //下载量
    int score = 0;

    RVALUES fileIds = NULL;
    int value_num = 0;

    redisContext *redis_conn = NULL;
    redis_conn = rop_connectdb_nopwd(REDIS_SERVER_IP, REDIS_SERVER_PORT);
    if (redis_conn == NULL) {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis connected error");
        return;
    }



    //取出value个fileId 
    //应该有缓存池,避免申请空间越来越大
    fileIds = (RVALUES)malloc(count * VALUES_ID_SIZE);

    ret = rop_range_list(redis_conn, FILE_LIST, fromId, endId, fileIds, &value_num);
    
    if (ret < 0) {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis range list error");
        rop_disconnect(redis_conn);
        return;
    }
    LOG(DATA_LOG_MODULE, DATA_LOG_PROC, " request List fileId value_num=%d\n", value_num);


    root = cJSON_CreateObject();
    array = cJSON_CreateArray();
    for (i = 0;i < value_num;i++) {
        //array[i]:
        cJSON* item = cJSON_CreateObject();

        //id
        bzero(file_id,MD5_LEN);
        strcpy(file_id,fileIds[i]);
        memcpy(file_id,fileIds[i],strlen(fileIds[i]) + 1);
        cJSON_AddStringToObject(item, "id", file_id);
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "fileId=%s\n", file_id);

        //kind
        //get_value_by_fileId(redis_conn,HASH_TYPE,file_id,file_type);
        cJSON_AddNumberToObject(item, "kind", 2);

        //title_m(filename)
        get_value_by_fileId(redis_conn,HASH_NAME,fileIds[i],filename);
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "filename=%s\n", filename);
        cJSON_AddStringToObject(item, "title_m", filename);

        //title_s
        get_value_by_fileId(redis_conn,HASH_USER,fileIds[i],user);
        cJSON_AddStringToObject(item, "title_s", user);

        //time
        get_value_by_fileId(redis_conn,HASH_USER,fileIds[i],create_time);
        cJSON_AddStringToObject(item, "descrip", create_time);
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "create_time=%s\n", create_time);

        //picurl_m
        memset(picurl, 0, URL_LEN);
        strcat(picurl, host_name);
        strcat(picurl, "/static/file_png/");


        get_file_suffix(filename, suffix);
        sprintf(pic_name, "%s.png", suffix);
        strcat(picurl, pic_name);
        cJSON_AddStringToObject(item, "picurl_m", picurl);

        //url
#if GET_URL_DYNAMIC
        get_file_url_dynamic(file_id, file_url);
#else
        get_value_by_fileId(redis_conn,HASH_URL,fileIds[i],file_url);
#endif
        cJSON_AddStringToObject(item, "url", file_url);
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "file_url=%s\n", file_url);

        //pv
        score = rop_zset_get_score(redis_conn, FILE_HOT_ZSET, file_id);
        cJSON_AddNumberToObject(item, "pv", score-1);

        //hot 
        //cJSON_AddNumberToObject(item, "hot", i%2);
        cJSON_AddNumberToObject(item, "hot", 0);


        cJSON_AddItemToArray(array, item);
    }



    cJSON_AddItemToObject(root, "games", array);

    out = cJSON_Print(root);

    LOG(DATA_LOG_MODULE, DATA_LOG_PROC,"\n%s\n", out);
    printf("%s\n", out);

    free(fileIds);
    free(out);

    rop_disconnect(redis_conn);
}

int main (void)
{

    char fromId[5];
    char count[5];
    char cmd[20];
    char kind[10];
    char fileId[MD5_LEN];


    while (FCGI_Accept() >= 0) {

        char *query = getenv("QUERY_STRING");
        host_init();

        
        bzero(fromId, 5);
        bzero(count, 5);
        bzero(cmd , 20);
        bzero(kind, 10);
        bzero(fileId,MD5_LEN);

        query_parse_key_value(query, "cmd", cmd, NULL);


        if (strcmp(cmd, "newFile") == 0) {

            //请求最新文件列表命令
            query_parse_key_value(query, "fromId", fromId, NULL);
            query_parse_key_value(query, "count", count, NULL);
            query_parse_key_value(query, "kind", kind, NULL);
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "=== fromId:%s, count:%s\n, cmd:%s\n, kind:%s\n", fromId, count, cmd, kind);
            
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "%s\n",host_name);

            printf("Content-type: text/html\r\n");
            printf("\r\n");

            print_file_list_json(atoi(fromId), atoi(count), cmd, kind);
        }
        else if (strcmp(cmd, "increase") == 0) {
            //文件被点击

            //得到点击的fileId
            query_parse_key_value(query, "fileId", fileId, NULL);
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "=== fileId:%s,cmd:%s", fileId,  cmd);

            str_replace(fileId, "%2F", "/");

            increase_file_pv(fileId);


            printf("Content-type: text/html\r\n");
            printf("\r\n");
        }
    }
    return 0;
}
