SRCS := $(wildcard *.cpp)


all:
	g++ -std=c++20 -I../include -lgtest -lgtest_main -pthread test.cpp -o test

format:
	clang-format -i $(SRCS)
