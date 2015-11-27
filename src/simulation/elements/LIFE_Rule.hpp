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

#ifndef simulation_elements_LIFE_Rule_H
#define simulation_elements_LIFE_Rule_H

#include "common/tpt-stdint.h"
#include <string>

class LIFE_Rule
{
protected:
	uint16_t ruleB, ruleS;
	// States are kind of how long until it dies, normal ones use two states(living,dead) for others the intermediate states live but do nothing
	int liveStatesCount;
	void addB(std::string b);
	void addS(std::string s);
	void setStates(int st);
	void clearRule();
	void parseRuleStrings(std::string b, std::string s);
public:
	int states()
	{
		return liveStatesCount+1;
	}
	int liveStates()
	{
		return liveStatesCount;
	}
	bool born(int neighbours)
	{
		return ruleB & (1<<neighbours);
	}
	bool survive(int neighbours)
	{
		return ruleS & (1<<neighbours);
	}
	std::string getRuleString();
	LIFE_Rule(std::string b, std::string s, int states_=2);
	LIFE_Rule(std::string ruleString);
};

#endif
