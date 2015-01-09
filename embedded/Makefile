CXX = gcc
OPTFLAGS = -O3
CXXFLAGS = -g -std=gnu99 -Wall -Wextra $(OPTFLAGS)

clean:
	rm -f $(TARGETS)
	rm -rf *.o *~

test: test.o
	$(CXX) $(CXXFLAGS) -o $@ $^

assembly: test.c list.h
	$(CXX) $(CXXFLAGS) -c -fverbose-asm -Wa,-adhln $< > test.lst

test.o: test.c list.h
	$(CXX) $(CXXFLAGS) -c $<
