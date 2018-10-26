OPTS=-Wno-unsequenced -fPIC -O2 -Wall
CFLAGS=`root-config --cflags` ${OPTS} -I./interface/
LIBS=-lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -lm -ldl -lROOTDataFrame -lROOTVecOps -rdynamic
#-lEve -lEG -lGeom -lGed -lCore
LDFLAGS=-L`root-config --libdir` ${LIBS} ${OPTS}

all: bin/looper bin/libnanoflow.so bin/simple_loop

SRC_FILES=src/*.cc
HEADER_FILES=interface/*.h

clean:
	rm -Rf bin/*

bin/nanoflow.o: src/nanoflow.cc interface/nanoflow.h interface/nanoevent.h
	c++ ${CFLAGS} -c src/nanoflow.cc -o bin/nanoflow.o

bin/myanalyzers.o: src/myanalyzers.cc interface/myanalyzers.h
	c++ ${CFLAGS} -c src/myanalyzers.cc -o bin/myanalyzers.o

bin/meanalyzer.o: src/meanalyzer.cc interface/myanalyzers.h
	c++ ${CFLAGS} -c src/meanalyzer.cc -o bin/meanalyzer.o

bin/looper.o: src/looper.cc interface/myanalyzers.h
	c++ ${CFLAGS} -c src/looper.cc -o bin/looper.o

bin/looper: bin/nanoflow.o bin/myanalyzers.o bin/looper.o bin/meanalyzer.o
	c++ ${LDFLAGS} bin/nanoflow.o bin/myanalyzers.o bin/looper.o bin/meanalyzer.o -o bin/looper

bin/libnanoflow.so: bin/nanoflow.o bin/myanalyzers.o
	c++ ${CFLAGS} ${LDFLAGS} bin/nanoflow.o bin/myanalyzers.o -shared -o bin/libnanoflow.so

bin/simple_loop: src/simple_loop.cc
	c++ ${CFLAGS} ${LDFLAGS} src/simple_loop.cc -o bin/simple_loop

format: ${SRC_FILES} ${HEADER_FILES}
	clang-format -i -style=Google ${SRC_FILES} ${HEADER_FILES} 
.PHONY: clean
