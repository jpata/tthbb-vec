OPTS=-Wno-unsequenced -g -std=c++14 -fPIC
CFLAGS=`root-config --cflags` ${OPTS} -I./interface/
LIBS=-lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -lm -ldl -rdynamic
#-lEve -lEG -lGeom -lGed -lCore
LDFLAGS=-L`root-config --libdir` ${LIBS} ${OPTS}

all: looper nanoflow

clean:
	rm -f bin/*.o

bin/nanoflow.o: src/nanoflow.cc interface/nanoflow.h
	c++ ${CFLAGS} -c src/nanoflow.cc -o bin/nanoflow.o

bin/looper.o: src/looper.cc interface/myanalyzers.h
	c++ ${CFLAGS} -c src/looper.cc -o bin/looper.o

looper: bin/nanoflow.o bin/looper.o
	c++ ${LDFLAGS} bin/nanoflow.o bin/looper.o -o looper

nanoflow: bin/nanoflow.o
	c++ ${LDFLAGS} bin/nanoflow.o -shared -o libnanoflow.so

simple_loop: src/simple_loop.cc
	c++ ${CFLAGS} ${LDFLAGS} src/simple_loop.cc -o simple_loop

.PHONY: clean
