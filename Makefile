FLAGS_GL = -lGL

ifeq "$(shell uname)" "Darwin"
	FLAGS_GL = -framework OpenGL
endif

main: main.c
	cc main.c -o main -lm $(FLAGS_GL) `sdl2-config --cflags` `sdl2-config --libs`
