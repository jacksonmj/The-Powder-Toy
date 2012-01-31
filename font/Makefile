editor: editor.c
	gcc -oeditor -DFONTEDITOR -DSCALE=1 editor.c -lSDL -lm -O3 -ffast-math -march=k8 -Wall -std=c99

editor.exe: editor.c
	gcc -oeditor.exe -DSCALE=1 -DFONTEDITOR editor.c -lmingw32 -lm -lSDLmain -lSDL -O3 -ffast-math -march=k8 -Wall -std=c99 -mwindows
	
packer: packer.c
	gcc -opacker -DFONTEDITOR packer.c -lm -O3 -ffast-math -Wall -std=c99

packer.exe: packer.c
	gcc -opacker.exe -DFONTEDITOR packer.c -lm -O3 -ffast-math -march=k8 -Wall -std=c99

unpacker: unpacker.c ../includes/font.h
	gcc -ounpacker -DFONTEDITOR unpacker.c -lm -O3 -ffast-math -Wall -std=c99
	

unpacker.exe: unpacker.c ../includes/font.h
	gcc -ounpacker.exe -DFONTEDITOR unpacker.c -lm -O3 -ffast-math -march=k8 -Wall -std=c99
	
clean:
	rm -f editor packer unpacker editor.exe packer.exe unpacker.exe
