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

#include "Sign.hpp"
#include "simulation/Simulation.h"
#include "common/Format.hpp"
#include <regex>
#include <sstream>
#include <iomanip>
#include <stdexcept>

const int MAXSIGNS = 16;
std::regex sign_Num_Message(R"(\{(.):([[:digit:]]+)\|(.*)\})");
std::regex sign_Text_Message(R"(\{(.):(.*)\|(.*)\})");
std::regex sign_Message(R"(\{(.)\|(.*)\})");

void Sign::rawToParsed()
{
	if (rawText.length()>2)
	{
		std::smatch match;
		switch (rawText[1])
		{
		case 'c':
		case 't':
			if (std::regex_match(rawText, match, sign_Num_Message))
			{
				if (match[1].str()=="c")
					type = Type::Save;
				else if (match[1].str()=="t")
					type = Type::ForumThread;
				else
					break;
				target = match[2].str();
				message = match[3].str();
				return;
			}
			break;
		case 's':
			if (std::regex_match(rawText, match, sign_Text_Message))
			{
				type = Type::SaveSearch;
				target = match[2].str();
				message = match[3].str();
				return;
			}
			break;
		case 'b':
			if (std::regex_match(rawText, match, sign_Message))
			{
				type = Type::Spark;
				target = "";
				message = match[2].str();
				return;
			}
			break;
		}
	}
	type = Type::Plain;
	target = "";
	message = rawText;
}

void Sign::parsedToRaw()
{
	switch (type)
	{
	case Type::Plain:
		rawText = message;
		break;
	case Type::Save:
		rawText = "{c:"+target+"|"+message+"}";
		break;
	case Type::ForumThread:
		rawText = "{t:"+target+"|"+message+"}";
		break;
	case Type::SaveSearch:
		rawText = "{s:"+target+"|"+message+"}";
		break;
	case Type::Spark:
		rawText = "{b|"+message+"}";
		break;
	}
}

std::string Sign::cleanString(std::string dirtyText)
{
	return format::CleanString(dirtyText, true, true, true);
}

Sign::Sign() {}

Sign::Sign(SimPosI pos_, std::string rawText_, Sign::Justification justification_)
	: pos(pos_), justification(justification_)
{
	setRawText(rawText_);
}

std::string Sign::getDisplayedText(Simulation *sim) const
{
	std::string txt = message.substr(0, 45);
	if (type==Type::Plain)
	{
		// TODO: jacob1 suggestion - replace {p|t|aheat} in message with values (needs compatibility code when loading TPT++ saves to insert "Temp:"/"Pressure:" text)
		std::stringstream ss;
		if (txt=="{p}")
		{
			float pressure = 0.0f;
			if (sim->pos_isValid(pos))
				pressure = sim->air.pv(pos);
			ss << "Pressure: " << std::fixed << std::setprecision(2) << pressure;
			txt = ss.str();
		}
		else if (txt=="{aheat}")
		{
			float aheat = sim->ambientTemp;
			if (sim->ambientHeatEnabled && sim->pos_isValid(pos))
				aheat = sim->air.hv(pos);
			ss << "Ambient temp: " << std::fixed << std::setprecision(2) << (aheat-273.15);
			txt = ss.str();
		}
		else if (txt=="{t}")
		{
			float temp = 0.0f;
			if (sim->pos_isValid(pos) && sim->pmap(pos).count())
			{
				// Find the average temperature of all particles in this location
				FOR_SIM_PMAP_POS(globalSim, PMapCategory::All, pos, ri)
				{
					temp += parts[ri].temp;
				}
				temp = temp/sim->pmap(pos).count();
				ss << "Temp: " << std::fixed << std::setprecision(2) << (temp-273.15);
				txt = ss.str();
			}
			else
			{
				txt = "Temp: 0.00";
			}
		}
	}
	return txt;
}

void Sign::getOffset(const int w, const int h, int &x0, int &y0) const
{
	if (justification==Justification::Right)
		x0 = pos.x - w;
	else if (justification==Justification::Left)
		x0 = pos.x;
	else
		x0 = pos.x - w/2;
	// TODO: is 4 correct? check that this gives same padding on top and bottom.
	y0 = (pos.y > h+4) ? pos.y - (h+4) : pos.y + 4;
}

std::string Sign::getRawText() const
{
	return rawText;
}

void Sign::setRawText(std::string newValue)
{
	rawText = cleanString(newValue);
	rawToParsed();
}

Sign::Type Sign::getType() const
{
	return type;
}

void Sign::setType(Sign::Type newValue)
{
	if (newValue>=Type::Plain && newValue<=Type::TYPE_MAX)
	{
		type = newValue;
		parsedToRaw();
	}
	else
	{
		throw std::invalid_argument("Invalid sign type");
	}
}

std::string Sign::getMessage() const
{
	return message;
}

void Sign::setMessage(std::string newValue)
{
	message = cleanString(newValue);
	parsedToRaw();
}

std::string Sign::getTarget() const
{
	return target;
}

void Sign::setTarget(std::string newValue)
{
	if (type==Type::Save || type==Type::ForumThread)
	{
		int id = format::StringToNumber<int>(newValue);
		if (id<=0)
		{
			throw std::invalid_argument("Invalid sign target ID");
		}
		newValue = format::NumberToString(id);
	}
	target = cleanString(newValue);
	parsedToRaw();
}



void Signs::erase(Sign &eraseSign)
{
	remove_if([eraseSign](Sign &sign){
		return (&sign == &eraseSign);
	});
}

void Signs::remove_if(std::function<bool (Sign &)> pred)
{
	erase(std::remove_if(begin(), end(), pred), end());
}

Sign *Signs::add()
{
	if (size()>=MAXSIGNS)
		return nullptr;
	push_back(Sign());
	return &at(size()-1);
}

Sign *Signs::add(SimPosI pos)
{
	Sign *sign = add();
	if (sign)
		sign->pos = pos;
	return sign;
}

