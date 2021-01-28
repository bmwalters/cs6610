FLAGS_GL = -lGL

ifeq "$(shell uname)" "Darwin"
	FLAGS_GL = -framework OpenGL
endif

main: main.c
	cc main.c -o main `sdl2-config --cflags --libs` $(FLAGS_GL)
