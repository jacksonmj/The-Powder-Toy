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
#include "simulation/ElemDataShared.h"
#include "simulation/ElemDataSim.h"
#include "LIFE.hpp"
#include "common/Format.hpp"

#include <algorithm>
#include <sstream>
#include <cctype>

//New IDs for GOL types
#define NGT_GOL 0
#define NGT_HLIF 1
#define NGT_ASIM 2
#define NGT_2x2 3
#define NGT_DANI 4
#define NGT_AMOE 5
#define NGT_MOVE 6
#define NGT_PGOL 7
#define NGT_DMOE 8
#define NGT_34 9
#define NGT_LLIF 10
#define NGT_STAN 11
#define NGT_SEED 12
#define NGT_MAZE 13
#define NGT_COAG 14
#define NGT_WALL 15
#define NGT_GNAR 16
#define NGT_REPL 17
#define NGT_MYST 18
#define NGT_LOTE 19
#define NGT_FRG2 20
#define NGT_STAR 21
#define NGT_FROG 22
#define NGT_BRAN 23

gol_menu gmenu[NGOL] =
{
	{"GOL",		PIXPACK(0x0CAC00), 0, "Game Of Life B3/S23"},
	{"HLIF",	PIXPACK(0xFF0000), 1, "High Life B36/S23"},
	{"ASIM",	PIXPACK(0x0000FF), 2, "Assimilation B345/S4567"},
	{"2x2",		PIXPACK(0xFFFF00), 3, "2x2 B36/S125"},
	{"DANI",	PIXPACK(0x00FFFF), 4, "Day and Night B3678/S34678"},
	{"AMOE",	PIXPACK(0xFF00FF), 5, "Amoeba B357/S1358"},
	{"MOVE",	PIXPACK(0xFFFFFF), 6, "'Move' particles. Does not move things.. it is a life type B368/S245"},
	{"PGOL",	PIXPACK(0xE05010), 7, "Pseudo Life B357/S238"},
	{"DMOE",	PIXPACK(0x500000), 8, "Diamoeba B35678/S5678"},
	{"34",		PIXPACK(0x500050), 9, "34 B34/S34"},
	{"LLIF",	PIXPACK(0x505050), 10, "Long Life B345/S5"},
	{"STAN",	PIXPACK(0x5000FF), 11, "Stains B3678/S235678"},
	{"SEED",	PIXPACK(0xFBEC7D), 12, "B2/S"},
	{"MAZE",	PIXPACK(0xA8E4A0), 13, "B3/S12345"},
	{"COAG",	PIXPACK(0x9ACD32), 14, "B378/S235678"},
	{"WALL",	PIXPACK(0x0047AB), 15, "B45678/S2345"},
	{"GNAR",	PIXPACK(0xE5B73B), 16, "B1/S1"},
	{"REPL",	PIXPACK(0x259588), 17, "B1357/S1357"},
	{"MYST",	PIXPACK(0x0C3C00), 18, "B3458/S05678"},
	{"LOTE",	PIXPACK(0xFF0000), 19, "Behaves kinda like Living on the Edge S3458/B37/4"},
	{"FRG2",	PIXPACK(0x00FF00), 20, "Like Frogs rule S124/B3/3"},
	{"STAR",	PIXPACK(0x0000FF), 21, "Like Star Wars rule S3456/B278/6"},
	{"FROG",	PIXPACK(0x00AA00), 22, "Frogs S12/B34/3"},
	{"BRAN",	PIXPACK(0xCCCC00), 23, "Brian 6 S6/B246/3"}
};

void LIFE_Rule::addB(std::string b)
{
	// born
	for (char c: b)
	{
		if (c>='0' && c<='8')
			rule[c-'0'] |= 2;
	}
}

void LIFE_Rule::addS(std::string s)
{
	// survive
	for (char c: s)
	{
		if (c>='0' && c<='8')
			rule[c-'0'] |= 1;
	}
}

