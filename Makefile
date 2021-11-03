# Makefile for CFA library and examples

# Input / output directories
SRC_DIR=src
BLD_DIR=build
TST_DIR=test
LIB_DIR=lib

# CFA sources
CFA_SRC=$(SRC_DIR)/cfa_*.c
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
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SFLAGS) $^ -o $(LIB_DIR)/$@

test_cfa_% : $(TST_DIR)/test_cfa_%.c $(CFA_LIB)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(LFLAGS) $< -o $(BLD_DIR)/$@

clean :
	rm $(LIB_DIR)/*
	rm $(BLD_DIR)/*
