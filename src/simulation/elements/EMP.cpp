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

#include "common/tptmath.h"
#include "common/tpt-stdint.h"
#include "EMP.hpp"
#include "WIRE.hpp"

EMP_ElemDataSim::EMP_ElemDataSim(Simulation *s, int t) :
	ElemDataSim(s,t),
	obs_simCleared(sim->hook_cleared, this, &EMP_ElemDataSim::Simulation_Cleared),
	obs_simAfterUpdate(sim->hook_afterUpdate, this, &EMP_ElemDataSim::Simulation_AfterUpdate)
{
	Simulation_Cleared();
}

int EMP_ElemDataSim::getFlashStrength()
{
	return tptmath::clamp_int(flashStrength, 0, 40);
}

void EMP_ElemDataSim::Simulation_Cleared()
{
	flashStrength = 0;
	triggerCount = 0;
}

class DeltaTempGenerator
{
protected:
	float stepSize;
	unsigned int maxStepCount;
	tptmath::SmallKBinomialGenerator binom;
public:
	DeltaTempGenerator(int n, float p, float tempStep) :
		stepSize(tempStep),
		// hardcoded limit of 10, to avoid massive lag if someone adds a few zeroes to MAX_TEMP
		maxStepCount((TEMP_RANGE/stepSize < 10) ? ((unsigned int)(TEMP_RANGE/stepSize)+1) : 10),
		binom(n, p, maxStepCount)
	{}
	float getDelta(float randFloat)
	{
		// randFloat should be a random float between 0 and 1
		return binom.calc(randFloat) * stepSize;
	}
	void apply(Simulation *sim, particle &p)
	{
		sim->part_add_temp(p, getDelta(sim->rng.randFloat()));
	}
};