void LIFE_Rule::parseRuleStrings(std::string b, std::string s)
{
	std::fill_n(rule, 9, 0);
	addB(b);
	addS(s);
}

std::string LIFE_Rule::getRuleString()
{
	std::ostringstream ssB, ssS;
	for (int neighbours=0; neighbours<=8; neighbours++)
	{
		if (born(neighbours))
			ssB << format::NumberToString(neighbours);
		if (survive(neighbours))
			ssS << format::NumberToString(neighbours);
	}
	std::string ruleString = "B"+ssB.str() + "/S"+ssS.str();
	if (states()>2)
		ruleString += "/"+format::NumberToString(states());
	return ruleString;
}

LIFE_Rule::LIFE_Rule(std::string b, std::string s, int states_) : statesCount(states_)
{
	// e.g. Rule("3", "23", 2) for GOL B3/S23
	parseRuleStrings(b, s);
}

LIFE_Rule::LIFE_Rule(std::string ruleString)
{
	// e.g. Rule("S3456/B278/6")
	// No order is enforced, and no part of it is compulsory
	std::fill_n(rule, 9, 0);
	statesCount = 2;
	std::istringstream ss(ruleString);
	std::string rulePart;
	char partType;
	while (!ss.eof())
	{
		std::getline(ss, rulePart, '/');
		std::istringstream(format::toLower(rulePart)) >> partType;
		if (partType=='b')
		{
			addB(rulePart);
		}
		else if (partType=='s')
		{
			addS(rulePart);
		}
		else if (std::isdigit(partType))
		{
			statesCount = format::StringToNumber<unsigned int>(rulePart);
		}
	}
}


LIFE_ElemDataShared::LIFE_ElemDataShared(SimulationSharedData *s, int t) : ElemDataShared(s,t)
{
	rules.push_back(LIFE_Rule(""));// rules[0] currently means "no life type", to simplify 0=="no life type" when doing LIFE update
	initDefaultRules();
}

void LIFE_ElemDataShared::initDefaultRules()
{
	rules.push_back(LIFE_Rule("B3/S23"));//GOL
	rules.push_back(LIFE_Rule("B36/S23"));//HLIF
	rules.push_back(LIFE_Rule("B345/S4567"));//ASIM
	rules.push_back(LIFE_Rule("B36/S125"));//2x2
	rules.push_back(LIFE_Rule("B3678/S34678"));//DANI
	rules.push_back(LIFE_Rule("B357/S1358"));//AMOE
	rules.push_back(LIFE_Rule("B368/S245"));//MOVE
	rules.push_back(LIFE_Rule("B357/S238"));//PGOL
	rules.push_back(LIFE_Rule("B35678/S5678"));//DMOE
	rules.push_back(LIFE_Rule("B34/S34"));//34
	rules.push_back(LIFE_Rule("B345/S5"));//LLIF
	rules.push_back(LIFE_Rule("B3678/S235678"));//STAN
	rules.push_back(LIFE_Rule("B2/S"));//SEED
	rules.push_back(LIFE_Rule("B3/S12345"));//MAZE
	rules.push_back(LIFE_Rule("B378/S235678"));//COAG
	rules.push_back(LIFE_Rule("B45678/S2345"));//WALL
	rules.push_back(LIFE_Rule("B1/S1"));//GNAR
	rules.push_back(LIFE_Rule("B1357/S1357"));//REPL
	rules.push_back(LIFE_Rule("B3458/S05678"));//MYST
	rules.push_back(LIFE_Rule("S3458/B37/4"));//LOTE
	rules.push_back(LIFE_Rule("S124/B3/3"));//FRG2
	rules.push_back(LIFE_Rule("S3456/B278/6"));//STAR
	rules.push_back(LIFE_Rule("S12/B34/3"));//FROG
	rules.push_back(LIFE_Rule("S6/B246/3"));//BRAN
}

