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

#ifndef ElemDataShared_h
#define ElemDataShared_h

class SimulationSharedData;

class ElemDataShared
{
protected:
	SimulationSharedData *simSD;
	int elementId;
public:
	ElemDataShared(SimulationSharedData *sd, int t_) : simSD(sd), elementId(t_) {}
	virtual ~ElemDataShared() {}
};

#endif
