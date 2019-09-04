#ifndef _SIM_STV_H
#define _SIM_STV_H

#include "model.h"

bool SimSTV(const Ballots &ballots, const Doubles &votecounts,
	Candidates &cands, const Config &config, Ints &order_c, Ints &order_a, 
	bool log, std::ostream &logs, int sim_until);

bool SimSTVUntil(const Ballots &ballots, Candidates &cands, 
    const Config &config, int sim_until, const char *newballot_file);


#endif
