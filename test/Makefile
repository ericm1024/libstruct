
TESTS = $(patsubst %.c,%, $(wildcard *_test.c))

.PHONY: all
all: $(TESTS)

.PHONY: test
test: $(TESTS)

.PHONY: runtest
runtest: test
	$(LD_ENVVAR)=$(LD_LIBRARY_PATH):$(LIBDIR) \
		$(BINDIR)/runtests $(TESTS)

.PHONY: clean
clean:
	rm -f $(TESTS)

%_test: %_test.c test.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBDIR)/$(SO_LIB_FULL_NAME)

test.o: test.c test.h
	$(CC) $(CFLAGS) -c $<
