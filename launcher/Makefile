# This is a simple makefile that compiles blaster_server

# set the names here to be the names of your source files with the
# .cxx or .cpp replaced by .o
# Be *** SURE *** to put the .o files here rather than the source files

LAUNCHER  = launcher.o

#------------ no need to change between these lines -------------------
CC=gcc
CFLAGS =
LDFLAGS=-l mraa

.SUFFIXES: .c

.c.o:
	@echo "########################################################################################"
	@echo "Building source file: " $<
	$(CC) $(CFLAGS) -c $< -o $@

#------------ no need to change between these lines -------------------


#------------ targets --------------------------------------------
# describe how to create the targets - often there will be only one target

all: launcher

launcher: $(LAUNCHER)
	@echo "########################################################################################"
	@echo "Building launcher..."
	$(CC) $(LAUNCHER) -o launcher $(LDFLAGS)

clean:
	rm -f *.o launcher

#------------ dependencies --------------------------------------------
