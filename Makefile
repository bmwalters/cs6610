CFLAGS = -g
FLAGS_GL = -lGL -lGLU

ifeq "$(shell uname)" "Darwin"
	FLAGS_GL = -framework OpenGL
endif

main: main.c obj.o obj.h mtl.o mtl.h
	cc main.c obj.o -o main -lm $(FLAGS_GL) -lGLEW `sdl2-config --cflags` `sdl2-config --libs` -lSDL2_image $(CFLAGS)

obj.o: obj.c obj.h mtl.h
	cc -c obj.c -o obj.o $(CFLAGS)

mtl.o: mtl.c mtl.h
	cc -c mtl.c -o mtl.o `sdl2-config --cflags` $(CFLAGS)
