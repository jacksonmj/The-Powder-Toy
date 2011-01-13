#include <powder.h>

int update_HPRT(UPDATE_FUNC_ARGS) {
	if (legacy_enable) return 0;
	int r, rx, ry, h_count = 0;
	parts[i].tmp = (int)((parts[i].temp-73.15f)/100+1);
	float c_heat = 0.0f;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if ((r>>8)>=NPART || !r)
					continue;
				if ((r&0xFF)!=PT_HPRT)
				{
					c_heat += parts[r>>8].temp;
					h_count++;
					if (hprt[parts[i].tmp][0]>-1)
						parts[r>>8].temp = (hprt[parts[i].tmp][0]+parts[r>>8].temp)/2.0f;
				}
			}
	hprt[parts[i].tmp][1] += c_heat;
	hprt[parts[i].tmp][2] += h_count;
	return 0;
}
