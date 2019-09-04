#ifndef _STV_DISTANCE_H
#define _STV_DISTANCE_H

#include "model.h"

void Preliminaries(const Ballots &ballots, 
	const Candidates &cand, const Config &config, Node &node,
	bool compbounds, const std::set<int> &elected_o, std::ostream &log);

double distance(const Ballots &ballots, const Candidates &cand, 
	const Config &config, Node &node, double upperbound,
	double tleft, bool lprelax, const std::set<int> &elected_o, 
	std::ostream &log, const Ints &order_c, const Ints &order_a,
	double &def_ub, Result &result);

#endif
