FLAGS_GL = -lGL

ifeq "$(shell uname)" "Darwin"
	FLAGS_GL = -framework OpenGL
endif

main: main.c obj.o obj.h
	cc main.c obj.o -o main -lm $(FLAGS_GL) `sdl2-config --cflags` `sdl2-config --libs`

obj.o: obj.c obj.h
	cc -c obj.c -o obj.o
