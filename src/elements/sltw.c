#include <element.h>

int update_SLTW(UPDATE_FUNC_ARGS) {
	int r, rx, ry, trade;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if ((r>>8)>=NPART || !r)
					continue;
				if ((r&0xFF)==PT_SALT && 1>(rand()%10000))
					kill_part(r>>8);
				if ((r&0xFF)==PT_PLNT&&5>(rand()%1000))
					kill_part(r>>8);
				if (((r&0xFF)==PT_RBDM||(r&0xFF)==PT_LRBD) && !legacy_enable && parts[i].temp>(273.15f+12.0f) && 1>(rand()%500))
				{
					part_change_type(i,x,y,PT_FIRE);
					parts[i].life = 4;
				}
			}
	for (trade=0; trade<4; trade++)
	{
		rx = rand()%3-1;
		ry = rand()%3-1;
		if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
		{
			r = pmap[y+ry][x+rx];
			if ((r>>8)>=NPART || !r)
				continue;
			if ((r&0xFF)==PT_SLTW&&(parts[i].life>parts[r>>8].life)&&parts[i].life>0&&parts_avg(r>>8, i, PT_INSL)!=PT_INSL)//diffusion
			{
				int temp = parts[i].life - parts[r>>8].life;
				if (temp>5)
				{
					parts[r>>8].life += temp/2;
					parts[i].life -= temp/2;
				}
			}
		}
	}
	return 0;
}
