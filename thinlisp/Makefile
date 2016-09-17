USE_READLINE ?=
CFLAGS += -g
LDFLAGS += -g
CC = llvm-gcc

#####################

TESTS = tests/ast_test.c

SOURCES_BASE = reader.h reader.c
SOURCES = $(SOURCES_BASE)


#####################

SRCS = 
OBJS = $(SRCS:%.c=%.o)
BINS = $(OBJS:%.o=%)
OTHER_OBJS = reader.o
OTHER_HDRS = reader.h

GLIB_CFLAGS ?= $(shell pkg-config --cflags glib-2.0)
GLIB_LDFLAGS ?= $(shell pkg-config --libs glib-2.0)

ifeq (,$(USE_READLINE))
RL_LIBRARY ?= edit
else
RL_LIBRARY ?= readline
CFLAGS += -DUSE_READLINE=1
endif

#CFLAGS += $(GLIB_CFLAGS) -I /usr/local/Cellar/readline/6.2.4/include/ 
CFLAGS += $(GLIB_CFLAGS) 
#LDFLAGS += -l$(RL_LIBRARY) $(GLIB_LDFLAGS) -ldl -lffi -L/usr/local/Cellar/readline/6.2.4/lib/
LDFLAGS += -l$(RL_LIBRARY) $(GLIB_LDFLAGS) -ldl -lffi 

#####################

.PHONY: clean

READER_PARTS=reader bistack runtime avl utils list
NVMEM_PARTS=nvmem runtime

OBJ=.
SRC=.
OBJECTS=heap.o reader.o
TEST_OBJECTS=ast_test.o

AVL_SOURCES=bistack avl runtime

$(OBJECTS) : %.o : $(SRC)/%.c
	$(CC) -c -o $(OBJ)/$@ $< $(CFLAGS)

$(TEST_OBJECTS) : %.o : $(SRC)/test/%.c
	$(CC) -c -o $(OBJ)/test/$@ $< $(CFLAGS)

all: test femto

heap_test: heap.c heap.h
	$(CC) $(CFLAGS) -g $< -DHEAP_TEST -o $@ && ./$@ && rm $@

bistack_test: bistack.c bistack.h runtime.c runtime.h heap.c heap.h
	$(CC) $(CFLAGS) -g $(filter %.c,$^) -DBISTACK_TEST -o $@ && ./$@ && rm $@

list_test: list.c list.h bistack.c bistack.h runtime.c runtime.h utils.c utils.h
	$(CC) $(CFLAGS) -g $(filter %.c,$^) -DLIST_TEST -o $@ && ./$@ && rm $@

utils_test: utils.c
	$(CC) $(CFLAGS) -g $< -DUTILS_TEST -o $@ && ./$@ && rm $@

reader_test: $(patsubst %,%.c,$(READER_PARTS)) $(patsubst %,%.h,$(READER_PARTS))
	$(CC) $(CFLAGS) -g $(filter %.c,$^) -DREADER_TEST -o $@ && ./$@ && rm $@

nvmem_test: $(patsubst %,%.c,$(NVMEM_PARTS)) $(patsubst %,%.h,$(NVMEM_PARTS))
	$(CC) $(CFLAGS) -g $(filter %.c,$^) -DNVMEM_TEST -o $@ && ./$@ && rm $@


avl_test: $(patsubst %,%.c,$(AVL_SOURCES)) $(patsubst %,%.h,$(AVL_SOURCES))
	$(CC) $(CLFAGS) -g $(patsubst %,%.c,$(AVL_SOURCES)) -DAVL_TEST -o $@ && ./$@ && rm $@

all_test: heap_test

femto: $(OBJECTS)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f test femto *.o test/*.o
