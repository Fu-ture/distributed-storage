/**
 * @brief  redis表 KEYS 值相关定义 
 * @author zou
 */

#ifndef _REDIS_KEYS_H_
#define _REDIS_KEYS_H_

#define REDIS_SERVER_IP     "127.0.0.1"
#define REDIS_SERVER_PORT   "6379"


#define KEY_NAME_SIZ                (50)
#define REDIS_DILIMT                "|"
#define REDIS_DILIMT_SIZ            (3)

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



/*-----------------------------------------------------------.
| 文件信息表(LIST)                                           |
| Key:     FILE_INFO_LIST                                    |
| Value:   file_id||url||filename||create time||user||type   |
| redis 语句                                                 |
|    插入 LPUSH key value                                    |
|    查询个数 LLEN key                                       |
|    大于最大需要截断 LTRIM key 0 max-1                      |
|    查询链表数据    LRANGE key 0 max-1                      |
`-----------------------------------------------------------*/
#define FILE_LIST                  "FILE_LIST"


/*------------------------------------------.
| 点击量文件表 (ZSET)                       |
| Key:     FILE_HOT_ZSET                    |
| Member:  file_id                          |
| Score:   pv                               |
| redis 语句                                |
|    ZINCRBY key increment member           |
`------------------------------------------*/
#define FILE_HOT_ZSET                   "FILE_HOT_ZSET"


/*--------------------------------.
| 文件类型                        |
`--------------------------------*/
#define FILE_TYPE_PIC               "1"
#define FILE_TYPE_ZIP               "2"
#define FILE_TYPE_VEDIO             "3"
#define FILE_TYPE_MEDIA             "4"
#define FILE_TYPE_TXT               "5"
#define FILE_TYPE_OTHER             "6"




#endif

