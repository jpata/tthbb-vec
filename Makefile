OPTS=-Wno-unsequenced -fPIC -Wall -O3
LIBS=-lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -lm -ldl -lROOTDataFrame -lROOTVecOps -rdynamic

SRC_FILES=src/*.cc
HEADER_FILES=interface/*.h

#final compiler and linker flags
ROOT_CFLAGS=`root-config --cflags`
ROOT_LIBDIR=`root-config --libdir`
CFLAGS=${ROOT_CFLAGS} ${OPTS} -I./interface/
LDFLAGS=-L${ROOT_LIBDIR} ${LIBS} ${OPTS}


#list of all objects for libraries
LIBNANOFLOW_DEPS = bin/nanoflow.o
LIBANALYZERS_DEPS = bin/myanalyzers.o
LOOPER_DEPS = bin/nanoflow.o bin/myanalyzers.o bin/looper.o

all: bin/looper bin/libnanoflow.so bin/simple_loop bin/libanalyzers.so

#objects
bin/%.o: src/madgraph/%.cc
	$(CXX) -c $(CFLAGS) $< -o $@

bin/%.o: src/%.cc
	$(CXX) -c $(CFLAGS) $< -o $@

bin/libnanoflow.so: $(LIBNANOFLOW_DEPS)
	$(CXX) ${CFLAGS} ${LDFLAGS} $(LIBNANOFLOW_DEPS) -shared -o $@ 

bin/libanalyzers.so: $(LIBANALYZERS_DEPS)
	$(CXX) ${CFLAGS} ${LDFLAGS} -L./bin -lnanoflow $(LIBANALYZERS_DEPS) -shared -o $@

#executables
bin/looper: $(LOOPER_DEPS) bin/libnanoflow.so
	$(CXX) ${LDFLAGS} $(LOOPER_DEPS) -o bin/looper

bin/simple_loop: src/simple_loop.cc
	$(CXX) ${CFLAGS} ${LDFLAGS} src/simple_loop.cc -o bin/simple_loop

#misc
format: ${SRC_FILES} ${HEADER_FILES}
	clang-format -i -style=Google ${SRC_FILES} ${HEADER_FILES}

clean:
	rm -Rf bin/*

.PHONY: clean run
