CFLAGS = -Wall -Wextra -Wpedantic -Werror -Wno-unused-function \
	-Wno-nullability-extension -Wno-nullability-completeness -Wno-expansion-to-defined -Wno-newline-eof \
	-g -fsanitize=undefined,address

FLAGS_GL = -lGL -lGLU
ifeq "$(shell uname)" "Darwin"
	FLAGS_GL = -framework OpenGL
endif

main: main.c obj.o obj.h mtl.o mtl.h
	cc main.c obj.o mtl.o -o main -lm $(FLAGS_GL) `pkg-config --cflags glew sdl2 SDL2_image` `pkg-config --libs glew sdl2 SDL2_image` $(CFLAGS)

obj.o: obj.c obj.h mtl.h
	cc -c obj.c -o obj.o $(CFLAGS)

mtl.o: mtl.c mtl.h
	cc -c mtl.c -o mtl.o `pkg-config --cflags SDL2_image` $(CFLAGS)
