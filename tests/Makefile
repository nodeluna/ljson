SRCS := $(wildcard *.cpp)

CC=g++

all:
	$(CC) -std=c++20 -I../include -lgtest -lpthread test.cpp -o test -g -Wall -Wextra -pedantic

format:
	clang-format -i $(SRCS)
