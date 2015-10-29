# This is a simple makefile that compiles blaster_server

# set the names here to be the names of your source files with the
# .cxx or .cpp replaced by .o
# Be *** SURE *** to put the .o files here rather than the source files

CJSON = cJSON/cJSON.o
BLASTER = blaster_server.o shell_cmd.o find_process.o cpu_usage.o cpu_info.o

#------------ no need to change between these lines -------------------
CC=gcc
CFLAGS = -O0 -g3 -Wall -MMD -MP -D_DEBUG
LDFLAGS=-l bluetooth  -lpthread -lm

.SUFFIXES: .c

.c.o:
	@echo "########################################################################################"
	@echo "Building source file: " $<
	$(CC) $(CFLAGS) -c $< -o $@

#------------ no need to change between these lines -------------------


#------------ targets --------------------------------------------
# describe how to create the targets - often there will be only one target

all: blaster

blaster: $(BLASTER) $(CJSON)
	@echo "########################################################################################"
	@echo "Building blaster_server..."
	$(CC) $(BLASTER) $(CJSON) -o blaster_server $(LDFLAGS)

clean:
	rm -f *.o *.d blaster_server cJSON/*.o cJSON/*.d

#------------ dependencies --------------------------------------------
