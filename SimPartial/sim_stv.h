/*
    Copyright (C) 2019  Michelle Blom

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#ifndef _SIM_STV_H
#define _SIM_STV_H

#include "model.h"

bool SimSTV(const Ballots &ballots, const Doubles &votecounts,
	Candidates &cands, const Config &config, Ints &order_c, Ints &order_a, 
	bool log, std::ostream &logs, int sim_until);

bool SimSTVUntil(const Ballots &ballots, Candidates &cands, 
    const Config &config, int sim_until, const char *newballot_file);


#endif