void EMP_ElemDataSim::Simulation_AfterUpdate()
{
	flashStrength = tptmath::clamp_int(flashStrength + triggerCount*3, 0, 40);
	flashStrength = tptmath::clamp_int(flashStrength - flashStrength/25 - 2, 0, 40);

	if (!triggerCount)
		return;

	/* Known differences from original one-particle-at-a-time version:
	 * - SPRK that disappears during a frame (such as SPRK with life==0 on that frame) will not cause destruction around it.
	 * - SPRK neighbour effects are calculated assuming the SPRK exists and causes destruction around it for the entire frame (so was not turned into BREL/NTCT partway through). This means mass EMP will be more destructive.
	 * - The chance of a METL particle near sparked semiconductor turning into BRMT within 1 frame is different if triggerCount>2. See comment for prob_breakMETLMore.
	 * - Probability of centre isElec particle breaking is slightly different (1/48 instead of 1-(1-1/80)*(1-1/120) = just under 1/48).
	 */

	particle *parts = sim->parts;

	float prob_changeCentre = tptmath::binomial_gte1(triggerCount, 1.0f/48);
	DeltaTempGenerator temp_centre(triggerCount, 1.0f/100, 3000.0f);

	float prob_breakMETL = tptmath::binomial_gte1(triggerCount, 1.0f/300);
	float prob_breakBMTL = tptmath::binomial_gte1(triggerCount, 1.0f/160);
	DeltaTempGenerator temp_metal(triggerCount, 1.0f/280, 3000.0f);
	/* Probability of breaking from BMTL to BRMT, given that the particle has just broken from METL to BMTL. There is no mathematical reasoning for the numbers used, other than:
	 * - larger triggerCount should make this more likely, so it should depend on triggerCount instead of being a constant probability
	 * - triggerCount==1 should make this a chance of 0 (matching previous behaviour)
	 * - triggerCount==2 should make this a chance of 1/160 (matching previous behaviour)
	 */
	// TODO: work out in a more mathematical way what this should be?
	float prob_breakMETLMore = tptmath::binomial_gte1(triggerCount/2, 1.0f/160);

	float prob_randWIFI = tptmath::binomial_gte1(triggerCount, 1.0f/8);
	float prob_breakWIFI = tptmath::binomial_gte1(triggerCount, 1.0f/16);

	float prob_breakSWCH = tptmath::binomial_gte1(triggerCount, 1.0f/100);
	DeltaTempGenerator temp_SWCH(triggerCount, 1.0f/100, 2000.0f);

	float prob_breakARAY = tptmath::binomial_gte1(triggerCount, 1.0f/60);

	float prob_randDLAY = tptmath::binomial_gte1(triggerCount, 1.0f/70);

	for (int i=0; i<=sim->parts_lastActiveIndex; i++)
	{
		int t = parts[i].type;
		if (t==PT_SPRK || (t==PT_SWCH && parts[i].life!=0 && parts[i].life!=10) || (t==PT_WIRE && !Element_WIRE::wasInactive(parts[i])))
		{
			SimPosI pos = SimPosF(parts[i].x, parts[i].y);
			bool isElec = false;
			if ((t==PT_SPRK && (parts[i].ctype==PT_PSCN || parts[i].ctype==PT_NSCN || parts[i].ctype==PT_PTCT ||
			        parts[i].ctype==PT_NTCT || parts[i].ctype==PT_INST || parts[i].ctype==PT_SWCH)) || t==PT_WIRE || t==PT_SWCH)
			{
				isElec = true;
				temp_centre.apply(sim, parts[i]);
				if (sim->rng.chancef(prob_changeCentre))
				{
					if (sim->rng.chance<2,5>())
						sim->part_change_type(i, pos, PT_BREL);
					else
						sim->part_change_type(i, pos, PT_NTCT);
				}
			}

			/* Alternative: we could build a map of affected areas (two uint8_t[YRES][XRES] arrays, summing number of SPRK neighbours and number of isElec SPRK neighbours for each coord. Or possibly uint16_t, because layering.), then iterate through positions in a separate loop.
			 * This would avoid the possibility of iterating through a single position 24*(number of SPRK layers) times. However, this would mean recalculating the probabilities for each position, which wouldn't necessarily be much faster (especially if the save is using EMP in a sensible manner instead of as a lag generator).
			 */
			for (int ry=-2; ry<=2; ry++)
				for (int rx=-2; rx<=2; rx++)
				{
					SimPosI rPos = pos+SimPosDI(rx,ry);
					if (sim->pos_isValid(rPos) && (rx || ry))
					{
						FOR_SIM_PMAP_POS(sim, PMapCategory::NotEnergy, rPos, ri)
						{
							int rt = parts[ri].type;

							//Some elements should only be affected by wire/swch, or by a spark on inst/semiconductor
							//So not affected by spark on metl, watr etc
							if (isElec)
							{
								switch (rt)
								{
								case PT_METL:
									temp_metal.apply(sim, parts[ri]);
									if (sim->rng.chancef(prob_breakMETL))
									{
										sim->part_change_type(ri, rPos, PT_BMTL);
										if (sim->rng.chancef(prob_breakMETLMore))
										{
											sim->part_change_type(ri, rPos, PT_BRMT);
											sim->part_add_temp(parts[ri], 1000.0f);
										}
									}
									break;
								case PT_BMTL:
									temp_metal.apply(sim, parts[ri]);
									if (sim->rng.chancef(prob_breakBMTL))
									{
										sim->part_change_type(ri, rPos, PT_BRMT);
										sim->part_add_temp(parts[ri], 1000.0f);
									}
									break;
								case PT_WIFI:
									if (sim->rng.chancef(prob_randWIFI))
									{
										//Randomise channel
										sim->part_set_temp(parts[ri],sim->rng.randInt<MIN_TEMP,MAX_TEMP>());
									}
									if (sim->rng.chancef(prob_breakWIFI))
									{
										sim->part_create(ri, rPos, PT_BREL);
										sim->part_add_temp(parts[ri], 1000.0f);
									}
									break;
								default:
									break;
								}
							}
							switch (rt)
							{
							case PT_SWCH:
								if (sim->rng.chancef(prob_breakSWCH))
									sim->part_change_type(ri, rPos, PT_BREL);
								temp_SWCH.apply(sim, parts[ri]);
								break;
							case PT_ARAY:
								if (sim->rng.chancef(prob_breakARAY))
								{
									sim->part_create(ri, rPos, PT_BREL);
									sim->part_add_temp(parts[ri], 1000.0f);
								}
								break;
							case PT_DLAY:
								if (sim->rng.chancef(prob_randDLAY))
								{
									//Randomise delay
									sim->part_set_temp(parts[ri], sim->rng.randInt<0,255>() + 273.15f);
								}
								break;
							default:
								break;
							}
						}
					}
				}
		}
	}

	triggerCount = 0;
}

int EMP_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->life)
	{
		*colr = cpart->life*1.5;
		*colg = cpart->life*1.5;
		*colb = 200-(cpart->life);
		if (*colr>255)
			*colr = 255;
		if (*colg>255)
			*colg = 255;
		if (*colb>255)
			*colb = 255;
		if (*colb<=0)
			*colb = 0;
	}
	return 0;
}

void EMP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_EMP";
	elem->ui->Name = "EMP";
	elem->Colour = COLPACK(0x66AAFF);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->PressureAdd_NoAmbHeat = 0.0f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 3;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 121;
	elem->Latent = 0;
	elem->ui->Description = "Electromagnetic pulse. Breaks activated electronics.";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Graphics = &EMP_graphics;
	elem->Func_SimInit = &SimInit_createElemData<EMP_ElemDataSim>;
}

