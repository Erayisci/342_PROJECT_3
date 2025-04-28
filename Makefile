
CC	:= gcc
CFLAGS := -g -Wall

TARGETS :=  app1-add app1-get libpstree.a
# Make sure that 'all' is the first target
all: $(TARGETS)

PST_SRC :=  pstree.c
PST_OBJS := $(PST_SRC:.c=.o)

libpstree.a: $(PST_OBJS)
	ar rcs $@ $(PST_OBJS)

PST_LIB :=  -L . -l pstree

pstree.o: pstree.c pstree.h
	gcc -c $(CFLAGS)  -o $@ pstree.c

app1-add: app1-add.o libpstree.a
	gcc $(CFLAGS) -o $@ app1-add.o  $(PST_LIB)
	
app1-get: app1-get.o libpstree.a
	gcc $(CFLAGS) -o $@ app1-get.o  $(PST_LIB)


clean:
	rm -rf core  *.o *.out *~ $(TARGETS)
	