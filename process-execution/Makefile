NAME		= fanotify_monitor
SRCS 		= fanotify_monitor.c
OBJS		= fanotify_monitor.o
INCLUDES	=
CFLAGS=-g -W -Wall -Wundef -O3 -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -Wp,-D_GLIBCXX_ASSERTIONS -fexceptions -fpie -fstack-protector-strong --param=ssp-buffer-size=4 -fstack-clash-protection -fcf-protection -flto=auto -ffat-lto-objects
LDFLAGS = -Wl,-z,relro -Wl,-z,now -flto=auto -ffat-lto-objects -fpie
LIBS=
CC=gcc

all:
	$(CC) -c $(CFLAGS) $(INCLUDES) $(SRCS) 
	$(CC) -o ${NAME} $(LDFLAGS) $(OBJS) $(LIBS)

clean:
	rm -f ${NAME} *.so* *.o
