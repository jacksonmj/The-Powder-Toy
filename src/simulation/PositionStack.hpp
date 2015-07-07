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

#ifndef simulation_PositionStack_h
#define simulation_PositionStack_h

#include "simulation/Config.hpp"
#include <cstdlib>
#include <exception>
#include <type_traits>

class PositionStackOverflowException: public std::exception
{
public:
	PositionStackOverflowException() { }
	virtual const char* what() const throw()
	{
		return "Maximum number of entries in the coordinate stack was exceeded";
	}
	~PositionStackOverflowException() throw() {}
};

template<class CoordType=void, size_t defaultStackLimit = (std::is_same<CoordType, SimPosCell>::value ? XRES/CELL*YRES/CELL : XRES*YRES)>
class PositionStack
{
private:
	CoordType *stack;
	size_t stackSize;
	size_t stackLimit;
public:
	PositionStack(size_t stackLimit_ = defaultStackLimit) :
		stack(nullptr),
		stackSize(0),
		stackLimit(stackLimit_)
	{
		stack = new CoordType[stackLimit];
	}
	~PositionStack()
	{
		delete[] stack;
	}
	/*{
		// Memory does not need to be initialised, so use malloc instead of new
		stack = reinterpret_cast<CoordType*>(malloc(sizeof(CoordType)*stackLimit));
	}
	~PositionStack()
	{
		free(stack);
	}*/
	void push(CoordType c)
	{
		if (stackSize>=stackLimit)
			throw PositionStackOverflowException();
		stack[stackSize] = c;
		stackSize++;
	}
	CoordType pop()
	{
		stackSize--;
		return stack[stackSize];
	}
	size_t size() const
	{
		return stackSize;
	}
	void clear()
	{
		stackSize = 0;
	}
};

template<>
class PositionStack<short>
{
private:
	unsigned short (*stack)[2];
	int stack_size;
	const static int stack_limit = XRES*YRES;
public:
	PositionStack() :
		stack(NULL),
		stack_size(0)
	{
		stack = (unsigned short(*)[2])(new unsigned short[2*stack_limit]);
	}
	~PositionStack()
	{
		if (stack) delete[] stack;
	}
	void push(int x, int y)
	{
		if (stack_size>=stack_limit)
			throw PositionStackOverflowException();
		stack[stack_size][0] = x;
		stack[stack_size][1] = y;
		stack_size++;
	}
	void pop(int& x, int& y)
	{
		stack_size--;
		x = stack[stack_size][0];
		y = stack[stack_size][1];
	}
	int getSize() const
	{
		return stack_size;
	}
	void clear()
	{
		stack_size = 0;
	}
};

#endif 
