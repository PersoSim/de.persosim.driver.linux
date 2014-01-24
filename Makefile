CC       = cc
CFLAGS   = -g -std=c99 -fpic -w
INCLUDE  = -I. -I/usr/include/PCSC
DEPENDFILE = .depend

#LDFLAGS  = -lm
#LDLIBS  = -lm


OBJ     := $(filter-out %test.o %Test.o, $(patsubst %.c,%.o,$(wildcard *.c)))
LIBNAME := libPersoSim.so
PREFIX   = /usr/local/pcsc

DEFS     = -DPCSC_DEBUG=1 #-DATR_DEBUG=1

all: $(LIBNAME)

# nerate and pull in dependency info
$(DEPENDFILE): *.c
	$(CC) -MM $+ > $(DEPENDFILE)
-include $(DEPENDFILE)


clean:
	rm -f *.o $(LIBNAME) $(DEPENDFILE) hexStringTest

test: hexStringTest
	./hexStringTest

%.so: $(OBJ)
	$(CC) $(CFLAGS) -shared $(OBJ) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< $(INCLUDE) $(DEFS)

% : %.c hexString.o 
	#TODO remove the hardcoded dependecy above
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDE) 

.PHONY: clean test

