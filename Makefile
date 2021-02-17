CXX = g++
CPPFLAGS = -Wall -g -std=c++11 -pthread 
BIN_DIR = bin
SRC_DIR = src


all: $(BIN_DIR)/.dirstamp $(BIN_DIR)/secrets

$(BIN_DIR)/secret.o: $(SRC_DIR)/secret.cpp $(SRC_DIR)/secret.h
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(BIN_DIR)/baseModule.o: $(SRC_DIR)/baseModule.cpp $(SRC_DIR)/baseModule.h
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(BIN_DIR)/interactiveModule.o: $(SRC_DIR)/interactiveModule.cpp $(SRC_DIR)/interactiveModule.h $(BIN_DIR)/baseModule.o
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(BIN_DIR)/headlessModule.o: $(SRC_DIR)/headlessModule.cpp $(SRC_DIR)/headlessModule.h $(BIN_DIR)/baseModule.o
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(BIN_DIR)/secrets: $(SRC_DIR)/main.cpp $(BIN_DIR)/secret.o $(BIN_DIR)/headlessModule.o $(BIN_DIR)/interactiveModule.o $(BIN_DIR)/baseModule.o
	$(CXX) $(CPPFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -rf $(BIN_DIR)

$(BIN_DIR)/.dirstamp:
	mkdir -p $(BIN_DIR)
	touch $(BIN_DIR)/.dirstamp
