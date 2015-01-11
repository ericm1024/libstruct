CC = gcc
OPTFLAGS = -O0
INCPATH = ../include
CFLAGS = -g -std=gnu99 -Wall -Wextra -pedantic $(OPTFLAGS) -I $(INCPATH)
TARGETS = flist_test list_test avl_test

vpath %.h $(INCPATH)

all: $(TARGETS)

clean:
	rm -f *.o *~ $(TARGETS)

test: $(TARGETS)
	valgrind --leak-check=full ./flist_test
	valgrind --leak-check=full ./list_test
	valgrind --leak-check=full ./avl_test

flist_test: flist_test.o flist.o
	$(CC) $(CFLAGS) -o $@ $^

list_test: list_test.o list.o
	$(CC) $(CFLAGS) -o $@ $^

avl_test: avl_test.o avl_tree.o test.o
	$(CC) $(CFLAGS) -o $@ $^

flist_test.o: flist_test.c flist.h
	$(CC) $(CFLAGS) -c $<

flist.o: flist.c flist.h flist_types.h
	$(CC) $(CFLAGS) -c $<

list_test.o: list_test.c list.h
	$(CC) $(CFLAGS) -c $<

list.o: list.c list.h list_types.h
	$(CC) $(CFLAGS) -c $<

avl_test.o: avl_test.c avl_tree.h
	$(CC) $(CFLAGS) -c $<

avl_tree.o: avl_tree.c avl_tree.h avl_tree_types.h bitops.h
	$(CC) $(CFLAGS) -c $<

test.o: test.c test.h
	$(CC) $(CFLAGS) -c $<
