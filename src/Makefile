CC = clang
OPTFLAGS = -O0
INCLUDE = ../include
RNG = ./pcg-c-master
CFLAGS = -g -std=gnu99 -Wall -Wextra -pedantic $(OPTFLAGS) -I$(INCLUDE) -I$(RNG)/include
TARGETS = flist_test list_test avl_test htable_test bloom_test

vpath %.h $(INCLUDE)
vpath %.h $(RNG)/include

all: $(TARGETS)

clean:
	rm -f *.o *~ $(TARGETS)

test: $(TARGETS)
	valgrind --leak-check=full ./flist_test
#	valgrind --leak-check=full ./list_test
	valgrind --leak-check=full ./avl_test
	valgrind --leak-check=full ./htable_test
	valgrind --leak-check=full --max-stackframe=20000000 ./bloom_test

flist_test: flist_test.o flist.o
	$(CC) $(CFLAGS) -o $@ $^

list_test: list_test.o list.o
	$(CC) $(CFLAGS) -o $@ $^

avl_test: avl_test.o avl_tree.o test.o
	$(CC) $(CFLAGS) -o $@ $^

htable_test: htable_test.o htable.o test.o fasthash.o murmurhash.o
	$(CC) $(CFLAGS) -lm -o $@ $^

bloom_test: bloom_test.o bloom.o test.o fasthash.o
	$(CC) $(CFLAGS) -lm -o $@ $^ $(RNG)/src/libpcg_random.a

flist_test.o: flist_test.c flist.h libstruct.h
	$(CC) $(CFLAGS) -c $<

flist.o: flist.c flist.h libstruct.h
	$(CC) $(CFLAGS) -c $<

list_test.o: list_test.c list.h libstruct.h
	$(CC) $(CFLAGS) -c $<

list.o: list.c list.h libstruct.h
	$(CC) $(CFLAGS) -c $<

avl_test.o: avl_test.c avl_tree.h libstruct.h
	$(CC) $(CFLAGS) -c $<

avl_tree.o: avl_tree.c avl_tree.h bitops.h libstruct.h
	$(CC) $(CFLAGS) -c $<

test.o: test.c test.h
	$(CC) $(CFLAGS) -c $<

htable_test.o: htable_test.c htable.h libstruct.h
	$(CC) $(CFLAGS) -c $<

htable.o: htable.c htable.h libstruct.h fasthash.h murmurhash.h
	$(CC) $(CFLAGS) -c $<

fasthash.o: fasthash.c fasthash.h
	$(CC) $(CFLAGS) -c $<

murmurhash.o: murmurhash.c murmurhash.h
	$(CC) $(CFLAGS) -c $<

btree.o: btree.c btree.h libstruct.h
	$(CC) $(CFLAGS) -c $<

bloom.o: bloom.c bloom.h libstruct.h fasthash.h
	$(CC) $(CFLAGS) -c $<

bloom_test.o: bloom_test.c bloom.h libstruct.h test.h
	$(CC) $(CFLAGS) -c $<
