#include "sim_stv.h"
#include<string>
#include<iostream>
#include<fstream>
#include<sstream>
#include<iomanip>
#include<cmath>

using namespace std;

// Function prototypes

void PrintList(const Ints &list, ostream &log){
    for(int i = 0; i < list.size(); ++i){
        log << list[i] << " ";
    }
}

void EliminateCandidate(int toe, Candidates &cand, 
    const Ballots &ballots, const Doubles &votecounts,
    const Config &config,bool log,ostream &logs);

void InsertCandidateSurplus(const Candidate &c, double surplus, 
    L_IDS &surpluses);

int NextCandidate(const Ballot &b, int id, const Candidates &cand);

void DistributeSurplus(Candidate &c, Candidates &cand, double surplus,
    const Ballots &ballots, const Doubles &votecounts, 
    const Config &config,bool log,ostream &logs);

bool SimSTVUntil(const Ballots &ballots, Candidates &cands, 
    const Config &config, int sim_until, const char *newballot_file){
    try{
        Ints order_c;
        Ints order_a;
    
        Doubles votecounts(ballots.size(), 0);
        for(int i = 0; i < ballots.size(); ++i){
            votecounts[i] = ballots[i].votes;
        }

        if(SimSTV(ballots, votecounts, cands, config, order_c,
            order_a, false, cout, sim_until)){
            // Record current state of candidate tallies in new ballot file
            Ints crem;
            Doubles newvotecounts(ballots.size(), 0);
            int nelected = 0;
            for(int i = 0; i < cands.size(); ++i){
                const Candidate &cd = cands[i];
                if(cd.seat != -1)
                    nelected += 1;

                if(cd.standing){
                    crem.push_back(cd.id);
                    for(int j = 0; j < cd.bweights.size(); ++j){
                        const IntDouble &bweight = cd.bweights[j];
                        newvotecounts[bweight.id] = votecounts[bweight.id]*
                            bweight.weight;
                    }
                }
            }
            ofstream ofile(newballot_file);
            for(int i = 0; i < crem.size()-1; ++i){
                ofile << crem[i] << ",";
            }
            ofile << crem.back() << endl;

            ofile << "Seats," << config.nseats - nelected << endl;
            ofile << "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-" << endl;
            for(int i = 0; i < ballots.size(); ++i){
                if(newvotecounts[i] == 0)
                    continue;

                const Ballot &b = ballots[i];
                Ints redb;
                for(int j = 0; j < b.prefs.size(); ++j){
                    int cid = cands[b.prefs[j]].id;
                    if(find(crem.begin(), crem.end(), cid) !=
                        crem.end()){
                        redb.push_back(cid);
                    }    
                } 
                if(!redb.empty()){
                    ofile << "(";
                    for(int j = 0; j < redb.size(); ++j){
                        ofile << redb[j] << ",";
                    }
                    ofile << redb.back() << ") : " << newvotecounts[i] << endl;
                }
            }

            ofile.close();
        }
    }
    catch(STVException &e)
    {
        throw e;
    }
    catch(...)
    {
        throw STVException("Unexpected error in STV simulation.");
    }

    return true;

}


