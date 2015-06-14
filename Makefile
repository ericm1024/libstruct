# version info 
export MAJOR_VERSION 	= 0
export MINOR_VERSION 	= 1
export REVISION		= 0
export FULL_VERSION 	= $(MAJOR_VERSION).$(MINOR_VERSION).$(REVISION)


# various important library file names
export LIB_NAME 	= libstruct
export SO_LIB_NAME 	= $(LIB_NAME).so.$(MAJOR_VERSION)
export SO_LIB_FULL_NAME = $(LIB_NAME).so.$(FULL_VERSION)
export STATIC_LIB_NAME 	= $(LIB_NAME).a


# directory variables
#     BUILD_ROOT: the root directory of the project
#     OBJDIR:     all objects needed to compile the library go here. this
#                 includes object files or shared libraries from dependencies
#                 this directory is ephemeral and is not part of the git repo
#                 (make rules generate/clean it up)
#     TESTDIR:    source code for tests goes here
#     BINDIR:     binaries and scripts go here
#     DOCDIR:     doxygen-generated doccumentation goes here. ephemeral.
#     LIBDIR:     the actual compiled library ends up here. ephemeral.
#     DEPDIR:     dependencies end up here. currently just an RNG library
#     VPATH:      vpath
export BUILD_ROOT = $(patsubst %/,%, $(dir $(realpath \
				$(lastword $(MAKEFILE_LIST)))))
export SRCDIR 	= $(BUILD_ROOT)/src
export OBJDIR 	= $(BUILD_ROOT)/obj
export TESTDIR 	= $(BUILD_ROOT)/test
export BINDIR 	= $(BUILD_ROOT)/bin
export DOCDIR 	= $(BUILD_ROOT)/doc
export LIBDIR 	= $(BUILD_ROOT)/lib
export DEPDIR 	= $(BUILD_ROOT)/dep
export VPATH 	= $(BUILD_ROOT)/include


# file variables
OBJS 		= $(wildcard $(OBJDIR)/*)
DEP_INCLUDES 	= $(realpath $(addsuffix /include,$(subst Makefile,,\
					$(wildcard $(DEPDIR)/*))))


# currently this library supports building with gcc or clang
OPTFLAGS 	= -O0
INCLUDE 	= -I$(BUILD_ROOT)/include $(addprefix -I,$(DEP_INCLUDES))
LIBS 		= -lm
DEBUG 		= -g
CSTD 		= -std=gnu99
WARN 		= -Wall -Wextra -pedantic
export CC 	= gcc
export CFLAGS 	= -fPIC $(DEBUG) $(CSTD) $(WARN) $(OPTFLAGS) \
			$(INCLUDE) $(LIBS)


# default: make dependencies, shared lib, documnetation, and run tests
.PHONY: all
all: deps shared doc test


# remove ephemeral directories and $(MAKE) clean in all subdirs that have
# makefiles
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(DOCDIR) $(LIBDIR)
	cd $(TESTDIR) && $(MAKE) clean
	cd $(DEPDIR) && $(MAKE) clean


# make necessary ephemeral directories
.PHONY: dirs
dirs:
	test -d $(OBJDIR) || mkdir $(OBJDIR)
	test -d $(LIBDIR) || mkdir $(LIBDIR)


# compile all objects as a shared library
.PHONY: shared
shared: objs
	cd $(OBJDIR)
	$(CC) -shared -Wl,-soname,$(SO_LIB_NAME) \
		-o $(LIBDIR)/$(SO_LIB_FULL_NAME) $(CFLAGS) $(OBJS)
	ln -s $(LIBDIR)/$(SO_LIB_FULL_NAME) $(LIBDIR)/$(SO_LIB_NAME)


# compiled all objects as a static library
.PHONY: static
static: objs
	cd $(LIBDIR)
	$(AR) rcs $(STATIC_LIB_NAME) $(OBJS)


# compile everything needed to compile the library
.PHONY: objs
objs: dirs deps
	cd $(SRCDIR) && $(MAKE)


# compile and run all tests
.PHONY: test
test: shared
	cd $(TESTDIR) && $(MAKE) test


# compile all dependencies
deps: dirs
	cd $(DEPDIR) && $(MAKE)
