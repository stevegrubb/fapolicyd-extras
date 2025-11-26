NAME		= test-driver
SRCS 		= test-driver.c
OBJS		= test-driver.o
INCLUDES	=
CFLAGS= -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -gdwarf -W -Wall -Og -fasynchronous-unwind-tables -D_GNU_SOURCE -Wp,-D_GLIBCXX_ASSERTIONS -fexceptions -I/home/build/fapolicyd-1.4.2-build/fapolicyd-1.4.2/ -I/home/build/fapolicyd-1.4.2-build/fapolicyd-1.4.2/src/library/ # -fsanitize=address,undefined,null,return,object-size,nonnull-attribute,returns-nonnull-attribute,bool,enum -ggdb -fno-sanitize-recover=signed-integer-overflow # -fanalyzer -fdiagnostics-path-format=separate-events
#LDFLAGS =  # -fsanitize=address,undefined
LDFLAGS = -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer
LIBS= -L/home/build/fapolicyd-1.4.2-build/fapolicyd-1.4.2/src/.libs -lfapolicyd -lmagic -lseccomp -lzstd -llzma -lbz2 -lz -lpthread -llmdb -lcrypto -ludev -lrpmio -lrpm
CC=gcc

all:
	$(CC) -c $(CFLAGS) $(INCLUDES) $(SRCS) 
	$(CC) -o ${NAME} $(LDFLAGS) $(OBJS) $(LIBS)

clean:
	rm -f ${NAME} *.o
