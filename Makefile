# Makefile for CFA library and examples

# Input / output directories
SRC_DIR=src
BLD_DIR=build
TST_DIR=test
LIB_DIR=lib

# CFA sources
CFA_SRC=$(SRC_DIR)/cfa_*.c $(SRC_DIR)/parsers/cfa_*.c
CFA_LIB=libcfa.so

# C flags
CC = gcc
CFLAGS=-I$(SRC_DIR) -std=c11
DEBUGFLAGS=-O0 -D_DEBUG -Wall -Wextra

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

test_cfa_% : $(TST_DIR)/test_cfa_%.c $(CFA_LIB)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(LFLAGS) $< -o $(BLD_DIR)/$@

tests : test_cfa_container test_cfa_dim test_cfa_example test_cfa_mem test_cfa_var
	build/test_cfa_container
	build/test_cfa_dim
	build/test_cfa_example
	build/test_cfa_mem
	build/test_cfa_var

clean :
	rm $(LIB_DIR)/*
	rm $(BLD_DIR)/*
