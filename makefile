BIN_PREFIX := bin
SRC_DIR := src
INCL :=
OBJ_NAMES := main.o renderer.o
OBJS = $(addprefix $(BIN_DIR)/, $(OBJ_NAMES))
HEADERS := $(wildcard $(SRC_DIR)/*.h)
EXEC = $(BIN_DIR)/vupro
LIBS := -lvulkan -lglfw
override CFLAGS += -std=c++17 -Wall $(INCL) $(OPTIM) $(DEFINES)
LFLAGS :=

.PHONY: all
all:
	$(eval BIN_DIR = $(BIN_PREFIX)/release)
	mkdir -p $(BIN_DIR)
	$(MAKE) -j4 BIN_DIR=$(BIN_DIR) OPTIM=-O2 DEFINES=-'DNDEBUG' $(EXEC)

$(EXEC): $(HEADERS) $(OBJS)
	g++ $(OBJS) $(LFLAGS) $(LIBS) -o $@

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $< $(CFLAGS) -c -o $@

.PHONY: debug
debug:
	$(eval BIN_DIR = $(BIN_PREFIX)/debug)
	mkdir -p $(BIN_DIR)
	$(MAKE) -j4 BIN_DIR=$(BIN_DIR) OPTIM=-O0 LFLAGS=-g3 CFLAGS=-g $(EXEC)

.PHONY: clean
clean:
	rm -rf $(BIN_PREFIX)
