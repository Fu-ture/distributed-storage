#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "redis_op.h"


int main(void)
{
    int ret = 0;

    char key[128]= "name";
    char str[128] = "zou";
    char buf[128]={0};
    char ip[128] ="127.0.0.1";
    char port[128] = "6379";
    redisContext* conn= NULL; 

    conn = rop_connectdb_nopwd(ip, port);
    if(conn == NULL)
        exit(1);

    ret = rop_set_String(conn, key,str);
    if(ret != 0) {
        printf("set_String error\n");
        exit(1);
    }

    ret = rop_get_String(conn,key,buf);
    printf("from %s get_String %s\n",key,buf);

    rop_disconnect(conn);

    return 0;
}
