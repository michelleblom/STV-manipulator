#ifndef _TREE_STV_H
#define _TREE_STV_H

#include "model.h"

Result RunTreeSTV(const Ballots &ballots, const Candidates &cands,
	const Config &config, const Ints &order_c, const Ints &order_a, 
	double upperbound,double timelimit,bool compbounds,const char *logf);

#endif
