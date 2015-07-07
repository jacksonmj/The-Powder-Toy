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

#ifndef simulation_CellsData_fastloop_h
#define simulation_CellsData_fastloop_h

#include "simulation/Config.hpp"
#include "common/tpt-stdint.h"

#define FOR_2D_UINT8_NONZERO(DATA, X, X_BEGIN, X_END, Y, Y_BEGIN, Y_END, LOOP_BODY) \
{\
	/* Run LOOP_BODY for coordinates where char DATA[Y][X] is non-zero */\
	\
	const int stepSize = sizeof(uint_fast32_t)/sizeof(unsigned char);\
	for (int Y=(Y_BEGIN); Y<(Y_END); Y++)\
	{\
		int X = (X_BEGIN);\
		/* Check unaligned bytes at beginning of row */\
		for (; ( uintptr_t(&((DATA)[Y][X])) % stepSize)!=0 && X<(X_END); X++)\
		{\
			if ((DATA)[Y][X])\
			{\
				LOOP_BODY\
			}\
		}\
		int startX = X;\
		int bigStepCount = ((X_END)-startX)/stepSize;\
		for (int bigStep=0; bigStep<bigStepCount; bigStep++)\
		{\
			/* Simultaneously check multiple bytes != 0 (likely to be 4 bytes on 32 bit, 8 bytes on 64 bit) */\
			if (!(reinterpret_cast<const uint_fast32_t*>(&(DATA)[Y][startX])[bigStep]))\
				continue;\
			/* At least one byte was non-zero, so go through and check each one */\
			for (unsigned int i=0; i<stepSize; i++)\
			{\
				if ((DATA)[y][startX+bigStep*stepSize+i])\
				{\
					int X = startX+bigStep*stepSize+i;\
					LOOP_BODY\
				}\
			}\
		}\
		X += stepSize*bigStepCount;\
		/* Check unaligned bytes at end of row */\
		for (; X<X_END; X++)\
		{\
			if ((DATA)[Y][X])\
			{\
				LOOP_BODY\
			}\
		}\
	}\
}

#define FOR_CELLS_UINT8_NONZERO(DATA, X, Y, LOOP_BODY) FOR_2D_UINT8_NONZERO((DATA), X, 0, XRES/CELL, Y, 0, YRES/CELL, LOOP_BODY)

#define FOR_2D_UINT8_MASKED(DATA, MASK, X, X_BEGIN, X_END, Y, Y_BEGIN, Y_END, LOOP_BODY) \
{\
	/* Run LOOP_BODY for coordinates where (char DATA[Y][X])&MASK is non-zero */\
	\
	const int stepSize = sizeof(uint_fast32_t)/sizeof(unsigned char);\
	uint_fast32_t bigMask = 0;\
	for (int i=0; i<stepSize; i++)\
		bigMask |= uint_fast32_t((unsigned char)(MASK)) << (8*sizeof(unsigned char)*i);\
	for (int Y=(Y_BEGIN); Y<(Y_END); Y++)\
	{\
		int X = (X_BEGIN);\
		/* Check unaligned bytes at beginning of row */\
		for (; ( uintptr_t(&((DATA)[Y][X])) % stepSize)!=0 && X<(X_END); X++)\
		{\
			if ((DATA)[Y][X] & (MASK))\
			{\
				LOOP_BODY\
			}\
		}\
		int startX = X;\
		int bigStepCount = ((X_END)-startX)/stepSize;\
		for (int bigStep=0; bigStep<bigStepCount; bigStep++)\
		{\
			/* Simultaneously check multiple bytes (likely to be 4 bytes on 32 bit, 8 bytes on 64 bit) */\
			if (!(reinterpret_cast<const uint_fast32_t*>(&(DATA)[Y][startX])[bigStep] & bigMask))\
				continue;\
			/* At least one byte was non-zero after masking, so go through and check each one */\
			for (unsigned int i=0; i<stepSize; i++)\
			{\
				if ((DATA)[Y][startX+bigStep*stepSize+i] & (MASK))\
				{\
					int X = startX+bigStep*stepSize+i;\
					LOOP_BODY\
				}\
			}\
		}\
		X += stepSize*bigStepCount;\
		/* Check unaligned bytes at end of row */\
		for (; X<X_END; X++)\
		{\
			if ((DATA)[Y][X] & (MASK))\
			{\
				LOOP_BODY\
			}\
		}\
	}\
}


#define FOR_CELLS_UINT8_MASKED(DATA, MASK, X, Y, LOOP_BODY) FOR_2D_UINT8_MASKED((DATA), (MASK), X, 0, XRES/CELL, Y, 0, YRES/CELL, LOOP_BODY)

#endif
