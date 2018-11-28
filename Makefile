OPTS=-Wno-unsequenced -fPIC -Wall -O3
LIBS=-lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -lm -ldl -lROOTDataFrame -lROOTVecOps -rdynamic

SRC_FILES=src/*.cc
HEADER_FILES=interface/*.h

#final compiler and linker flags
ROOT_CFLAGS=`root-config --cflags`
ROOT_LIBDIR=`root-config --libdir`
CFLAGS=${ROOT_CFLAGS} ${OPTS} -I./interface/
LDFLAGS=-L${ROOT_LIBDIR} ${LIBS} ${OPTS}

all: bin/simple_loop bin/nf

#objects
bin/%.o: src/%.cc
	$(CXX) -c $(CFLAGS) $< -o $@

#executables
bin/nf: bin/nf.o
	$(CXX) ${LDFLAGS} bin/nf.o -o bin/nf

bin/simple_loop: src/simple_loop.cc
	$(CXX) ${CFLAGS} ${LDFLAGS} src/simple_loop.cc -o bin/simple_loop

#misc
format: ${SRC_FILES} ${HEADER_FILES}
	clang-format -i -style=Google ${SRC_FILES} ${HEADER_FILES}

clean:
	rm -Rf bin/*

.PHONY: clean run
