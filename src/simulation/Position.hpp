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

#ifndef Simulation_Coords_h
#define Simulation_Coords_h

#include "common/tpt-stdint.h"
#include "common/tptmath.h"
#include "common/Compat.hpp"
#include "simulation/Config.hpp"

#include <cmath>
#include <cstdlib> // integer std::abs
#include <algorithm> // std::max

// Some classes to ease converting between integer and float coords and cell coords

/* Positions within a simulation
 * N.B. Positions with invalid coordinates (negative or outside XRES,YRES) might not behave correctly, e.g.:
 *  - converting SimPosI to SimPosCell truncates towards 0 instead of flooring, so negative coords will not convert correctly. This is deliberate, for speed.
 */
class SimPosF;//float
class SimPosFT;//floats which may need truncating (from double or x87 register length to float length)
class SimPosI;//int
class SimPosCell;//cell (int)

// Changes in position
class SimPosDF;//float
class SimPosDI;//int
class SimPosDCell;//cell (int)
/* Member functions include:
 * length() - length (2-norm) (sqrt(x^2+y^2))
 * length_sq() - squared length (x^2+y^2)
 * length_1() - Manhattan distance (1-norm) (abs(x)+abs(y))
 * maxComponent() - returns abs value of largest component (inf-norm) (max(abs(x),abs(y))
 * unit() - returns a scaled version with length()=1
 * unit_1() - returns a scaled version with length_1()=1 (max component = 1)
 */

// Areas
class SimAreaF;
class SimAreaI;
class SimAreaCell;

// TODO: test effect of different datatypes on speed (may need to add extra SimPosXX_least classes using int_least16_t for storage in arrays, and use int_fast16_t in SimPosXX classes for calculations)
// TODO: test effect of SADD16 on speed on ARM (if not already generated automatically)  (int16_t datatype)
namespace detail
{
using SimPosDT = int16_t;
using SimPosT = int16_t;


// Various mixins, which get added to SimPos classes by inheriting from them (CRTP)

template <class Self>
class SimPos_common_cmp
{
public:
	friend bool operator==(const Self lhs, const Self rhs)
	{ return (lhs.x==rhs.x && lhs.y==rhs.y); }
	friend bool operator!=(const Self lhs, const Self rhs)
	{ return (lhs.x!=rhs.x || lhs.y!=rhs.y); }
};

template <class Self, class Delta>
class SimPos_pos_cmp
{
public:
	bool inArea(const Self topLeft, const Delta size) const
	{
		Self const p(*static_cast<const Self*>(this));
		return (p.x>=topLeft.x && p.y>=topLeft.y && p.x<topLeft.x+size.x && p.y<topLeft.y+size.y);
	}
};

template <class Self, class DiffType>
class SimPos_pos_addsub
{
public:
	friend Self operator+(const Self lhs, const DiffType rhs)
	{ return Self(lhs.x+rhs.x, lhs.y+rhs.y); }
	friend Self operator-(const Self lhs, const DiffType rhs)
	{ return Self(lhs.x-rhs.x, lhs.y-rhs.y); }

	friend Self operator+(const DiffType lhs, const Self rhs)
	{ return rhs+lhs; }

	friend DiffType operator-(const Self lhs, const Self rhs)
	{ return DiffType(lhs.x-rhs.x, lhs.y-rhs.y); }

	friend Self& operator+=(Self& lhs, const DiffType rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		return lhs;
	}
	friend Self& operator-=(Self& lhs, const DiffType rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		return lhs;
	}
};

template <class Self, class DiffType, class PSelf, class PDiffType>
class SimPos_pos_addsub_promote
{
public:
	friend PSelf operator+(const Self lhs, const DiffType rhs)
	{ return PSelf(lhs)+PDiffType(rhs); }
	friend PSelf operator-(const Self lhs, const DiffType rhs)
	{ return PSelf(lhs)-PDiffType(rhs); }

	friend PSelf operator+(const DiffType lhs, const Self rhs)
	{ return rhs+lhs; }

