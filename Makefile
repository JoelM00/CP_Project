#source files
SRC = buckets.c

OBJ = $(SRC:.cpp=.o)

OUT = buckets

#include directories
INCLUDES = -I.

#C compiler flags
CCFLAGS = -O0 -Wall -std=c99 -fopenmp -std=c99 -I/share/apps/papi/5.4.1/include

#compiler
CCC = gcc

#libraries
LIBS = -L/share/apps/papi/5.4.1/lib -lm -lpapi

.SUFFIXES: .cpp .c

default: $(OUT)

.cpp.o:
	$(CCC) $(CCFLAGS) $(INCLUDES) -c $< -o $@

.c.o:
	$(CCC) $(CCFLAGS) $(INCLUDES) -c $< -o $@

$(OUT): $(OBJ)
	$(CCC) -o $(OUT) $(CCFLAGS) $(OBJ) $(LIBS)

depend: dep
#
#dep
#	makedepend -- $(CFLAGS) -- $(INCLUDES) $(SRC)

clean:
	rm -f *.o .a *~ Makefile.bak $(OUT) bucketsSeq

run:
	srun --partition=cpar buckets 2 1000 10

runValues:
	srun --partition=cpar buckets $(nThreads) $(tamArray) $(nBuckets)

bucketsSeq:
	gcc -o bucketsSeq -O2 -std=c99 -Wall -I/share/apps/papi/5.4.1/include bucketsSeq.c -L/share/apps/papi/5.4.1/lib -lm -lpapi

runSeq:
	srun --partition=cpar bucketsSeq $(tamArray) $(nBuckets)

papi:
	module load papi/5.4.1
