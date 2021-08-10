# This is the name of your project. It will be used for the final output file.
# If you don't specify any other dependencies a .c file with this name will
# automatically be used as your main file.
TARGET = template

# This variable sets the board you're compiling your project for.
# Right now the only option is wdsp-dev for the development board.
BOARD = wdsp-dev

# Make sure this path points to the libwdsp.mk file in your WhateverDSP dictory.
include WhateverDSP/libwdsp.mk


# Down here you can specify your own additional dependencies if your project
# has more than one .c file.
# For most cases you won't even need to write your own build rules since
# make and the included scripts already have most of them preconfigured.
# So it already knows how to turn your .c and .cpp files into .o files.

# You can set up a list of all the object file you need to compile
#OBJECTS = file1.o file2.o

# And then list all their dependencies
# You can automatically generate those using gcc -MM *.c
#file1.o: file1.c header1.h header2.h
#file2.o: file2.c header2.h

# Then you just need to tell the build script that the target
# depends on those object files.
#$(TARGET): $(OBJECTS)

# If you want to be really neat you can even include a rule to clean up
# those object file when you run make clean.
#.PHONY: clean
#clean:
#	rm -f $(OBJECTS)