	friend Self& operator+=(Self& lhs, const DiffType rhs)
	{
		lhs = lhs + rhs;
		return lhs;
	}
	friend Self& operator-=(Self& lhs, const DiffType rhs)
	{
		lhs = lhs - rhs;
		return lhs;
	}
};

template <class Self>
class SimPos_diff_addsub
{
public:
	Self operator-() const
	{ return Self(-static_cast<Self*>(this)->x, -static_cast<Self*>(this)->y); };
	friend Self operator+(const Self lhs, const Self rhs)
	{ return Self(lhs.x+rhs.x, lhs.y+rhs.y); }
	friend Self operator-(const Self lhs, const Self rhs)
	{ return Self(lhs.x-rhs.x, lhs.y-rhs.y); }

	friend Self& operator+=(Self& lhs, const Self rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		return lhs;
	}
	friend Self& operator-=(Self& lhs, const Self rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		return lhs;
	}
};

template <class Self, class Other, class PromoteTo>
class SimPos_diff_addsub_promote
{
public:
	friend PromoteTo operator+(const Self lhs, const Other rhs)
	{ return PromoteTo(lhs)+PromoteTo(rhs); }
	friend PromoteTo operator-(const Self lhs, const Other rhs)
	{ return PromoteTo(lhs)-PromoteTo(rhs); }
	friend PromoteTo operator-(const Other lhs, const Self rhs)
	{ return PromoteTo(lhs)-PromoteTo(rhs); }

	friend PromoteTo operator+(const Other lhs, const Self rhs)
	{ return rhs+lhs; }

	friend Self& operator+=(Self& lhs, const Other rhs)
	{
		lhs = lhs + rhs;
		return lhs;
	}
	friend Self& operator-=(Self& lhs, const Other rhs)
	{
		lhs = lhs - rhs;
		return lhs;
	}
};


template <class Self, class ScalarT, class Ret>
class SimPos_diff_mul
{
public:
	friend Self& operator*=(Self& v, const ScalarT s)
	{
		v = v*s;
		return v;
	}
	// Promote Self to Ret when multiplied by value of type ScalarT (e.g. SimPosDI * float returns a SimPosDF)
	friend Ret operator*(const Self v, const ScalarT s)
	{ return Ret(v)*s; }
	friend Ret operator*(const ScalarT s, const Self v)
	{ return v*s; }
};

template <class Self, class ScalarT>
class SimPos_diff_mul<Self, ScalarT, Self>
{
public:
	friend Self& operator*=(Self& v, const ScalarT s)
	{
		v = v*s;
		return v;
	}
	friend Self operator*(const Self v, const ScalarT s)
	{ return Self(v.x*s, v.y*s); }
	friend Self operator*(const ScalarT s, const Self v)
	{ return v*s; }
};

template <class Self, class ScalarT, class Ret>
class SimPos_diff_div
{
public:
	friend Self& operator/=(Self& v, const ScalarT s)
	{
		v = v/s;
		return v;
	}
	// Promote Self to Ret when divided by value of type ScalarT (e.g. SimPosDI / float returns a SimPosDF)
	friend Ret operator/(const Self v, const ScalarT s)
	{ return Ret(v)/s; }
};

template <class Self, class ScalarT>
class SimPos_diff_div<Self, ScalarT, Self>
{
public:
	friend Self& operator/=(Self& v, const ScalarT s)
	{
		v = v/s;
		return v;
	}
	friend Self operator/(const Self v, const ScalarT s)
	{ return Self(v.x/s, v.y/s); }
};

template <class Self, class DF=SimPosDF>// DF as template parameter because SimPosDF hasn't been fully defined yet, putting it as a parameter means it doesn't need to be defined until this template is instantiated
class SimPos_diff_misc
{
public:
	float length() const
	{ return std::sqrt(float(static_cast<const Self*>(this)->length_sq())); }
	DF unit() const
	{
		DF df(*static_cast<const Self*>(this));
		float l = static_cast<const Self*>(this)->length();
		return df/l;
	}
	DF unit_1() const
	{
		DF df(*static_cast<const Self*>(this));
		float l = static_cast<const Self*>(this)->maxComponent();
		return df/l;
	}
};


template <class Self, class PosType, class DiffType>
class SimArea_common
{
public:
	friend bool operator==(const Self lhs, const Self rhs)
	{ return (lhs.topLeft==rhs.topLeft && lhs.size==rhs.size); }
	friend bool operator!=(const Self lhs, const Self rhs)
	{ return (lhs.topLeft!=rhs.topLeft || lhs.size!=rhs.size); }

