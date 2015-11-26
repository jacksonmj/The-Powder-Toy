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

#include <string>

class LIFE_Rule
{
protected:
	// States are kind of how long until it dies, normal ones use two states(living,dead) for others the intermediate states live but do nothing
	int statesCount;
	// indexed by number of neighbours, bitmask 1=survive 2=born
	char rule[9];
	void addB(std::string b);
	void addS(std::string s);	
	void parseRuleStrings(std::string b, std::string s);
public:
	int states()
	{
		return statesCount;
	}
	bool born(int neighbours)
	{
		return rule[neighbours]&2;
	}
	bool survive(int neighbours)
	{
		return rule[neighbours]&1;
	}
	std::string getRuleString();
	LIFE_Rule(std::string b, std::string s, int states_=2);
	LIFE_Rule(std::string ruleString);
};

#endif
