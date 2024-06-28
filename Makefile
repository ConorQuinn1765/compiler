SRC_DIR := src
HDR_DIR := header
OBJ_DIR := obj
BIN_DIR := .

EXE := $(BIN_DIR)/parse

SRC := $(wildcard $(SRC_DIR)/*.c)
SRC += $(SRC_DIR)/parser.c $(SRC_DIR)/scanner.c
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJ += $(OBJ_DIR)/parser.o $(OBJ_DIR)/scanner.o

CC       := gcc
CPPFLAGS := -Iheader -MMD -MP
CFLAGS   := -Wall -pedantic -g

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $^ -o $@

$(SRC_DIR)/scanner.c: $(SRC_DIR)/scanner.flex $(HDR_DIR)/parser.h
	flex -o$(SRC_DIR)/scanner.c $(SRC_DIR)/scanner.flex

$(SRC_DIR)/parser.c $(HDR_DIR)/parser.h: $(SRC_DIR)/parser.bison
	bison --defines=$(HDR_DIR)/parser.h --output=$(SRC_DIR)/parser.c $(SRC_DIR)/parser.bison

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(OBJ_DIR) $(EXE) $(SRC_DIR)/parser.c $(HDR_DIR)/parser.h $(SRC_DIR)/scanner.c 

-include $(OBJ:.o=.d)

