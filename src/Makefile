all :	maze64 maze32 mazebt mazecpp

maze64 : maze64.c
	gcc --std=gnu99 maze64.c -o maze64 -lpthread

maze32 : maze32.c
	gcc --std=gnu99 maze32.c -o maze32 -lpthread

mazebt: mazebackt.c
	gcc --std=gnu99 mazebackt.c -o mazebt -lpthread

mazecpp: maze.cpp
	g++ -std=gnu++11 -pthread maze.cpp -o mazecpp

clean : 
	rm maze32 maze64 mazebt mazecpp