	void setCorners(const PosType topLeft_, const PosType bottomRight_)
	{
		Self& a(*static_cast<Self*>(this));
		a.topLeft = topLeft_;
		a.size = bottomRight_-topLeft_ + DiffType(1,1);
	}
	// Returns the bottom-right-most point that is inside the area
	PosType bottomRight() const
	{
		Self const a(*static_cast<const Self*>(this));
		return a.topLeft+a.size - DiffType(1,1);
	}

	// Position bounds for use in loops (x=begin.x; x<end.x; x++)
	PosType posBegin() const
	{
		Self const a(*static_cast<const Self*>(this));
		return a.topLeft;
	}
	PosType posEnd() const
	{
		Self const a(*static_cast<const Self*>(this));
		return a.topLeft+a.size;
	}

	bool contains(PosType p) const
	{
		Self const a(*static_cast<const Self*>(this));
		return (p.x>=a.topLeft.x && p.y>=a.topLeft.y && p.x<a.topLeft.x+a.size.x && p.y<a.topLeft.y+a.size.y);
	}
	bool contains(Self a) const
	{
		return contains(a.topLeft) && contains(a.bottomRight());
	}
};



}


// Actual SimPos classes

class SimPosDF :
	public detail::SimPos_common_cmp<SimPosDF>,
	public detail::SimPos_diff_addsub<SimPosDF>,
	public detail::SimPos_diff_addsub_promote<SimPosDF, SimPosDCell, SimPosDF>,
	public detail::SimPos_diff_addsub_promote<SimPosDF, SimPosDI, SimPosDF>,
	public detail::SimPos_diff_mul<SimPosDF, float, SimPosDF>,
	public detail::SimPos_diff_div<SimPosDF, float, SimPosDF>,
	public detail::SimPos_diff_misc<SimPosDF>
{
public:
	using T = float;
	T x, y;
	constexpr SimPosDF() : x(0), y(0) {}
	constexpr SimPosDF(float x_, float y_) : x(x_), y(y_) {}
	constexpr SimPosDF(const SimPosDI d);
	constexpr SimPosDF(const SimPosDCell d);
	static constexpr SimPosDF zero() { return SimPosDF(0,0); }

	constexpr T length_sq() const { return x*x+y*y; }
	T length_1() const { return std::abs(x) + std::abs(y); }
	T maxComponent() const { return std::max(std::abs(x), std::abs(y)); }
};


class SimPosDI :
	public detail::SimPos_common_cmp<SimPosDI>,
	public detail::SimPos_diff_addsub<SimPosDI>,
	public detail::SimPos_diff_addsub_promote<SimPosDI, SimPosDCell, SimPosDI>,
	public detail::SimPos_diff_mul<SimPosDI, int, SimPosDI>,
	public detail::SimPos_diff_mul<SimPosDI, float, SimPosDF>,
	public detail::SimPos_diff_div<SimPosDI, float, SimPosDF>,
	public detail::SimPos_diff_misc<SimPosDI>
{
public:
	using T = detail::SimPosDT;
	T x, y;
	constexpr SimPosDI() : x(0), y(0) {}
	constexpr SimPosDI(T x_, T y_) : x(x_), y(y_) {}
	SimPosDI(const SimPosDF d) : x(std::lround(d.x)), y(std::lround(d.y)) {}
	constexpr SimPosDI(const SimPosDCell d);
	static constexpr SimPosDI zero() { return SimPosDI(0,0); }

	constexpr int length_sq() const { return x*x+y*y; }
	int length_1() const { return std::abs(x) + std::abs(y); }
	int maxComponent() const { return std::max(std::abs(x), std::abs(y)); }
};


class SimPosDCell :
	public detail::SimPos_common_cmp<SimPosDCell>,
	public detail::SimPos_diff_addsub<SimPosDCell>,
	public detail::SimPos_diff_mul<SimPosDCell, int, SimPosDCell>,
	public detail::SimPos_diff_mul<SimPosDCell, float, SimPosDF>,
	public detail::SimPos_diff_div<SimPosDCell, float, SimPosDF>,
	public detail::SimPos_diff_misc<SimPosDCell>
{
public:
	using T = detail::SimPosDT;
	T x, y;
	constexpr SimPosDCell() : x(0), y(0) {}
	constexpr SimPosDCell(T x_, T y_) : x(x_), y(y_) {}
	SimPosDCell(const SimPosDF d) : x(std::lround(d.x/CELL)), y(std::lround(d.y/CELL)) {}
	static constexpr SimPosDCell zero() { return SimPosDCell(0,0); }

	constexpr int length_sq() const { return (x*x+y*y)*CELL*CELL; }
	int length_1() const { return (std::abs(x) + std::abs(y))*CELL; }
	int maxComponent() const { return std::max(std::abs(x), std::abs(y))*CELL; }
};


