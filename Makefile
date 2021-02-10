CFLAGS = -g
FLAGS_GL = -lGL -lGLU

ifeq "$(shell uname)" "Darwin"
	FLAGS_GL = -framework OpenGL
endif

main: main.c obj.o obj.h
	cc main.c obj.o -o main -lm $(FLAGS_GL) -lGLEW `sdl2-config --cflags` `sdl2-config --libs` $(CFLAGS)

obj.o: obj.c obj.h
	cc -c obj.c -o obj.o $(CFLAGS)
