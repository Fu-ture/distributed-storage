
WORKDIR=.
VPATH =./src ./test

CC=gcc
CPPFLAGS= -I./incl -I/usr/local/include/hiredis/ -I/usr/include/fastdfs/ -I/usr/include/fastcommon/
CFLAGS=-Wall 
LIBS= -lhiredis -lpthread -lfcgi -lfdfsclient -lm

#找到当前目录下所有的.c文件
src = $(wildcard *.c)

#将当前目录下所有的.c  转换成.o给obj
obj = $(patsubst %.c, %.o, $(src))


#fdfs_test=fdfs_test
#redis_test=redis_test
#test_main = test_main
#test_fcgi = test_fcgi
#test_upload = test_upload
uploadFile = uploadFile
dataMain = dataMain

#target=$(fdfs_test) $(redis_test) $(test_main) $(test_fcgi) $(test_upload)
target=$(uploadFile) $(dataMain)


ALL:$(target)

#生成所有的.o文件
$(obj):%.o:%.c
	$(CC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS) 


#$(test_main):make_log.o test_main.o redis_op.o
#	$(CC) $^ -o $@ $(LIBS)

#fdfs_est程序
#$(fdfs_test):make_log.o log_test.o
#	$(CC) $^ -o $@ $(LIBS)


#$(redis_test):redis_test.o
#	$(CC) $^ -o $@ $(LIBS)

#$(test_fcgi):test_fcgi.o
#	$(CC) $^ -o $@ $(LIBS)

#$(test_upload):test_upload.o make_log.o redis_op.o
#	$(CC) $^ -o $@ $(LIBS)

$(dataMain):dataMain.o make_log.o redis_op.o util_cgi.o cJSON.o
	$(CC) $^ -o $@ $(LIBS)

$(uploadFile):uploadFile.o make_log.o redis_op.o uploadop.o util_cgi.o
	$(CC) $^ -o $@ $(LIBS)
#clean指令

clean:
	rm -rf $(target)
	rm -rf $(obj)
	rm -rf *.o

#distclean:
#	-rm -rf $(obj) $(target)

#将clean目标 改成一个虚拟符号
.PHONY: clean ALL distclean