constexpr SimPosDI::SimPosDI(SimPosDCell d) : x(d.x*CELL), y(d.y*CELL) {}
constexpr SimPosDF::SimPosDF(SimPosDI d) : x(d.x), y(d.y) {}
constexpr SimPosDF::SimPosDF(SimPosDCell d) : SimPosDF(SimPosDI(d)) {}



class SimPosFT
{
public:
	float x, y;
	SimPosFT() {}
	// hopefully force truncation of floats in x87 registers by storing and reloading from memory, so that rounding issues don't cause particles to appear in the wrong pmap list. If using -mfpmath=sse or an ARM CPU, this may be unnecessary.
	SimPosFT(float x_, float y_) {
		volatile float tmpx = x_, tmpy = y_;
		x = tmpx, y = tmpy;
	}
	SimPosFT(const SimPosF p);
};

class SimPosF :
	public detail::SimPos_common_cmp<SimPosF>,
	public detail::SimPos_pos_cmp<SimPosF, SimPosDF>,
	public detail::SimPos_pos_addsub_promote<SimPosF, SimPosDI, SimPosF, SimPosDF>,
	public detail::SimPos_pos_addsub_promote<SimPosF, SimPosDCell, SimPosF, SimPosDF>,
	public detail::SimPos_pos_addsub<SimPosF, SimPosDF>
{
public:
	using T = float;
	T x, y;
	SimPosF () {}
	constexpr SimPosF(float x_, float y_) : x(x_), y(y_) {}
	SimPosF(const SimPosFT p) : x(p.x), y(p.y) {}
	constexpr SimPosF(const SimPosI p);

	SimPosCell cell() const;
	SimPosCell cell_safe() const;
};

class SimPosI :
	public detail::SimPos_common_cmp<SimPosI>,
	public detail::SimPos_pos_cmp<SimPosI, SimPosDI>,
	public detail::SimPos_pos_addsub_promote<SimPosI, SimPosDCell, SimPosI, SimPosDI>,
	public detail::SimPos_pos_addsub<SimPosI, SimPosDI>,
	public detail::SimPos_pos_addsub_promote<SimPosI, SimPosDF, SimPosF, SimPosDF>
{
public:
	using T = detail::SimPosT;
	T x, y;
	SimPosI() {}
	constexpr SimPosI(T x_, T y_) : x(x_), y(y_) {}
	constexpr SimPosI(const SimPosF p) : x(std::lround(p.x)), y(std::lround(p.y)) {}
	SimPosI(const SimPosFT p) : SimPosI(SimPosF(p)) {}

	SimPosCell cell() const;
	SimPosCell cell_safe() const;
};

class SimPosCell :
	public detail::SimPos_common_cmp<SimPosCell>,
	public detail::SimPos_pos_cmp<SimPosCell, SimPosDCell>,
	public detail::SimPos_pos_addsub<SimPosCell, SimPosDCell>,
	public detail::SimPos_pos_addsub_promote<SimPosCell, SimPosDI, SimPosI, SimPosDI>,
	public detail::SimPos_pos_addsub_promote<SimPosCell, SimPosDF, SimPosF, SimPosDF>
{
public:
	using T = detail::SimPosT;
	T x, y;
	SimPosCell() {}
	constexpr SimPosCell(T x_, T y_) : x(x_), y(y_) {}
	SimPosCell(const SimPosFT c) : SimPosCell(SimPosI(c)) {}

	// NB: default conversions to SimPosCell do not work correctly for negative positions
	constexpr SimPosCell(const SimPosF p) : SimPosCell(SimPosI(p)) {}
	constexpr SimPosCell(const SimPosI p) : x(p.x/CELL), y(p.y/CELL) {}

	constexpr SimPosI topLeft() const { return SimPosI(x*CELL, y*CELL); }
	constexpr SimPosI centre() const { return SimPosI(x*CELL+CELL/2, y*CELL+CELL/2); }
	constexpr SimPosI bottomRight() const { return SimPosI(x*CELL+CELL-1, y*CELL+CELL-1); }
};