// Function for simulating STV election
bool SimSTV(const Ballots &ballots, const Doubles &votecounts, 
    Candidates &cand, const Config &config, Ints &order_c, 
    Ints &order_a, bool log, ostream &logs, int sim_until)
{
    try {
        double totvotes = 0;
        if(log) logs << "First preference tallies: " << endl;
        for(Candidates::iterator it = cand.begin(); it != cand.end(); ++it){
            it->sim_votes = 0;
            it->max_votes = 0;
            it->bweights.clear();
            it->standing = 1;
            it->seat = -1;
            it->surplus = 0;

            for(Ints::const_iterator li = it->ballots.begin();
                li != it->ballots.end(); ++li){
                it->bweights.push_back(IntDouble(*li, 1));
                it->sim_votes += votecounts[*li];
            }

            it->max_votes = it->sim_votes;
            totvotes += it->sim_votes;

            if(log) logs<<"    Candidate "<<it->id<<" "<<it->sim_votes<< endl;
        }

        // Step 1: Determine quota
        double quota = (int)(1.0 + (totvotes/(double)(config.nseats+1.0))); 
        if(config.quota_override != -1){
            quota = config.quota_override;
        }

        if(log) logs << "The quota for election is " << quota << endl;
        L_IDS surpluses;        

        int currseat = 0;
        int round = 0;
        while(currseat < config.nseats){
            if(sim_until != -1 && round == sim_until){
                if(log) logs << "" << endl;
                return true;
            }

            int standing = 0;

            // if a candidate has a quota, add them to the list of 
            // candidates with a surplus
            for(int i = 0; i < cand.size(); ++i){
                Candidate &c = cand[i];

                if(c.standing) 
                    ++standing;

                if(c.surplus || !c.standing)
                    continue;

                if(c.standing && c.sim_votes >= quota){
                    InsertCandidateSurplus(c, max(0.0,c.sim_votes-quota), 
                        surpluses);
                    c.surplus = 1;
                }
            }
        
            if(standing == config.nseats - currseat){
                if(log) logs << "Number of candidates left standing equals "<<
                    " the number of remaining seats." << endl;

                // Elect all remaining candidates
                Ints standing;
                for(int i = 0; i < cand.size(); ++i){
                    Candidate &c = cand[i];
                    if(c.standing){
                        Ints::iterator it = standing.begin();
                        bool inserted = false;
                        for(;  it != standing.end(); ++it){
                            if(c.sim_votes > cand[*it].sim_votes){
                                standing.insert(it, i);
                                inserted = true;
                                break;
                            }
                        }
                        if(!inserted){
                            standing.push_back(i);
                        }
                    }
                }


                for(int i = 0; i < standing.size(); ++i){
                    Candidate &c = cand[standing[i]];
                    if(log) logs<<"Candidate "<<c.id<<" elected (votes "
                        << c.sim_votes << ") " << endl;
                    c.seat = currseat++;
                    c.standing = 0;
                    order_c.push_back(c.index);
                    order_a.push_back(1);   
                }
                break;
            }
            
            if(surpluses.empty()){
                // eliminate candidate with fewest votes.
                // distribute votes at their value.
                double leastvotes = numeric_limits<double>::max();
                int toeliminate = -1;
                for(int i = 0; i < cand.size(); ++i){
                    if(log && cand[i].standing){
                        logs << "Candidate " << cand[i].id << " has "
                            << cand[i].sim_votes << " votes " << endl;
                    }

                    if(cand[i].standing && cand[i].sim_votes < leastvotes){
                        leastvotes = cand[i].sim_votes;
                        toeliminate = i;
                    }
                }

                order_c.push_back(toeliminate);
                order_a.push_back(0);

                if(log){
                    logs << "Candidate " << cand[toeliminate].id
                        << " eliminated on " << cand[toeliminate].sim_votes
                        << " votes" << endl; 
                    //cout << "Possible upper bound " << quota - 
                    //  cand[toeliminate].sim_votes << endl;
                }

                EliminateCandidate(toeliminate, cand, ballots, 
                    votecounts, config,log, logs);
            }
            else{
                for(int i = 0; i < cand.size(); ++i){
                    if(log && cand[i].standing){
                        logs << "Candidate " << cand[i].id << " has "
                            << cand[i].sim_votes << " votes " << endl;
                    }
                }

                // start with candidate with largest surplus
                Candidate &elect = cand[surpluses.front().id];
                double surplus = surpluses.front().weight;

                elect.seat = currseat++;
                elect.standing = 0;

                order_c.push_back(elect.index);
                order_a.push_back(1);

                if(log) logs<<"Candidate "<<elect.id<<" elected (votes "
                    << elect.sim_votes << ") " << endl;

                // distribute surplus
                if(currseat < config.nseats){
                    DistributeSurplus(elect,cand,surplus,ballots,
                        votecounts, config,log, logs);
                }

                surpluses.pop_front();
            }

            if(currseat == config.nseats){
                // All seats filled.
                if(order_c.size() != cand.size()){
                    for(int i = 0; i < cand.size(); ++i){
                        const Candidate &c = cand[i];
                        if(c.standing) {
                            order_c.push_back(c.index);
                            order_a.push_back(0);
                        }
                    }
                }
                break;
            }

            ++round;
        }
    }
    catch(STVException &e)
    {
        throw e;
    }
    catch(...)
    {
        throw STVException("Unexpected error in STV simulation.");
    }

    if(log) logs << "Simulation complete" << endl;
    return true;
}