void Element_LIFE::setType(SimulationSharedData *simSD, particle &p, int type)
{
	if (Element_LIFE::isValidType(simSD, type))
	{
		std::vector<LIFE_Rule> &rules = simSD->elemData<LIFE_ElemDataShared>(PT_LIFE)->rules;
		p.ctype = type;
		p.tmp = rules[type+1].states()-1;
	}
}

void Element_LIFE::setType(Simulation *sim, particle &p, int type)
{
	if (Element_LIFE::isValidType(sim, type))
	{
		std::vector<LIFE_Rule> &rules = sim->elemDataShared<LIFE_ElemDataShared>(PT_LIFE)->rules;
		p.ctype = type;
		p.tmp = rules[type+1].states()-1;
	}
}

bool Element_LIFE::isValidType(SimulationSharedData *simSD, int type)
{
	// "type" is ctype value, with 0 meaning GOL
	// It is NOT rulenum, with 0 meaning none and 1 meaning GOL
	if (type >= 0 && type < (int)simSD->elemData<LIFE_ElemDataShared>(PT_LIFE)->rules.size()-1)
	{
		return true;
	}
	return false;
}

bool Element_LIFE::isValidType(Simulation *sim, int type)
{
	if (type >= 0 && type < (int)sim->elemDataShared<LIFE_ElemDataShared>(PT_LIFE)->rules.size()-1)
	{
		return true;
	}
	return false;
}

void LIFE_ElemDataSim::readLife()
{
	//TODO: maybe this should only loop through active particles
	//go through every particle and set neighbor map
	auto sim = this->sim;// put in stack/register instead of indirect access via this, since used a lot
	std::vector<LIFE_Rule> &rules = sim->elemDataShared<LIFE_ElemDataShared>(PT_LIFE)->rules;
	const PMapCategory lifeCat = PMapCategory::NotEnergy; //or sim->pmap_category(PT_LIFE), but hardcoded value is faster
	int maxRuleNum = rules.size();
	for (int ny=CELL; ny<YRES-CELL; ny++)
	{
		for (int nx=CELL; nx<XRES-CELL; nx++)
		{
			SimPosI pos(nx,ny);
			if (!sim->pmap(pos).count(lifeCat))
			{
				ruleMap[ny][nx] = 0;
				continue;
			}
			int r = sim->pmap(pos).find_one(sim->parts, PT_LIFE, lifeCat);
			if (r>=0)
			{
				int ruleNum = parts[r].ctype+1;
				if (ruleNum<=0 || ruleNum>maxRuleNum)
				{
					sim->part_kill(r);
					ruleMap[ny][nx] = 0;
					continue;
				}
				ruleMap[ny][nx] = ruleNum;
				if (sim->parts[r].tmp == rules[ruleNum].states()-1)
				{
					for (int nnx=-1; nnx<2; nnx++)
					{
						for (int nny=-1; nny<2; nny++)//it will count itself as its own neighbor, which is needed, but will have 1 extra for delete check
						{
							SimPosI npos = sim->pos_wrapMainArea_simple(pos+SimPosDI(nnx,nny));
							if (!sim->pmap(npos).count(lifeCat) || sim->pmap(npos).find_one(sim->parts, PT_LIFE, lifeCat)>=0)
							{
								//the total neighbor count is in 0
								neighbourMap[npos.y][npos.x][0] ++;
								//insert golnum into neighbor table
								for (int i=1; i<9; i++)
								{
									if (!neighbourMap[npos.y][npos.x][i])
									{
										neighbourMap[npos.y][npos.x][i] = (ruleNum<<4)+1;
										break;
									}
									else if((neighbourMap[npos.y][npos.x][i]>>4)==ruleNum)
									{
										neighbourMap[npos.y][npos.x][i]++;
										break;
									}
								}
							}
						}
					}
				}
				else
				{
					parts[r].tmp --;
				}
			}
		}
	}
}

