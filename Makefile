SRC_DIR := src
OBJ_DIR := obj

# All source files
SRC := $(wildcard $(SRC_DIR)/*.c)

# All object files
OBJ := $(OBJ_DIR)/y.tab.o $(OBJ_DIR)/lex.yy.o $(OBJ_DIR)/parse.o $(OBJ_DIR)/example.o

# All binaries
BIN := example liso_server echo_client

# C compiler
CC := gcc

# C PreProcessor Flags
CPPFLAGS := -Iinclude

# Compiler flags
CFLAGS := -g -Wall

# Default target
default: all

# All targets
all : example liso_server echo_client

# Example target
example: $(OBJ)
	$(CC) $^ -o $@

# Generate lex.yy.c from lexer.l
$(SRC_DIR)/lex.yy.c: $(SRC_DIR)/lexer.l
	flex -o $@ $^

# Generate y.tab.c from parser.y
$(SRC_DIR)/y.tab.c: $(SRC_DIR)/parser.y
	yacc -d $^
	mv y.tab.c $@
	mv y.tab.h $(SRC_DIR)/y.tab.h

# Object files compilation rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# liso_server target (changed from echo_server)
liso_server: $(OBJ_DIR)/liso_server.o $(OBJ_DIR)/y.tab.o $(OBJ_DIR)/lex.yy.o $(OBJ_DIR)/parse.o
	$(CC) -Werror $^ -o $@

# echo_client target
echo_client: $(OBJ_DIR)/echo_client.o $(OBJ_DIR)/y.tab.o $(OBJ_DIR)/lex.yy.o $(OBJ_DIR)/parse.o
	$(CC) -Werror $^ -o $@

# Create the object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up generated files
clean:
	$(RM) $(OBJ) $(BIN) $(SRC_DIR)/lex.yy.c $(SRC_DIR)/y.tab.* 
	$(RM) -r $(OBJ_DIR)
