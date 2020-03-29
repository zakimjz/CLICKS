CC = g++
CFLAGS = -O3
OBJS =
LIBS =

default: mconvert

clean:
	rm -rf *~ *.o mconvert

mconvert: convert.cpp
	$(CC) $(CFLAGS) -o mconvert convert.cpp $(OBJS) $(LIBS)

.SUFFIXES: .o .cpp

.cpp.o:
	$(CC) $(CFLAGS) -c $<