void LIFE_ElemDataSim::updateLife()
{
	//go through every particle again, but check neighbor map, then update particles
	auto sim = this->sim;// put in stack/register, since used a lot
	std::vector<LIFE_Rule> &rules = sim->elemDataShared<LIFE_ElemDataShared>(PT_LIFE)->rules;
	const PMapCategory lifeCat = PMapCategory::NotEnergy;
	for (int ny=CELL; ny<YRES-CELL; ny++)
	{
		for (int nx=CELL; nx<XRES-CELL; nx++)
		{
			SimPosI pos(nx,ny);
			int r = sim->pmap(pos).find_one(sim->parts, PT_LIFE, lifeCat);
			if (sim->pmap(pos).count(lifeCat) && r<0)
				continue;
			int totalNeighbourCount = neighbourMap[ny][nx][0];
			if (totalNeighbourCount)
			{
				int ruleNum = ruleMap[ny][nx];
				if (!sim->pmap(pos).count(lifeCat))
				{
					// Find which type we can try and create
					// A life type can only be created if at least half the LIFE neighbours are of that type, and the number of LIFE neighbours (any type) is correct.
					int threshold = (totalNeighbourCount+1)/2;
					int createRuleNum = 0xFF;
					for (int i=1; i<9; i++)
					{
						if (!neighbourMap[ny][nx][i])
							break;
						int checkRuleNum = (neighbourMap[ny][nx][i]>>4);
						if (rules[checkRuleNum].born(totalNeighbourCount) && (neighbourMap[ny][nx][i]&0xF)>=threshold)
						{
							// Conflict resolution based on ruleNum, where there are two LIFE types, each comprising exactly half the neighbours.
							// (Unfortunately, this makes custom LIFE types a bit more difficult, though not impossible, and can't be changed due to the requirement for compatibility.)
							if (checkRuleNum<createRuleNum)
								createRuleNum = checkRuleNum;
						}
					}
					if (createRuleNum<0xFF)
					{
						int p = sim->part_create(-1, pos, PT_LIFE);
						if (p>=0)
						{
							Element_LIFE::setType(sim->simSD.get(), sim->parts[p], createRuleNum-1);
						}
					}
				}
				else if (!rules[ruleNum].survive(totalNeighbourCount-1))
					//subtract 1 because it counted itself
				{
					if (sim->parts[r].tmp==rules[ruleNum].states()-1)
						sim->parts[r].tmp --;
				}
				for (int i=0; i<9; i++)
					neighbourMap[ny][nx][i] = 0;
			}
			//we still need to kill things with 0 neighbors (higher state life)
			if (r>=0 && sim->parts[r].tmp<=0)
					sim->part_kill(r, pos);
		}
	}
}

void LIFE_ElemDataSim::Simulation_Cleared()
{
	std::fill_n(&neighbourMap[0][0][0], YRES*XRES*9, 0);
	speedCounter = 0;
	generation = 0;
}

void LIFE_ElemDataSim::Simulation_BeforeUpdate()
{
	if (sim->elementCount[PT_LIFE] && ++speedCounter>=speed)
	{
		speedCounter = 0;
		readLife();
		updateLife();
	}
}

LIFE_ElemDataSim::LIFE_ElemDataSim(Simulation *s, int t) :
	ElemDataSim(s,t),
	obs_simCleared(sim->hook_cleared, this, &LIFE_ElemDataSim::Simulation_Cleared),
	obs_simBeforeUpdate(sim->hook_beforeUpdate, this, &LIFE_ElemDataSim::Simulation_BeforeUpdate),
	speed(1)
{
	Simulation_Cleared();
}


class Element_UI_LIFE : public Element_UI
{
public:
	Element_UI_LIFE(Element *el)
		: Element_UI(el)
	{}

	std::string getHUDText(Simulation *sim, int i, bool debugMode)
	{
		int lifeType = sim->parts[i].ctype;
		if (lifeType>=0 && lifeType<NGOLALT)
			return Name + " (" + gmenu[lifeType].name + ")";
		else
			return Name;
	}
};

