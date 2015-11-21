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

#ifndef simulation_Sign_h
#define simulation_Sign_h

#include "simulation/Position.hpp"
#include <string>
#include <vector>
#include <functional>

class Simulation;

extern const int MAXSIGNS;

class Sign
{
public:
	enum class Type { Plain=1, Save, Spark, ForumThread, SaveSearch, TYPE_MAX=SaveSearch };
	enum class Justification { Left = 0, Middle = 1, Right = 2, NoJustify = 3 };

protected:
	void rawToParsed();
	void parsedToRaw();
	static std::string cleanString(std::string dirtyText);
	// raw:
	std::string rawText;
	// parsed:
	Type type;
	std::string message, target;

public:
	Sign();
	Sign(SimPosI pos_, std::string rawText_="", Justification justification_=Justification::Middle);

	SimPosI pos;
	Justification justification;
	std::string getDisplayedText(Simulation *sim) const;
	void getOffset(const int w, const int h, int &x0, int &y0) const;

	std::string getRawText() const;
	void setRawText(std::string newValue);

	Type getType() const;
	void setType(Type newValue);
	std::string getMessage() const;
	void setMessage(std::string newValue);
	std::string getTarget() const;
	void setTarget(std::string newValue);
};

class Signs : protected std::vector<Sign>
{
public:
	using vec = std::vector<Sign>;
	using vec::erase;
	using vec::operator[];
	using vec::size;
	using vec::begin;
	using vec::end;
	using vec::rbegin;
	using vec::rend;
	using vec::clear;

	void erase(Sign& eraseSign);
	void remove_if(std::function<bool(Sign&)> pred);
	void eraseArea(SimPosI topLeft, SimPosDI size);
	Sign *add();
	Sign *add(SimPosI pos);
};

#endif 
