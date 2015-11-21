
#include <ctime>
#include <string>
#include <stdexcept>
#include <iostream>
#include <iterator>
#include "Format.hpp"

std::string format::URLEncode(std::string source)
{
	std::stringstream ss;

	for (char c: source)
	{
		if ((c>='0' && c<='9') ||
			(c>='a' && c<='z') ||
			(c>='A' && c<='Z'))
			ss << c;
		else
		{
			ss << '%' << hex[c>>4] << hex[c&0xF];
		}
	}

	return ss.str();
}

std::string format::UnixtimeToDate(time_t unixtime, std::string dateFormat)
{
	struct tm * timeData;
	char buffer[128];

	timeData = localtime(&unixtime);

	strftime(buffer, 128, dateFormat.c_str(), timeData);
	return std::string(buffer);
}

std::string format::UnixtimeToDateMini(time_t unixtime)
{
	time_t currentTime = time(NULL);
	struct tm currentTimeData = *localtime(&currentTime);
	struct tm timeData = *localtime(&unixtime);

	if(currentTimeData.tm_year != timeData.tm_year)
	{
		return UnixtimeToDate(unixtime, "%b %Y");
	}
	else if(currentTimeData.tm_mon != timeData.tm_mon || currentTimeData.tm_mday != timeData.tm_mday)
	{
		return UnixtimeToDate(unixtime, "%d %B");
	}
	else
	{
		return UnixtimeToDate(unixtime, "%H:%M:%S");
	}
}

std::string format::CleanString(std::string dirtyString, bool asciiOnly, bool stripColour, bool stripNewlines, bool numericOnly)
{
	for (size_t i = 0; i < dirtyString.size(); i++)
	{
		switch(dirtyString[i])
		{
		case '\b':
			if (stripColour)
			{
				dirtyString.erase(i, 2);
				i--;
			}
			else
				i++;
			break;
		case '\x0E':
			if (stripColour)
			{
				dirtyString.erase(i, 1);
				i--;
			}
			break;
		case '\x0F':
			if (stripColour)
			{
				dirtyString.erase(i, 4);
				i--;
			}
			else
				i += 3;
			break;
		case '\r':
		case '\n':
			if (stripNewlines)
				dirtyString[i] = ' ';
			break;
		default:
			if (numericOnly && (dirtyString[i] < '0' || dirtyString[i] > '9'))
			{
				dirtyString.erase(i, 1);
				i--;
			}
			// if less than ascii 20 or greater than ascii 126, delete
			else if (asciiOnly && (dirtyString[i] < ' ' || dirtyString[i] > '~'))
			{
				dirtyString.erase(i, 1);
				i--;
			}
			break;
		}
	}
	return dirtyString;
}

