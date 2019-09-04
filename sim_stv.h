#ifndef _SIM_STV_H
#define _SIM_STV_H

#include "model.h"

bool SimSTV(const Ballots &ballots, const Doubles &votecounts,
    Candidates &cands, const Config &config, Ints &order_c, Ints &order_a, 
    bool log, std::ostream &logs);

double WEUB(const Ballots &ballots, const Doubles &votecounts,
    Candidates &cand, const Config &config, const std::set<int> &elected,
    const Ints &order_c, const Ints &order_a, bool log, std::ostream &logs,
    Manipulation &chgs, int nodelafter, const char *ac_format, int ac_seats,
    const char *tprefix);

int RunManipulation(const Ballots &ballots, const Candidates &cands, 
    const Config &config, const std::set<int> &elected_o, 
    const Manipulation &mp, const char *logfile, const Ints &orgic,
    const Ints &origa, Manipulation &fm, bool applyhalf, double applyfrac, 
    const char *ac_format, int ac_seats, const char *tprefix);


bool TestManipulation(const Candidates &cands, const Manipulation &cmanip,
    const char *ac_format, int ac_seats, const char *tprefix);

#endif
