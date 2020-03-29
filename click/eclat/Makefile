CC = g++ 
CFLAGS = -O3
HDRS = eclat.h timetrack.h calcdb.h eqclass.h stats.h\
	maximal.h chashtable.h
OBJS = calcdb.o eqclass.o stats.o maximal.o eclat.o enumerate.o\
	chashtable.o
LIBS = 
TARGET = eclat

default: $(TARGET)

clean:
	rm -rf *~ *.o $(TARGET)

eclat: $(OBJS) $(HDRS)
#	$(CC) $(CFLAGS) -o eclat $(OBJS) $(LIBS)
#	strip $(TARGET)

makebin: 
	$(CC) $(CFLAGS) -o makebin makebin.cc

.SUFFIXES: .o .cpp

.cpp.o:
	$(CC) $(CFLAGS) -c $<


# dependencies
# $(OBJS): $(HDRS)
eclat.o: $(HDRS)
enumerate.o: $(HDRS)
calcdb.o: calcdb.h eclat.h eqclass.h
eqclass.o: eqclass.h eclat.h calcdb.h
maximal.o: maximal.h calcdb.h eqclass.h
stats.o: stats.h
chashtable.o: chashtable.h eclat.h
