#sudo redis-server ./conf/redis.conf &

spawn-fcgi -a 127.0.0.1 -p 8083 -f ./dataMain
spawn-fcgi -a 127.0.0.1 -p 8084 -f ./uploadFile