int LIFE_graphics(GRAPHICS_FUNC_ARGS)
{
	pixel pc;
	if (cpart->ctype==NGT_LOTE)//colors for life states
	{
		if (cpart->tmp==2)
			pc = PIXRGB(255, 128, 0);
		else if (cpart->tmp==1)
			pc = PIXRGB(255, 255, 0);
		else
			pc = PIXRGB(255, 0, 0);
	}
	else if (cpart->ctype==NGT_FRG2)//colors for life states
	{
		if (cpart->tmp==2)
			pc = PIXRGB(0, 100, 50);
		else
			pc = PIXRGB(0, 255, 90);
	}
	else if (cpart->ctype==NGT_STAR)//colors for life states
	{
		if (cpart->tmp==4)
			pc = PIXRGB(0, 0, 128);
		else if (cpart->tmp==3)
			pc = PIXRGB(0, 0, 150);
		else if (cpart->tmp==2)
			pc = PIXRGB(0, 0, 190);
		else if (cpart->tmp==1)
			pc = PIXRGB(0, 0, 230);
		else
			pc = PIXRGB(0, 0, 70);
	}
	else if (cpart->ctype==NGT_FROG)//colors for life states
	{
		if (cpart->tmp==2)
			pc = PIXRGB(0, 100, 0);
		else
			pc = PIXRGB(0, 255, 0);
	}
	else if (cpart->ctype==NGT_BRAN)//colors for life states
	{
		if (cpart->tmp==1)
			pc = PIXRGB(150, 150, 0);
		else
			pc = PIXRGB(255, 255, 0);
	}
	else if (cpart->ctype >= 0 && cpart->ctype < NGOLALT)
	{
		pc = gmenu[cpart->ctype].colour;
	}
	else
	{
		pc = sim->elements[cpart->type].Colour;
	}
	*colr = PIXR(pc);
	*colg = PIXG(pc);
	*colb = PIXB(pc);
	return 0;
}

void LIFE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI_LIFE>();

	elem->Identifier = "DEFAULT_PT_LIFE";
	elem->ui->Name = "LIFE";
	elem->Colour = COLPACK(0x0CAC00);
	elem->ui->MenuVisible = 0;
	elem->ui->MenuSection = SC_LIFE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 9000.0f;
	elem->HeatConduct = 40;
	elem->Latent = 0;
	elem->ui->Description = "Game Of Life! B3/S23";

	elem->State = ST_NONE;
	elem->Properties = TYPE_SOLID|PROP_LIFE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Graphics = &LIFE_graphics;
	simSD->elemData_create<LIFE_ElemDataShared>(t);
	elem->Func_SimInit = &SimInit_createElemData<LIFE_ElemDataSim>;
}


//Old IDs for LIFE types, from when they were separate elements
#define GT_GOL 78
#define GT_HLIF 79
#define GT_ASIM 80
#define GT_2x2 81
#define GT_DANI 82
#define GT_AMOE 83
#define GT_MOVE 84
#define GT_PGOL 85
#define GT_DMOE 86
#define GT_34 87
#define GT_LLIF 88
#define GT_STAN 89
#define GT_SEED 134
#define GT_MAZE 135
#define GT_COAG 136
#define GT_WALL 137
#define GT_GNAR 138
#define GT_REPL 139
#define GT_MYST 140
#define GT_LOTE 142
#define GT_FRG2 143
#define GT_STAR 144
#define GT_FROG 145
#define GT_BRAN 146

std::vector<int> const oldgolTypes =
{{
	GT_GOL,
	GT_HLIF,
	GT_ASIM,
	GT_2x2,
	GT_DANI,
	GT_AMOE,
	GT_MOVE,
	GT_PGOL,
	GT_DMOE,
	GT_34,
	GT_LLIF,
	GT_STAN,
	GT_SEED,
	GT_MAZE,
	GT_COAG,
	GT_WALL,
	GT_GNAR,
	GT_REPL,
	GT_MYST,
	GT_LOTE,
	GT_FRG2,
	GT_STAR,
	GT_FROG,
	GT_BRAN,
}};

