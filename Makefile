# Makefile for CFA library and examples

# Input / output directories
SRC_DIR=src
BLD_DIR=build
TST_DIR=test

# Compiler variables
CC = gcc
INCLUDE_DIRS = -I$(SRC_DIR)

DEBUG_FLAGS = -Wno-deprecated -O0 -g -Wall -D_DEBUG -D_GLIBC_DEBUG
RELEASE_FLAGS = -Wno-deprecated -O3 -msse3

#FLAGS = $(RELEASE_FLAGS) $(COMMON_FLAGS)
FLAGS = $(DEBUG_FLAGS) $(COMMON_FLAGS)

# Linker variables
LDFLAGS = $(FLAGS) 
LD = gcc $(LIBRARY_DIRS)
COMMON_LIBS = -lnetcdf

CFA_O = $(BLD_DIR)/cfa_container.o $(BLD_DIR)/cfa_dim.o
	
$(BLD_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(FLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(BLD_DIR)/%.o : $(TST_DIR)/%.c
	$(CC) $(FLAGS) $(INCLUDE_DIRS) -c $< -o $@

test_cfa_dim : $(BLD_DIR)/test_cfa_dim.o $(CFA_O)
	$(LD) $(FLAGS) $(INCLUDE_DIRS) $(CFA_O) $< -o $(BLD_DIR)/$@

test_cfa_container : $(BLD_DIR)/test_cfa_container.o $(CFA_O)
	$(LD) $(FLAGS) $(INCLUDE_DIRS) $(CFA_O) $< -o $(BLD_DIR)/$@

test_cfa_example : $(BLD_DIR)/test_cfa_example.o $(CFA_O)
	$(LD) $(FLAGS) $(INCLUDE_DIRS) $(CFA_O) $< -o $(BLD_DIR)/$@

tests: test_cfa_dim test_cfa_container test_cfa_example

clean :
	rm -r $(BLD_DIR)/*