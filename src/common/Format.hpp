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

#ifndef common_Format_h
#define common_Format_h

#include <sstream>
#include <string>

namespace format
{
	const static char hex[] = "0123456789ABCDEF";

	template <typename T> std::string NumberToString(T number)
	{
		std::stringstream ss;
		ss << number;
		return ss.str();
	}

	template <typename T> T StringToNumber(const std::string & text)
	{
		std::stringstream ss(text);
		T number;
		return (ss >> number)?number:0;
	}

	std::string URLEncode(std::string value);
	std::string UnixtimeToDate(time_t unixtime, std::string dateFomat = "%d %b %Y");
	std::string UnixtimeToDateMini(time_t unixtime);
	std::string CleanString(std::string dirtyString, bool asciiOnly, bool stripColour, bool stripNewlines, bool numericOnly = false);
}

#endif
