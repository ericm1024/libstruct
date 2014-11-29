CXX = /opt/intel/bin/icc

CXXFLAGS = -g -std=c99 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=199309 -O2 $(EXTRA_CXXFLAGS)

TARGETS = llist_test llist_bench

# phony targets

all: $(TARGETS)

clean:
	rm -f $(TARGETS)
	rm -rf *.o *~

test: llist_test
	valgrind --leak-check=full ./llist_test

bench: llist_bench
	./llist_bench

# programs

llist_test: llist.o llist_test.o
	$(CXX) $(CXXFLAGS) -o llist_test $^

llist_bench: llist.o llist_bench.o
	$(CXX) $(CXXFLAGS) -lrt -o llist_bench $^

# object files

llist_test.o: llist_test.c llist.h
	$(CXX) $(CXXFLAGS) -c llist_test.c

llist_bench.o: llist_bench.c llist.h
	$(CXX) $(CXXFLAGS) -c llist_bench.c

llist.o: llist.c llist.h
	$(CXX) $(CXXFLAGS) -c llist.c
