# Makefile for CFA library and examples

# Input / output directories
SRC_DIR=src
BLD_DIR=build
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

$(CFA_LIB) : $(CFA_SRC)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SFLAGS) -lnetcdf $^ -o $(LIB_DIR)/$@

test_% : $(TST_DIR)/test_%.c $(CFA_LIB)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(LFLAGS) $< -o $(BLD_DIR)/$@

tests : test_cfa test_cfa_dim test_cfa_example test_cfa_mem test_cfa_load test_cfa_var
	build/test_cfa
	build/test_cfa_dim
	build/test_cfa_example
	build/test_cfa_mem
	build/test_cfa_var
	build/test_cfa_load

clean :
	rm $(LIB_DIR)/*
	rm $(BLD_DIR)/*
