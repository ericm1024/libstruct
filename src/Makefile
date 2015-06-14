
OBJS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(wildcard *.c)))


all: $(OBJS)

avl_tree.o: avl_tree.c avl_tree.h bitops.h
	$(CC) $(CFLAGS) -c $< -o $@

bloom.o: bloom.c bloom.h fasthash.h
	$(CC) $(CFLAGS) -c $< -o $@

cuckoo_htable.o: cuckoo_htable.c cuckoo_htable.h fasthash.h util.h
	$(CC) $(CFLAGS) -c $< -o $@

radix_tree.o: radix_tree.c radix_tree.h bitops.h
	$(CC) $(CFLAGS) -c $< -o $@

rbtree.o: rbtree.c rbtree.h bitops.h
	$(CC) $(CFLAGS) -c $< -o $@

# catch all for everything else
$(OBJDIR)/%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@
