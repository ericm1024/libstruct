CC = clang
OPTFLAGS = -O0
INCLUDE = ../include
RNG = ./pcg-c-master
CFLAGS = -g -std=gnu99 -Wall -Wextra -pedantic $(OPTFLAGS) -I$(INCLUDE) -I$(RNG)/include
TARGETS = flist_test list_test avl_test htable_test bloom_test rbtree_test \
	  chunky_str_test binary_heap_test cuckoo_htable_test radix_tree_test

vpath %.h $(INCLUDE)
vpath %.h $(RNG)/include

all: $(TARGETS)

clean:
	rm -f *.o *~ $(TARGETS)

test: $(TARGETS)
	../bin/runtests $(TARGETS)

flist_test: flist_test.o flist.o test.o
	$(CC) $(CFLAGS) -o $@ $^

list_test: list_test.o list.o test.o
	$(CC) $(CFLAGS) -o $@ $^

avl_test: avl_test.o avl_tree.o test.o
	$(CC) $(CFLAGS) -o $@ $^

htable_test: htable_test.o htable.o test.o fasthash.o murmurhash.o
	$(CC) $(CFLAGS) -lm -o $@ $^

cuckoo_htable_test: cuckoo_htable_test.o cuckoo_htable.o test.o fasthash.o
	$(CC) $(CFLAGS) -lm -o $@ $^ $(RNG)/src/libpcg_random.a

bloom_test: bloom_test.o bloom.o test.o fasthash.o
	$(CC) $(CFLAGS) -lm -o $@ $^ $(RNG)/src/libpcg_random.a

rbtree_test: rbtree_test.o rbtree.o test.o
	$(CC) $(CFLAGS) -lm -o $@ $^

chunky_str_test: chunky_str_test.o list.o chunky_str.o test.o
	$(CC) $(CFLAGS) -lm -o $@ $^

binary_heap_test: binary_heap_test.o binary_heap.o test.o
	$(CC) $(CFLAGS) -lm -o $@ $^

radix_tree_test: radix_tree_test.o radix_tree.o test.o
	$(CC) $(CFLAGS) -lm -o $@ $^ $(RNG)/src/libpcg_random.a

flist_test.o: flist_test.c flist.h
	$(CC) $(CFLAGS) -c $<

flist.o: flist.c flist.h
	$(CC) $(CFLAGS) -c $<

list_test.o: list_test.c list.h
	$(CC) $(CFLAGS) -c $<

list.o: list.c list.h
	$(CC) $(CFLAGS) -c $<

avl_test.o: avl_test.c avl_tree.h
	$(CC) $(CFLAGS) -c $<

avl_tree.o: avl_tree.c avl_tree.h bitops.h
	$(CC) $(CFLAGS) -c $<

test.o: test.c test.h
	$(CC) $(CFLAGS) -c $<

htable_test.o: htable_test.c htable.h
	$(CC) $(CFLAGS) -c $<

cuckoo_htable_test.o: cuckoo_htable_test.c cuckoo_htable.h
	$(CC) $(CFLAGS) -c $<

htable.o: htable.c htable.h fasthash.h murmurhash.h
	$(CC) $(CFLAGS) -c $<

fasthash.o: fasthash.c fasthash.h
	$(CC) $(CFLAGS) -c $<

murmurhash.o: murmurhash.c murmurhash.h
	$(CC) $(CFLAGS) -c $<

btree.o: btree.c btree.h
	$(CC) $(CFLAGS) -c $<

bloom.o: bloom.c bloom.h fasthash.h
	$(CC) $(CFLAGS) -c $<

bloom_test.o: bloom_test.c bloom.c bloom.h test.h
	$(CC) $(CFLAGS) -c $<

rbtree.o: rbtree.c rbtree.h
	$(CC) $(CFLAGS) -c $<

rbtree_test.o: rbtree_test.c rbtree.c rbtree.h test.h
	$(CC) $(CFLAGS) -c $<

chunky_str.o: chunky_str.c chunky_str.h list.h
	$(CC) $(CFLAGS) -c $<

chunky_str_test.o: chunky_str_test.c chunky_str.h
	$(CC) $(CFLAGS) -c $<

binary_heap.o: binary_heap.c binary_heap.h
	$(CC) $(CFLAGS) -c $<

binary_heap_test.o: binary_heap_test.c binary_heap.h
	$(CC) $(CFLAGS) -c $<

cuckoo_htable.o: cuckoo_htable.c cuckoo_htable.h
	$(CC) $(CFLAGS) -c $<

radix_tree.o: radix_tree.c radix_tree.h
	$(CC) $(CFLAGS) -c $<

radix_tree_test.o: radix_tree_test.c radix_tree.h
	$(CC) $(CFLAGS) -c $<