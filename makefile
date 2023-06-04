GXX = g++

a.out: main.cpp bitformat.h tap.h tap.cpp
	${GXX} -std=c++23 -Wall -Wextra -Werror -Wconversion -pedantic -O3 main.cpp

run: a.out
	./a.out