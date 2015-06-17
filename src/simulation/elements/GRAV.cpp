/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "simulation/ElementsCommon.h"
#include "simulation/ElemDataSim.h"

class GRAV_ElemDataSim : public ElemDataSim
{
public:
	float col_r, col_g, col_b;
	float col2_r, col2_g, col2_b;
private:
	Observer_ClassMember<GRAV_ElemDataSim> obs_simCleared, obs_simBeforeUpdate;
	int tickVal;
public:
	int tick()
	{
		return tickVal;
	}
	void tickToColour(int x, float &r, float &g, float &b)
	{
		x = x%180;
		if (x<60)
		{
			r = 60-x;
			g = 0;
			b = x;
		}
		else if (x<120)
		{
			r = 0;
			g = x-60;
			b = 120-x;
		}
		else
		{
			r = x-120;
			g = 180-x;
			b = 0;
		}
	}
	void tick(int newVal)
	{
		tickVal = newVal%180;
		tickToColour(tickVal, col_r, col_g, col_b);
		tickToColour(tickVal*2, col2_r, col2_g, col2_b);
	}

	void randomTick()
	{
		tick(sim->rng.randInt<0,359>());
	}
	void incTick()
	{
		tick(tick()+1);
	}

	GRAV_ElemDataSim(Simulation *s, int t) :
		ElemDataSim(s, t),
		obs_simCleared(sim->hook_cleared, this, &GRAV_ElemDataSim::randomTick),
		obs_simBeforeUpdate(sim->hook_beforeUpdate, this, &GRAV_ElemDataSim::incTick)
	{
		tick(0);
	}
};

int GRAV_graphics(GRAPHICS_FUNC_ARGS)
{
	auto ed = sim->elemData<GRAV_ElemDataSim>(PT_GRAV);

	float r=20, g=20, b=20;

	if (cpart->vx>0)
	{
		r += (cpart->vx)*ed->col_r;
		g += (cpart->vx)*ed->col_g;
		b += (cpart->vx)*ed->col_b;
	}
	else
	{
		r -= (cpart->vx)*ed->col_b;
		g -= (cpart->vx)*ed->col_r;
		b -= (cpart->vx)*ed->col_g;
	}

	if (cpart->vy>0)
	{
		r += (cpart->vy)*ed->col_g;
		g += (cpart->vy)*ed->col_b;
		b += (cpart->vy)*ed->col_r;
	}
	else
	{
		r -= (cpart->vy)*ed->col2_r;
		g -= (cpart->vy)*ed->col2_g;
		b -= (cpart->vy)*ed->col2_b;
	}

	*colr = r;
	*colg = g;
	*colb = b;
	return 0;
}

void GRAV_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_GRAV";
	elem->ui->Name = "GRAV";
	elem->Colour = COLPACK(0xFFE0A0);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 10;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 85;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->ui->Description = "Very light dust. Changes colour based on velocity.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Graphics = &GRAV_graphics;
	elem->Func_SimInit = &SimInit_createElemData<GRAV_ElemDataSim>;
}

