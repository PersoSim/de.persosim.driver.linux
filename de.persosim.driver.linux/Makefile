CC       = cc
CFLAGS   = -g -std=gnu99 -fpic -Werror -fno-stack-protector
INCLUDE  = -I. -I/usr/include/PCSC
DEPENDFILE = .depend

#LDFLAGS  = -lm
#LDLIBS  = -lm


OBJ     := $(filter-out %test.o %Test.o, $(patsubst %.c,%.o,$(wildcard *.c)))
LIBNAME := ifdhPersoSim.so
PREFIX   = /usr/local/pcsc

DEFS     = -DPCSC_DEBUG=1 #-DATR_DEBUG=1

all: $(LIBNAME) tests

# gennerate and pull in dependency info
$(DEPENDFILE): *.c *.h
	$(CC) -MM $+ > $(DEPENDFILE)
-include $(DEPENDFILE)

clean:
	rm -f *.o $(LIBNAME) $(DEPENDFILE) *Test

install: $(LIBNAME)
	sudo cp reader.conf /etc/reader.conf.d/persoSim
	sudo cp $(LIBNAME) /usr/lib/pcsc/drivers/serial/

uninstall: 
	sudo rm -f /etc/reader.conf.d/persoSim
	sudo rm -f /usr/lib/pcsc/drivers/serial/$(LIBNAME)

tests: hexStringTest

runtests: tests
	./hexStringTest

%.so: $(OBJ)
	$(CC) $(CFLAGS) -shared $(OBJ) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< $(INCLUDE) $(DEFS)

% : %.c hexString.o 
	#TODO remove the hardcoded dependecy above
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDE) 

.PHONY: clean test

