CXX ?= c++
CXXFLAGS ?= -O2 -std=c++17
OPENMP ?= 0
OPENMP_CXX ?= $(if $(wildcard /opt/homebrew/bin/g++-15),/opt/homebrew/bin/g++-15,)

ifeq ($(OPENMP),1)
	ifeq ($(CXX),c++)
		ifneq ($(OPENMP_CXX),)
			CXX := $(OPENMP_CXX)
		endif
	endif
	CXXFLAGS += -fopenmp
endif

SRC = main.cpp board.cpp fen.cpp pgn.cpp movegen.cpp evaluate.cpp search.cpp parallel.cpp utils.cpp
OUT = chess_hpc

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT)

.PHONY: all clean

clean:
	rm -f $(OUT)
