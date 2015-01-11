TARGETS = doc

all: $(TARGETS)
	make -C src/

doc:
	doxygen

clean:
	rm -f *.o *~
	rm -rf $(TARGETS)
