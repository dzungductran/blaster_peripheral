# This is a simple makefile that compiles blaster_server

# set the names here to be the names of your source files with the
# .cxx or .cpp replaced by .o
# Be *** SURE *** to put the .o files here rather than the source files

9DOF  = SFE_LSM9DS0.o 9DOF.o

#------------ no need to change between these lines -------------------
CC=g++
CFLAGS =
LDFLAGS=-l mraa

.SUFFIXES: .cpp

.cpp.o:
	@echo "########################################################################################"
	@echo "Building source file: " $<
	$(CC) $(CFLAGS) -c $< -o $@

#------------ no need to change between these lines -------------------


#------------ targets --------------------------------------------
# describe how to create the targets - often there will be only one target

all: 9DOFd

9DOFd: $(9DOF)
	@echo "########################################################################################"
	@echo "Building 9DOF..."
	$(CC) $(9DOF) -o 9DOFd $(LDFLAGS)

clean:
	rm -f *.o 9DOFd

#------------ dependencies --------------------------------------------
