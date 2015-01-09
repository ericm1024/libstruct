CXX = gcc
OPTFLAGS = -O0
CXXFLAGS = -g -std=gnu99 -Wall -Wextra $(OPTFLAGS) -pedantic -DDEBUG
TARGETS = flist_test list_test avl_test

all: $(TARGETS)

clean:
	rm -f *.o *~ $(TARGETS)

test: $(TARGETS)
	valgrind --leak-check=full ./flist_test
	valgrind --leak-check=full ./list_test
	valgrind --leak-check=full ./avl_test

flist_test: flist_test.o flist.o
	$(CXX) $(CXXFLAGS) -o $@ $^

list_test: list_test.o list.o
	$(CXX) $(CXXFLAGS) -o $@ $^

avl_test: avl_test.o avl_tree.o test.o
	$(CXX) $(CXXFLAGS) -o $@ $^

flist_test.o: flist_test.c flist.h
	$(CXX) $(CXXFLAGS) -c $<

flist.o: flist.c flist.h flist_types.h
	$(CXX) $(CXXFLAGS) -c $<

list_test.o: list_test.c list.h
	$(CXX) $(CXXFLAGS) -c $<

list.o: list.c list.h list_types.h
	$(CXX) $(CXXFLAGS) -c $<

avl_test.o: avl_test.c avl_tree.h
	$(CXX) $(CXXFLAGS) -c $<

avl_tree.o: avl_tree.c avl_tree.h avl_tree_types.h
	$(CXX) $(CXXFLAGS) -c $<

test.o: test.c test.h
	$(CXX) $(CXXFLAGS) -c $<
