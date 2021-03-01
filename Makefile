CFLAGS = -Wall -Wextra -Wpedantic -Werror -Wno-unused-function \
	-Wno-nullability-extension -Wno-nullability-completeness -Wno-expansion-to-defined -Wno-newline-eof \
	-g -fsanitize=undefined,address

FLAGS_GL = -lGL -lGLU
ifeq "$(shell uname)" "Darwin"
	FLAGS_GL = -framework OpenGL
endif

main: main.c object.o object.h obj.o obj.h mtl.o mtl.h matvec.o matvec.h gl.h
	cc main.c object.o obj.o mtl.o matvec.o -o main -lm $(FLAGS_GL) `pkg-config --cflags glew sdl2 SDL2_image` `pkg-config --libs glew sdl2 SDL2_image` $(CFLAGS)

object.o: object.c object.h obj.h matvec.h gl.h
	cc -c object.c -o object.o $(CFLAGS)

obj.o: obj.c obj.h mtl.h
	cc -c obj.c -o obj.o $(CFLAGS)

mtl.o: mtl.c mtl.h
	cc -c mtl.c -o mtl.o `pkg-config --cflags SDL2_image` $(CFLAGS)

matvec.o: matvec.c matvec.h
	cc -c matvec.c -o matvec.o $(CFLAGS)
