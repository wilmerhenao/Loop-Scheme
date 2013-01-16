
CC=g++

scenario: loop.cpp
	$(CC) -o loop loop.cpp -O2 -lglut -lGL -lGLU
