#ifdef FONTEDITOR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define INCLUDE_FONTDATA
#include "../includes/font.h"

#define CELLW	12
#define CELLH	10

char xsize=CELLW, ysize=CELLH;
char base=7, top=2;
char font[256][CELLH][CELLW];
char width[256];



char *tag = "(c) 2008 Stanislaw Skowronek";

int main(int argc, char *argv[])
{
	FILE *f;
	int ba = 0, bn, w, c, i, j;
	char *rp;

	memset(font, 0, sizeof(font));

	for (c=0; c<256; c++)
	{
		rp = font_data + font_ptrs[c];
		w = *(rp++);
		width[c] = w;
		bn = 0;
		for (j=0; j<FONT_H; j++)
			for (i=0; i<w; i++)
			{
				if (!bn)
				{
					ba = *(rp++);
					bn = 8;
				}
				font[c][j][i] = ba&3;
				ba >>= 2;
				bn -= 2;
			}
	}

	f = fopen("font.bin", "w");
	fwrite(&xsize, 1, 1, f);
	fwrite(&ysize, 1, 1, f);
	fwrite(&base, 1, 1, f);
	fwrite(&top, 1, 1, f);
	fwrite(width, 1, 256, f);
	fwrite(font, CELLW*CELLH, 256, f);
	fclose(f);

	return 0;
}

#endif
