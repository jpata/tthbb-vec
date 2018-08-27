OPTS=-Wno-unsequenced -O3 -std=c++14
CFLAGS=`root-config --cflags` ${OPTS} -I./interface/
LDFLAGS=`root-config --cflags  --libs --evelibs` ${OPTS}

all: looper

bin/nanoflow.o: src/nanoflow.cc interface/nanoflow.h
	c++ ${CFLAGS} -c src/nanoflow.cc -o bin/nanoflow.o

bin/myanalyzers.o: src/myanalyzers.cc interface/myanalyzers.h interface/nanoflow.h
	c++ ${CFLAGS} -c src/myanalyzers.cc -o bin/myanalyzers.o

bin/looper.o: src/looper.cc
	c++ ${CFLAGS} -c src/looper.cc -o bin/looper.o

looper: bin/nanoflow.o bin/looper.o bin/myanalyzers.o
	c++ ${LDFLAGS} bin/nanoflow.o bin/looper.o bin/myanalyzers.o -o looper