void InsertCandidateSurplus(const Candidate &c,double surplus,L_IDS &surpluses)
{
    try{
        L_IDS::iterator it = surpluses.begin();

        for( ; it != surpluses.end(); ++it){
            if(it->weight > surplus)
                continue;

            break;
        }

        surpluses.insert(it, IntDouble(c.index, surplus));
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(...){
        throw STVException("Unexpected error in InsertCandidateSurplus.");
    }
}


int NextCandidate(const Ballot &b, int index, const Candidates &cand)
{
    try{
        int idx = find(b.prefs.begin(), b.prefs.end(), index)-b.prefs.begin();
    
        for(int i = idx+1; i < b.prefs.size(); ++i){
            if(cand[b.prefs[i]].standing == 0 || cand[b.prefs[i]].surplus == 1) 
                continue;

            return b.prefs[i];
        }

        return -1;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(...){
        throw STVException("Unexpected error in NextCandidate.");
    }

    return -1;
}

void DistributeSurplus(Candidate &c, Candidates &cand, double surplus,
    const Ballots &ballots, const Doubles &votecounts, 
    const Config &config, bool log, ostream &logs)
{
    c.standing = 0;
    if(surplus < 0.001) return;

    if(log) logs<<"Surplus of "<<surplus<<" to be distributed." << endl;

    try
    {
        // compute number of transferrable papers
        IDS2d totransfer(cand.size());

        IDS &tlist = c.bweights;

        double tpapers = 0;
        for(IDS::iterator it = tlist.begin(); it != tlist.end(); ++it)
        {
            int next = NextCandidate(ballots[it->id], c.index, cand);
            if(next >= 0)
            {
                tpapers += votecounts[it->id]*it->weight;
                totransfer[next].push_back(*it);
            }
        }

        double tvalue = min(1.0, surplus / tpapers);

        if(log) logs << "Transfer value: " << tvalue << endl;
        // the ballots in 'totransfer' will be distributed, each with a value
        // of 'tvalue'
        for(int i = 0; i < totransfer.size(); ++i)
        {
            const IDS &list = totransfer[i];
            if(list.empty()) 
                continue;

            Candidate &ct = cand[i];
            double total = 0;

            for(int j = 0; j < list.size(); ++j)
            {
                IntDouble td = list[j];
                td.weight *= tvalue;

                ct.bweights.push_back(td);

                total += votecounts[td.id] * td.weight;
            }

            if(log) logs << "    Transferring " << total <<
                " votes from " << c.id << " to " << ct.id << endl; 
            
            ct.sim_votes += total;
            ct.max_votes += total;
        }

        c.sim_votes -= surplus;
    }
    catch(STVException &e)
    {
        throw e;
    }
    catch(exception &e)
    {
        throw STVException(string(e.what()));
    }
    catch(...)
    {
        throw STVException("Unexpected error in DistributeSurplus.");
    }

} 


void EliminateCandidate(int toe, Candidates &cand, 
    const Ballots &ballots, const Doubles &votecounts,
    const Config &config, bool log, ostream &logs)
{
    try{
        Candidate &e = cand[toe];
        e.standing = false;
        IDS2d totransfer(cand.size());

        // Distribute all ballots (at their current value) to rem candidates
        for(IDS::iterator it=e.bweights.begin();it!=e.bweights.end();++it){
            int next = NextCandidate(ballots[it->id], e.index, cand);
            if(next >= 0)
            {
                //cand[next].bweights.push_back(*it);
                totransfer[next].push_back(*it);
            }

            it->weight = 0;
        }

        e.sim_votes = 0;
        for(int i = 0; i < totransfer.size(); ++i){
            const IDS &list = totransfer[i];
            if(list.empty()) continue;
            Candidate &ct = cand[i];

            double total = 0;
            for(int j = 0; j < list.size(); ++j){
                total += votecounts[list[j].id] * list[j].weight;
                ct.bweights.push_back(list[j]);
            }

            ct.sim_votes += total;
            ct.max_votes += total;
            if(log) logs << "    " << total << " votes distributed from " 
                << e.id << " to " << ct.id << endl;
        }
    }
    catch(STVException &e){
        throw e;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(...){
        throw STVException("Unexpected error in EliminateCandidate.");
    }
}


