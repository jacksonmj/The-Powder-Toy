editor: editor.c
	gcc -oeditor -DFONTEDITOR -DSCALE=1 editor.c -lSDL -lm -O3 -ffast-math -march=k8 -Wall -std=c99

packer: packer.c
	gcc -opacker -DFONTEDITOR packer.c -lm -O3 -ffast-math -Wall -std=c99

unpacker: unpacker.c ../includes/font.h
	gcc -ounpacker -DFONTEDITOR unpacker.c -lm -O3 -ffast-math -Wall -std=c99

clean:
	rm -f editor packer unpacker
