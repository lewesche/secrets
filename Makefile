CXX = g++
CPPFLAGS = -Wall -g -std=c++11
BIN_DIR = bin
SRC_DIR = src


all: $(BIN_DIR)/.dirstamp $(BIN_DIR)/secrets

$(BIN_DIR)/secret.o: $(SRC_DIR)/secret.cpp $(SRC_DIR)/secret.h
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(BIN_DIR)/secrets: $(SRC_DIR)/main.cpp $(BIN_DIR)/secret.o
	$(CXX) $(CPPFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -rf $(BIN_DIR)

$(BIN_DIR)/.dirstamp:
	mkdir -p $(BIN_DIR)
	touch $(BIN_DIR)/.dirstamp
