CC = g++ 
CFLAGS = -O3 -Wno-deprecated 
HDRS = ../eclat/eclat.h ../eclat/timetrack.h ../eclat/calcdb.h ../eclat/eqclass.h ../eclat/stats.h ../eclat/maximal.h ../eclat/chashtable.h timeutil.h KCDataset.h KCUtility.h KCGlobal.h KCCliques.h KCArray.h KCMerge.h KCSelective.h

EOBJS = ../eclat/calcdb.o ../eclat/eqclass.o ../eclat/stats.o ../eclat/maximal.o ../eclat/eclat.o ../eclat/enumerate.o ../eclat/chashtable.o 

COBJS = KCDataset.o KCMain.o KCUtility.o KCCliques.o KCArray.o KCMerge.o KCSelective.o timeutil.o

LIBS =
TARGET = kcdriver 

default: $(TARGET)

clean:
	rm -rf *~ $(COBJS) $(EOBJS) $(TARGET)

kcdriver: $(COBJS) $(EOBJS) $(HDRS)
	$(CC) $(CFLAGS) -o kcdriver $(COBJS) $(EOBJS) $(LIBS)

.SUFFIXES: .o .cpp .cc

.cpp.o:
	$(CC) $(CFLAGS) -o $*.o -c $<

.cc.o:
	$(CC) $(CFLAGS) -o $*.o -c $<
