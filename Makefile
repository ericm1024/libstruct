CXX = gcc

OPTFLAGS = -02
CXXFLAGS = -g -std=c99 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=199309 $(OPTFLAGS) $(EXTRA_CXXFLAGS)

TARGETS = llist_test llist_bench

# phony targets

all: $(TARGETS)

clean:
	rm -f $(TARGETS)
	rm -rf *.o *~

test: dllist_test
	valgrind --leak-check=full ./dllist_test

bench: dllist_bench
	./dllist_bench

# programs
dllist_test: dllist.o dllist_test.o
	$(CXX) $(CXXFLAGS) -o dllist_test $^

dllist_bench: dllist.o dllist_bench.o
	$(CXX) $(CXXFLAGS) -lrt -o dllist_bench $^

# object files
htable.o: htable.c htable.h
	$(CXX) $(CXXFLAGS) -c htable.c

dllist.o: dllist.c dllist.h
	$(CXX) $(CXXFLAGS) -c dllist.c