TPT_INLINE SimPosFT::SimPosFT(const SimPosF p) : SimPosFT(p.x, p.y) {}
constexpr SimPosF::SimPosF(const SimPosI p) : x(p.x), y(p.y) {}

TPT_INLINE SimPosCell SimPosI::cell() const { return SimPosCell(*this); }
TPT_INLINE SimPosCell SimPosI::cell_safe() const { return SimPosCell(std::floor(float(x)/CELL), std::floor(float(y)/CELL)); }
TPT_INLINE SimPosCell SimPosF::cell() const { return SimPosCell(*this); }
TPT_INLINE SimPosCell SimPosF::cell_safe() const { return SimPosCell(std::floor(x/CELL), std::floor(y/CELL)); }



class SimAreaF :
	public detail::SimArea_common<SimAreaF, SimPosF, SimPosDF>
{
public:
	using T = SimPosF;
	using DT = SimPosDF;
	T topLeft;
	DT size;

	SimAreaF() : topLeft(), size() {}
	SimAreaF(const T topLeft_, const DT size_) : topLeft(topLeft_), size(size_) {}
	// Note that topLeft and bottomRight are coords that are *inside* the area. "topLeft+size" gives a coord just outside the area.
	SimAreaF(const T topLeft_, const T bottomRight_) { setCorners(topLeft_, bottomRight_); }

	SimAreaF(const SimAreaI a);
	SimAreaF(const SimAreaCell a);
};


class SimAreaI :
	public detail::SimArea_common<SimAreaI, SimPosI, SimPosDI>
{
public:
	using T = SimPosI;
	using DT = SimPosDI;
	T topLeft;
	DT size;

	SimAreaI() : topLeft(), size() {}
	SimAreaI(const T topLeft_, const DT size_) : topLeft(topLeft_), size(size_) {}
	// Note that topLeft and bottomRight are coords that are *inside* the area. "topLeft+size" gives a coord just outside the area.
	SimAreaI(const T topLeft_, const T bottomRight_) { setCorners(topLeft_, bottomRight_); }

	SimAreaI(const SimAreaF a) : SimAreaI(a.topLeft, a.bottomRight()) {}
	SimAreaI(const SimAreaCell a);
};

class SimAreaCell :
	public detail::SimArea_common<SimAreaCell, SimPosCell, SimPosDCell>
{
public:
	using T = SimPosCell;
	using DT = SimPosDCell;
	T topLeft;
	DT size;

	SimAreaCell() : topLeft(), size() {}
	SimAreaCell(const T topLeft_, const DT size_) : topLeft(topLeft_), size(size_) {}
	// Note that topLeft and bottomRight are coords that are *inside* the area. "topLeft+size" gives a coord just outside the area.
	SimAreaCell(const T topLeft_, const T bottomRight_) { setCorners(topLeft_, bottomRight_); }

	SimAreaCell(const SimAreaF a) : SimAreaCell(a.topLeft, a.bottomRight()) {}
	SimAreaCell(const SimAreaI a) : SimAreaCell(a.topLeft, a.bottomRight()) {}
};

TPT_INLINE SimAreaF::SimAreaF(SimAreaI a) : SimAreaF(a.topLeft, a.size) {}
TPT_INLINE SimAreaF::SimAreaF(SimAreaCell a) : SimAreaF(a.topLeft.topLeft(), a.bottomRight().bottomRight()) {}
TPT_INLINE SimAreaI::SimAreaI(SimAreaCell a) : SimAreaI(a.topLeft.topLeft(), a.bottomRight().bottomRight()) {}


namespace SimPos
{

constexpr SimPosF midpoint(SimPosF p1, SimPosF p2) // Return point halfway between the arguments
{
	return SimPosF((p1.x+p2.x)/2, (p1.y+p2.y)/2);
}
constexpr SimPosF midpoint(SimPosI p1, SimPosI p2) // Return point halfway between the arguments
{
	return SimPosF(float(p1.x+p2.x)/2, float(p1.y+p2.y)/2);
}

constexpr SimPosI midpoint_tl(SimPosI p1, SimPosI p2) // Return point approximately halfway between the arguments, using the SPRK/INSL round-to-top-left behaviour
{
	return SimPosF((p1.x+p2.x)/2, (p1.y+p2.y)/2);
}

}

#endif
