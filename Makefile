OBJS=proxy_cache.c
CC=gcc
EXEC=proxy_cache
all:
		$(CC) $(OBJS) -o $(EXEC) -pthread -lcrypto
clean:
		rm -rf $(EXEC)
cl:
		rm -r ~/cache ~/logfile