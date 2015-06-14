.PHONY: all
all: rand

.PHONY: clean
clean:
	cd pcg-c-master && $(MAKE) clean

# pcg-c-master random number generation library
.PHONY: rand
rand:
	cd pcg-c-master && $(MAKE)
	cp pcg-c-master/src/libpcg_random.a $(OBJDIR)
