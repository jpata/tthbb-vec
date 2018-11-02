OPTS=-Wno-unsequenced -fPIC -Wall -O3
LIBS=-lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -lm -ldl -lROOTDataFrame -lROOTVecOps -rdynamic

#linking to madgraph
LDFLAGS_MG=-L./bin/ -lamp_hmm
CFLAGS_MG=-I./src/madgraph

#final compiler and linker flags
ROOT_CFLAGS=`root-config --cflags` -std=c++11
ROOT_LIBDIR=`root-config --libdir`
CFLAGS=${ROOT_CFLAGS} ${OPTS} -I./interface/ ${CFLAGS_MG}
LDFLAGS=-L${ROOT_LIBDIR} ${LIBS} ${OPTS}

LDFLAGS_LOOPER=${LDFLAGS_MG}

#list of all objects for libraries
LIBAMP_HMM_DEPS = bin/me_hmumu.o bin/rambo.o bin/read_slha.o bin/ProcessGGH.o bin/ProcessQQZ.o bin/Parameters_sm__hgg_plugin_full.o bin/HelAmps_sm__hgg_plugin_full.o
LIBNANOFLOW_DEPS = bin/nanoflow.o
LOOPER_DEPS = bin/nanoflow.o bin/myanalyzers.o bin/looper.o bin/meanalyzer.o

all: bin/looper bin/libnanoflow.so bin/libamp_hmm.so bin/simple_loop bin/df

#objects
bin/%.o: src/madgraph/%.cc
	$(CXX) -c $(CFLAGS) $< -o $@

bin/%.o: src/%.cc
	$(CXX) -c $(CFLAGS) $< -o $@

#libraries
bin/libamp_hmm.so: $(LIBAMP_HMM_DEPS)
	$(CXX) ${CFLAGS} ${LDFLAGS} $(LIBAMP_HMM_DEPS) -shared -o $@

bin/libnanoflow.so: $(LIBNANOFLOW_DEPS)
	$(CXX) ${CFLAGS} ${LDFLAGS} $(LIBNANOFLOW_DEPS) -shared -o bin/libnanoflow.so

#executables
bin/looper: $(LOOPER_DEPS) bin/libnanoflow.so bin/libamp_hmm.so
	$(CXX) ${LDFLAGS} ${LDFLAGS_LOOPER} $(LOOPER_DEPS) -o bin/looper

bin/simple_loop: src/simple_loop.cc
	$(CXX) ${CFLAGS} ${LDFLAGS} src/simple_loop.cc -o bin/simple_loop

bin/df: src/dataframe.cc
	c++ ${CFLAGS} ${LDFLAGS} src/dataframe.cc -o bin/df

#misc
format: ${SRC_FILES} ${HEADER_FILES}
	clang-format -i -style=Google ${SRC_FILES} ${HEADER_FILES}

clean:
	rm -Rf bin/*

run: runA runB

runA:
	DYLD_LIBRARY_PATH=/Users/joosep/Documents/OpenLoops/lib ./bin/looper data/ggh_hmumu/input.json out_ggh.json

runB:
	DYLD_LIBRARY_PATH=/Users/joosep/Documents/OpenLoops/lib ./bin/looper data/dyjets_ll/input.json out_dyjets.json

.PHONY: clean run
