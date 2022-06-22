# Makefile for CFA-C library and examples

# Input / output directories
SRC_DIR=src
BLD_DIR=build
BLD_EX_DIR=$(BLD_DIR)/examples
TST_DIR=test
LIB_DIR=lib

# CFA sources
CFA_SRC=$(SRC_DIR)/cfa*.c $(SRC_DIR)/parsers/cfa*.c
CFA_LIB=libcfa.so

# C flags (-g for breakpoint debugging)
CC = gcc
CFLAGS=-I$(SRC_DIR) -std=c11
DEBUGFLAGS=-O0 -D_DEBUG -Wall -Wextra -g

# Linker flags for shared library
SFLAGS = -shared -fPIC

# Linker flags for everthing else
LFLAGS = -L$(LIB_DIR) -lcfa -lnetcdf

# make everything
all : $(CFA_LIB)

# Make the output directories
$(LIB_DIR):
	mkdir $(LIB_DIR)

$(BLD_DIR):
	mkdir $(BLD_DIR)

$(BLD_EX_DIR): $(BLD_DIR)
	mkdir $(BLD_DIR)/examples

$(CFA_LIB) : $(CFA_SRC) $(LIB_DIR)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SFLAGS) -lnetcdf $(CFA_SRC) -o $(LIB_DIR)/$@

test_% : $(TST_DIR)/test_%.c $(CFA_LIB) $(BLD_DIR)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(LFLAGS) $< -o $(BLD_DIR)/$@

example% : $(TST_DIR)/examples/example%.c $(CFA_LIB) $(BLD_EX_DIR)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(LFLAGS) $< -o $(BLD_EX_DIR)/$@

tests : test_cfa test_cfa_dim test_cfa_mem test_cfa_var test_cfa_cont
	build/test_cfa
	build/test_cfa_dim
	build/test_cfa_mem
	build/test_cfa_var
	build/test_cfa_cont

clean :
	rm -r $(LIB_DIR)/*
	rm -r $(BLD_DIR)/*
