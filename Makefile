# Author: Ahmed Osama
# Purpose: Makefile for building the dist_search C++ application.
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude -O3 -pthread

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/master.cpp $(SRC_DIR)/worker.cpp $(SRC_DIR)/algorithms.cpp $(SRC_DIR)/socket_utils.cpp
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET = dist_search

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
