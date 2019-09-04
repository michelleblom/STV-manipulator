#include "sim_stv.h"
#include<string>
#include<iostream>
#include<fstream>
#include<sstream>
#include<iomanip>
#include<cmath>
#include<memory>
#include<cstdio>
#include<array>
#include<boost/algorithm/string/predicate.hpp>

using namespace std;


std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


struct ChangeTest{
    int c1;
    int c2;
    int amount;

    bool operator<(const ChangeTest &other) {
        return amount < other.amount;
    }
};

void PrintList(const Ints &list){
    for(int i = 0; i < list.size(); ++i){
        cout << list[i] << " ";
    }
}
void PrintList(const Ints &list, ostream &log){
    for(int i = 0; i < list.size(); ++i){
        log << list[i] << " ";
    }
}

int SumPairVal(const vector<Change> &pairs){
    int sum = 0;
    for(int k = 0; k < pairs.size(); ++k)
        sum += get<1>(pairs[k]);
    return sum;
}

int PositionOf(int cidx, const Ints &list){
    for(int i = 0; i < list.size(); ++i){
        if(list[i] == cidx)
            return i;
    }
    return INT_MAX;
}

pair<int,int> PositionIn(int cidx, const Ints &list, const Ints &result){
    int seated = 0;
    int position = 0;
    for(int i = 0; i < list.size(); ++i){
        if(list[i] == cidx){
            return make_pair(seated, position);
        }
        if(result[list[i]])
            seated++;

        position++;
    }
    return make_pair(result.size(), result.size());
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

bool TestRemove(int amount, const Candidate &cd, const Ballots &ballots, 
    const Candidates &cands, const Config &config, 
    const std::set<int> &orig_elected, Manipulation &cmanip, int nodeleteafter,
    const char *ac_format, int ac_seats, const char *tprefix);

bool TestAdd(int amount, const Candidate &cd, const Ballots &ballots, 
    const Candidates &cands, const Config &config, 
    const std::set<int> &orig_elected, Manipulation &cmanip,
   const char *ac_format, int ac_seats, const char *tprefix);


bool TestShift(int amount, const Candidate &c1, const Candidate &c2, 
    const Ballots &ballots, const Candidates &cands, const Config &config, 
    const std::set<int> &orig_elected, Manipulation &cmanip, int nodeleteafter,
    const char *ac_format, int ac_seats, const char *tprefix);
    
void FindBestManipulation(const vector<ChangeTest> &totest, 
    Manipulation &changes, int &weub, const Candidates &cand, 
    const Ballots &ballots, const Config &config, 
    const std::set<int> &elected, int nodeleteafter,
    const char *ac_format, int ac_seats, const char *tprefix);

void AddTo(const Candidate &c1, int amount, Manipulation &cmanip){
    Ints repl;
    repl.push_back(c1.index);
    cmanip.toadd.push_back(make_tuple(repl, amount, -1, -1));
}

double WEUB_DelOnly(const Ballots &ballots, const Doubles &votecounts,
    const Candidates &origcand, const Config &config, const std::set<int> &elected,
    const Ints &order_c, const Ints &order_a, bool log, ostream &logs,
    Manipulation &changes, int nodeleteafter, const char *ac_format, int ac_seats, 
    const char *tprefix)
{
    try{
        Candidates cand(origcand);
        for(set<int>::const_iterator it = elected.begin();
            it != elected.end(); ++it){
            Candidate &c = cand[*it];
            sort(c.bweights.begin(), c.bweights.end());
        }

        Manipulation base(changes);

        int weub = -1;
        for(set<int>::const_iterator it = elected.begin();
            it != elected.end(); ++it){
            const Candidate &cit = cand[*it];
            int tryvalue = ceil(cit.max_votes);
            if(weub != -1)
                tryvalue = min(weub, tryvalue);

            Manipulation cmanip(base);
            bool verified = TestRemove(tryvalue, cit, ballots,
                cand, config, elected, cmanip, nodeleteafter,
                ac_format, ac_seats, tprefix);
            if(verified){
                changes = cmanip;
                weub = tryvalue;
                int DELTA = ceil(tryvalue/2.0);

                vector<pair<int,bool> > tested;
                tested.push_back(make_pair(tryvalue, true));

                while(true){
                    Manipulation cmanip1(base);
                    tryvalue = tryvalue-DELTA;
                    if(tryvalue > 0){
                        bool found = false;
                        bool found_v = false;
                        for(int i = 0; i < tested.size(); ++i){
                            if(get<0>(tested[i]) == tryvalue){
                                found = true;
                                found_v = get<1>(tested[i]);
                                break;
                            }
                        }
                        if(found){
                            verified = found_v;
                        }
                        else{ 
                            verified = TestRemove(tryvalue, cit, ballots,
                                cand, config, elected, cmanip1, nodeleteafter,
                                ac_format, ac_seats, tprefix);
                            tested.push_back(make_pair(tryvalue, verified));
                        }
                    }
                    else{
                        verified = false;
                    }

                    if(verified){
                        changes = cmanip1;
                        weub = tryvalue;
                    }
                    else{
                        tryvalue += DELTA;
                        if(DELTA == 1)
                            break;
                        DELTA = ceil(DELTA/2.0);
                    }
                } 
            }
        }

        return weub;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(STVException &e)
    {
        throw e;
    }
    catch(...)
    {
        throw STVException("Unexpected error in WEUB_DelOnly.");
    } 

}


double WEUB_AddOnly(const Ballots &ballots, const Doubles &votecounts,
    const Candidates &cand, const Config &config, const std::set<int> &elected,
    const Ints &order_c, const Ints &order_a, bool log, ostream &logs,
    Manipulation &changes, int nodeleteafter, const char *ac_format, int ac_seats, 
    const char *tprefix)
{
    try{
        int maxvotes = 0;
        Ints losers;
        for(int i = 0; i < cand.size(); ++i){
            const Candidate &c = cand[i];
            maxvotes = max(maxvotes, (int)ceil(c.max_votes));
            if(c.seat == -1)
                losers.push_back(i);
        } 

        Manipulation base(changes);
        int weub = -1;
        for(int i = 0; i < losers.size(); ++i){
            // Can we make loser[i] win by giving them up to 
            // 'maxvotes' additional first preference votes?
            const Candidate &c = cand[losers[i]];
            int tryvalue = maxvotes - c.fp_votes;

            if(weub != -1)
                tryvalue = min(weub, tryvalue);

            Manipulation cmanip(base);
            bool verified = TestAdd(tryvalue, c, ballots,
                cand, config, elected, cmanip, ac_format,
                ac_seats, tprefix);

            if(verified){
                vector<pair<int,bool> > tested;
                tested.push_back(make_pair(tryvalue, true));

                changes = cmanip;
                weub = tryvalue;
                int DELTA = ceil(tryvalue/2.0);

                while(true){
                    Manipulation cmanip1(base);
                    tryvalue = tryvalue-DELTA;
                    if(tryvalue > 0){
                        bool found = false;
                        bool found_v = false;
                        for(int i = 0; i < tested.size(); ++i){
                            if(get<0>(tested[i]) == tryvalue){
                                found = true;
                                found_v = get<1>(tested[i]);
                                break;
                            }
                        }
                        if(found){
                            verified = found_v;
                        }
                        else{ 
                            verified = TestAdd(tryvalue, c, ballots,
                                cand, config, elected, cmanip1, ac_format,
                                ac_seats, tprefix);
                            tested.push_back(make_pair(tryvalue, verified));
                        }               
                    }
                    else{
                        verified = false;
                    }

                    if(verified){
                        changes = cmanip1;
                        weub = tryvalue;
                    }
                    else{
                        tryvalue += DELTA;
                        if(DELTA == 1)
                            break;
                        DELTA = ceil(DELTA/2.0);
                    }
                } 
            }
        }
          
        return weub;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(STVException &e)
    {
        throw e;
    }
    catch(...)
    {
        throw STVException("Unexpected error in WEUB_AddOnly.");
    } 

}


double WEUB(const Ballots &ballots, const Doubles &votecounts,
    Candidates &cand, const Config &config, const std::set<int> &elected,
    const Ints &order_c, const Ints &order_a, bool log, ostream &logs,
    Manipulation &changes, int nodeleteafter, const char *ac_format,
    int ac_seats, const char *tprefix)
{
    try {
        if(config.addonly)
            return WEUB_AddOnly(ballots, votecounts, cand, config, elected,
                order_c, order_a, log, logs, changes, nodeleteafter,
                ac_format, ac_seats, tprefix);

        if(config.deleteonly)
            return WEUB_DelOnly(ballots, votecounts, cand, config, elected,
                order_c, order_a, log, logs, changes, nodeleteafter,
                ac_format, ac_seats, tprefix);

        double totvotes = 0;

        int weub = -1;
        Doubles2d votes_per_round;
        Doubles leastvotes_per_round(cand.size(), -1);

        for(Candidates::iterator it = cand.begin(); it != cand.end(); ++it){
            it->fp_votes = 0;
            it->sim_votes = 0;
            it->bweights.clear();
            it->standing = 1;
            it->seat = -1;
            it->surplus = 0;

            for(Ints::const_iterator li = it->ballots.begin();
                li != it->ballots.end(); ++li){
                it->bweights.push_back(IntDouble(*li, 1));
                it->sim_votes += votecounts[*li];
                it->fp_votes += votecounts[*li];
            }
            totvotes += it->sim_votes;

            Doubles votes(cand.size(), 0);
            votes[0] = it->sim_votes;
            votes_per_round.push_back(votes);

            if(leastvotes_per_round[0] == -1){
                leastvotes_per_round[0] = it->sim_votes;
            }
            else{
                leastvotes_per_round[0] = min(it->sim_votes,
                    leastvotes_per_round[0]);
            }
        }

        // Step 1: Determine quota
        double quota = (int)(1.0 + (totvotes/(double)(config.nseats+1.0))); 
    
        if(config.quota_override != -1){
            quota = config.quota_override;
        }

        L_IDS surpluses;        

        int currseat = 0;
        int counter = 0;

        vector<ChangeTest> totest;

        while(currseat < config.nseats){
            int standing = 0;

            // if a candidate has a quota, add them to the list of 
            // candidates with a surplus
            for(int i = 0; i < cand.size(); ++i){
                Candidate &c = cand[i];

                votes_per_round[i][counter] = c.sim_votes;
                if(c.standing) 
                    ++standing;
                else
                    continue;

                if(c.surplus)
                    continue;

                if(c.standing && (c.sim_votes >= quota)){
                    InsertCandidateSurplus(c, max(0.0,c.sim_votes-quota), 
                        surpluses);
                    c.surplus = 1;
                }
            }

            if(standing == config.nseats - currseat){
                for(int i = 0; i < cand.size(); ++i){
                    if(cand[i].standing)
                        cand[i].seat = currseat++;
                }

                FindBestManipulation(totest, changes, weub, cand, ballots,
                    config, elected, nodeleteafter, ac_format, ac_seats,
                    tprefix);

                return ceil(weub);
            }                   
            if(surpluses.empty()){
                // eliminate candidate with fewest votes.
                // distribute votes at their value.
                double leastvotes = numeric_limits<double>::max();
                int toeliminate = -1;
                for(int i = 0; i < cand.size(); ++i){
                    if(cand[i].standing && cand[i].sim_votes < leastvotes){
                        leastvotes = cand[i].sim_votes;
                        toeliminate = i;
                    }
                }

                leastvotes_per_round[counter] = leastvotes;

                if(config.FIX_UNTIL < counter){
                    for(set<int>::const_iterator it = elected.begin();
                        it != elected.end(); ++it){
                        if(cand[*it].standing){
                            // can we eliminate *it here? we will need to remove
                            // 'change' votes from *it & give them to someone else.
                            double change = ceil(cand[*it].sim_votes-leastvotes+1);
                            //cout << "change = " << change << endl;
                            ChangeTest t;
                            t.c1 = *it;
                            t.c2 = toeliminate;
                            t.amount = change;
                            totest.push_back(t);
                        }
                    }
                }
            
                EliminateCandidate(toeliminate, cand, ballots, 
                    votecounts, config, false, cout);
            }
            else{
                // start with candidate with largest surplus
                Candidate &elect = cand[surpluses.front().id];
                double surplus = surpluses.front().weight;
                double e_tally = elect.sim_votes;

                double leastvotes = numeric_limits<double>::max();
                double maxvotes = -1;
                for(int i = 0; i < cand.size(); ++i){
                    const Candidate &c = cand[i];
                    if(!c.standing)
                        continue;
                    if(c.sim_votes < leastvotes){
                        leastvotes = c.sim_votes;
                    }
                    if(c.sim_votes > maxvotes){
                        maxvotes = c.sim_votes;
                    }
                }
 
                if(config.FIX_UNTIL < counter){
                    // How many votes do we need to take away from the elected
                    // candidate and give to one that is still remaining (a
                    // candidate that is an original "loser") to put them
                    // above a quota.
                    // Create sorted version of ballots in terms of where elected
                    // candidate is positioned.
                    for(int i = 0; i < cand.size(); ++i){
                        if(i == elect.index)
                            continue;
                        double tally = cand[i].sim_votes;

                        if(elected.find(i) == elected.end() &&
                            cand[i].standing){
                            // Try give candidate 'i' a quota at this point
                            int shift = ceil(max(0.0, max(quota - tally, e_tally-tally+1)));
                            //cout << "shift " << shift << endl;
                            ChangeTest t;
                            t.c1 = elect.index;
                            t.c2 = i;
                            t.amount = shift;
                            totest.push_back(t);
                        }
                    }
                }

                elect.seat = currseat++;
                elect.standing = 0;

                // distribute surplus
                DistributeSurplus(elect,cand,surplus,ballots,
                    votecounts, config, false, cout);

                surpluses.pop_front();
            }

            if(currseat == config.nseats){
                FindBestManipulation(totest, changes, weub, cand, ballots,
                    config, elected, nodeleteafter, ac_format, ac_seats,
                    tprefix);
                return ceil(weub);
            }

            ++counter;
        }
        FindBestManipulation(totest, changes, weub, cand, ballots,
            config, elected, nodeleteafter, ac_format, ac_seats, tprefix);

        return ceil(weub);
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(STVException &e)
    {
        throw e;
    }
    catch(...)
    {
        throw STVException("Unexpected error in WEUB.");
    } 
}


// Function for simulating STV election
bool SimSTV(const Ballots &ballots, const Doubles &votecounts, 
    Candidates &cand, const Config &config, Ints &order_c, 
    Ints &order_a, bool log, ostream &logs)
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
        int quota = 1 + (int)(totvotes/(double)(config.nseats+1.0)); 

        if(config.original_votes != -1 && (config.addonly || config.deleteonly)){
            quota = 1 + (int)(config.original_votes/(double)(config.original_seats+1.0));
        }
        else if(config.quota_override != -1){
            quota = config.quota_override;
        }

        if(log) logs << "Total votes " << totvotes << " revised count " << config.original_votes << endl;
        if(log) logs << "The quota for election is " << quota << endl;
        L_IDS surpluses;        

        int currseat = 0;
        while(currseat < config.nseats){
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
        }
    }
    catch(exception &e){
        throw STVException(string(e.what()));
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


bool IsPrefix(const Ints &prefix, const Ints &list){
    if(prefix.size() > list.size())
        return false;

    for(int i = 0; i < prefix.size(); ++i)
        if(prefix[i] != list[i])
            return false;

    return true;
}


typedef vector<Change>::iterator VCIT;
typedef vector<VCIT> VCITS;


int RunManipulation(const Ballots &ballots, const Candidates &cands, 
    const Config &config, const std::set<int> &elected_o, 
    const Manipulation &mp, const char *logfile, const Ints &origc,
    const Ints &origa, Manipulation &fm, 
    bool applyhalf, double applyfrac, const char *ac_format, int ac_seats,
    const char *tprefix)
{
    try{
        ofstream ofile(logfile);
        Candidates candcopy(cands);

        Manipulation mcopy(mp);
        Ballots newballots(ballots);

        IDS bweights;
        for(int i = 0; i < newballots.size(); ++i)
            bweights.push_back(IntDouble(i, -1));

        // who is a new loser?
        set<int> newlosers;
        set<int> newwinners;
        Ints seatings(cands.size(),0);
        for(int i = 0; i < mp.order_c.size(); ++i){
            if(mp.order_a[i] == 0){
                newlosers.insert(mp.order_c[i]);
            }
            else{
                seatings[mp.order_c[i]] = 1;
                newwinners.insert(mp.order_c[i]);
            }
        }
        
        Ints outputL(cands.size());
        //Ints outputW(cands.size());

        cout << "Losers : ";
        for(set<int>::const_iterator st = newlosers.begin();
            st != newlosers.end(); ++st){
            cout << *st << " ";
        }
        cout << endl;

        cout << "Orig Winners : ";
        for(set<int>::const_iterator st = elected_o.begin();
            st != elected_o.end(); ++st){
            cout << *st << " ";
        }
        cout << endl;

        Ints::iterator it = set_intersection(newlosers.begin(), newlosers.end(),
            elected_o.begin(), elected_o.end(), outputL.begin());
        //Ints::iterator it1 = set_difference(newwinners.begin(), newwinners.end(),
        //    elected_o.begin(), elected_o.end(), outputW.begin());
               
        outputL.resize(it-outputL.begin());
        //outputW.resize(it1-outputW.begin());
 
        if(!outputL.empty()){
            const Candidate &c = cands[outputL.front()];
            cout << "New loser " << c.id << endl;
            for(int j = 0; j < c.bweights.size(); ++j){
                const IntDouble &id = c.bweights[j];

                bweights[id.id].weight = max(bweights[id.id].weight, id.weight);
            }

            cout << "Sorting weights" << endl;
            sort(bweights.begin(), bweights.end());
            cout << "Finished sorting weights" << endl;
        }

        /*if(!outputW.empty()){
            const Candidate &c = cands[outputW.front()];
            cout << "New winner " << c.id << endl;
           
            const int idx = c.index; 
            sort(mcopy.toadd.begin(), mcopy.toadd.end(), 
                [idx,seatings](const Change &c1, const Change &c2){
                    pair<int,int> pos_c1 = PositionIn(idx, get<0>(c1),seatings);
                    pair<int,int> pos_c2 = PositionIn(idx, get<0>(c2), seatings);
                    return pos_c1 < pos_c2;
                });
            cout << "Sorted 'toadds' based on new winner position" << endl;
        }*/

        Ints c2pos(cands.size(), cands.size());
        int ccntr = 0;
        for(int i = 0; i < cands.size(); ++i){
            if(find(config.ignore_cands.begin(), config.ignore_cands.end(),
                i) != config.ignore_cands.end())
                continue;
            c2pos[mcopy.order_c[ccntr]] = ccntr;
            ++ccntr;
        }

        double tot_deleted = 0;
        double actual_manipulation = 0;

        double origvotes = 0;
        for(int i = 0; i < ballots.size(); ++i){
            origvotes += ballots[i].votes;
        }

        int tot_to_be_deleted = SumPairVal(mcopy.todel);
        int tot_to_be_added = SumPairVal(mcopy.toadd);

        if(applyhalf){
            tot_to_be_deleted = ceil(tot_to_be_deleted*applyfrac);
            tot_to_be_added = ceil(tot_to_be_added*applyfrac);
        }

        if(!config.addonly && !config.deleteonly &&
            tot_to_be_deleted != tot_to_be_added){
            int val = min(tot_to_be_deleted, tot_to_be_added);
            tot_to_be_deleted = val;
            tot_to_be_added = val;
        }

        ofile << "Total to be deleted " << tot_to_be_deleted << endl;
        ofile << "Total to be added " << tot_to_be_added << endl;

        double tot_added_to_profile = 0;
        if(!config.addonly){
            for(int i = 0; i < candcopy.size(); ++i){
                candcopy[i].Reset();            
            }      
            // For each type of ballot in ballots, check whether some should 
            // be deleted, build up new set of ballots (newballots).
            ofile << "TOTAL TO BE DELETED " << tot_to_be_deleted << endl;
            for(int i = 0; i < bweights.size(); ++i){
                Ballot &b = newballots[bweights[i].id];
                if(tot_deleted == tot_to_be_deleted){
                    Candidate &cand = candcopy[b.prefs[0]];
                    cand.ballots.push_back(b.tag);
                    cand.sum_votes += b.votes;
                    cand.fp_votes += b.votes;
                    tot_added_to_profile += b.votes;
                    continue;
                }

                if(!mcopy.todel.empty()){
                    Ints eqc;
                    ReduceToEQ(config, b.prefs, c2pos, eqc);

                    if(eqc.empty())
                        continue;
 
                    int delled = 0;
                    VCIT it = mcopy.todel.begin();
                    while(it != mcopy.todel.end()){
                        // Is it->first a match of eqc?
                        if(IsPrefix(get<0>(*it), eqc)){ 
                            double todel = get<1>(*it);
                            double rtodel = todel;
                            if(applyhalf && tot_deleted+todel > 
                                tot_to_be_deleted){
                                rtodel = (tot_to_be_deleted - tot_deleted);
                            }

                            if(b.votes >= rtodel){
                                b.votes -= rtodel;
                                tot_deleted += rtodel;
                                if(rtodel == todel)
                                    it = mcopy.todel.erase(it);
                                else{
                                    get<1>(*it) -= rtodel;
                                    ++it;
                                }

                                delled += rtodel;
                                fm.todel.push_back(make_tuple(
                                    b.prefs, rtodel, b.tag , -1));
                            }
                            else{ // navail < rtodel
                                get<1>(*it) -= b.votes;
                                tot_deleted += b.votes;
                                delled += b.votes;
                                fm.todel.push_back(make_tuple(
                                    b.prefs, b.votes, b.tag, -1));
                                b.votes = 0;
                                ++it;
                            }
                            if(b.votes == 0)
                                break;
                        }
                        else{
                            ++it;
                        }
                        if(tot_deleted == tot_to_be_deleted)
                            break;
                    }
                }
            
                Candidate &cand = candcopy[b.prefs[0]];
                cand.ballots.push_back(b.tag);
                cand.sum_votes += b.votes;
                cand.fp_votes += b.votes;
                tot_added_to_profile += b.votes;
            }
        }
        ofile << "TOTAL DELETED " << tot_deleted << endl;

        double tot_added = 0;
        int cntr = newballots.size();
        ofile << origvotes << " " << tot_added_to_profile << endl;
        if(!config.deleteonly){
            if(config.addonly){
                for(int i = 0; i < newballots.size(); ++i){
                    const Ballot &b = newballots[i];
                    Candidate &cand = candcopy[b.prefs[0]];
                    cand.ballots.push_back(i);
                    cand.sum_votes += b.votes;
                    cand.fp_votes += b.votes;
                    continue;
                }
            }
        
            // Add ballots (do not exceed number deleted).
            for(int i = 0; i < mcopy.toadd.size(); ++i){
                if(tot_added == tot_to_be_added)
                    break;

                const Ints &prefs = get<0>(mcopy.toadd[i]);
                int nvotes = get<1>(mcopy.toadd[i]);
                int rnvotes = nvotes;
                if(tot_added + nvotes > tot_to_be_added){
                    rnvotes = (tot_to_be_added - tot_added);
                }
                
                if(!config.addonly){
                    if(tot_added + rnvotes > tot_deleted){
                        rnvotes -= (tot_added + rnvotes - tot_deleted);
                    }
                }

                tot_added += rnvotes;

                Ballot bt;
                bt.prefs = prefs;
                bt.tag = cntr++;
                bt.votes = rnvotes;

                Candidate &cand = candcopy[prefs[0]];
                cand.sum_votes += rnvotes;
                cand.ballots.push_back(bt.tag);
    
                newballots.push_back(bt);
                fm.toadd.push_back(make_tuple(bt.prefs, rnvotes, -1,-1));
                tot_added_to_profile += rnvotes;
    
                if(!config.addonly){
                    if(tot_added >= tot_deleted)
                        break;
                }
            }
        }

        ofile << "Added " << tot_added << " Deleted " << tot_deleted << endl;
        Doubles votecounts;
        double totvotes = 0;
        for(int i = 0; i < newballots.size(); ++i){
            votecounts.push_back(newballots[i].votes);
            totvotes += newballots[i].votes;
        }
        ofile << "Total votes " << totvotes << " " << tot_added_to_profile <<
            " " <<  origvotes << endl;

        // Test if manipulation 'fm' works
        bool alternate = TestManipulation(candcopy, fm, ac_format, ac_seats,
            tprefix); 
        
        double extra = 0;
        if(!alternate){
            ofile<<"Manipulation does not actually change result."<<endl;
            // Run my simulator to populate some attributes in candidate recs
            Ints simorderc;
            Ints simordera;

            Config nconfig(config);
            if(nconfig.original_votes != -1)
                nconfig.original_votes += (tot_added - tot_deleted);

            SimSTV(newballots, votecounts, candcopy, nconfig, 
                simorderc,simordera, false, cout);  

            extra = ceil(WEUB(newballots, votecounts, candcopy, config,
                elected_o, origc, origa, true, ofile, fm, ballots.size(),
                ac_format, ac_seats, tprefix));
        }

        if(config.addonly){
            actual_manipulation = extra + tot_added;
        }
        else if(config.deleteonly){
            actual_manipulation = extra + tot_deleted;
        }
        else{
            actual_manipulation = extra + max(tot_deleted, tot_added);       
        }
        
        ofile << "Actual manipulation required = " << actual_manipulation 
            << " votes." << endl; 
        ofile.close();
        return ceil(actual_manipulation);
    }   
    catch(STVException &e){
        throw e;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(...){
        throw STVException("Unexpected error in RunManipulation.");
    }

}

bool TestRemove(int amount, const Candidate &cd, const Ballots &ballots, 
    const Candidates &cands, const Config &config, 
    const std::set<int> &elected, Manipulation &cmanip, int nodeleteafter,
    const char *ac_format, int ac_seats, const char *tprefix)
{
    try
    {
        cout << "Testing removal of " << amount << " from " << cd.id << " ";
        Ballots newballots(ballots);

        int rem = amount;
        if(cd.fp_votes >= rem){
            // Do the easy manipulation
            for(int j = 0; j < cd.bweights.size(); ++j){
                int bid = cd.bweights[j].id;
                if(bid >= nodeleteafter)
                    continue;
                const Ballot &bl = newballots[bid];
                if(bl.prefs[0] != cd.index)
                    continue;

                double avail = bl.votes;
                if(avail >= rem){
                    avail -= rem;
                    cmanip.todel.push_back(make_tuple(bl.prefs, rem,bid,-1));
                    rem = 0;
                }
                else{
                    rem -= avail;
                    cmanip.todel.push_back(make_tuple(bl.prefs, avail,bid,-1));
                    avail = 0;
                }
                newballots[bid].votes = avail;
                if(rem == 0)
                    break;
            }
            if(rem > 0){
                return false;
            }
        }
        else{
            for(int j = 0; j < cd.bweights.size(); ++j){
                if(rem == 0)
                    break;

                int bid = cd.bweights[j].id;
                if(bid >= nodeleteafter)
                    continue;

                Ballot &bt = newballots[bid];
                if(rem <= bt.votes){
                    bt.votes -= rem;
                    cmanip.todel.push_back(make_tuple(bt.prefs,rem,bid,-1));
                    rem = 0;
                }
                else{
                    rem -= bt.votes;
                    cmanip.todel.push_back(make_tuple(
                        bt.prefs, bt.votes, bid, -1));
                    bt.votes = 0;
                }
            }
            
            if(rem > 0){
                return false;
            }
        }
        bool alternate = false;
        if(ac_format){
            // Print manipulation to file, run Andrew's code and determine
            // whether a change has taken place. 
            alternate = TestManipulation(cands, cmanip, ac_format, ac_seats,
                tprefix);
        }
        else{
            Candidates candcopy(cands);
            for(int i = 0; i < candcopy.size(); ++i){
                candcopy[i].Reset();
            }

            Doubles votecounts(newballots.size(), 0);
            double vcount = 0;
            for(int i = 0; i < newballots.size(); ++i){
                const Ballot &b = newballots[i];
                vcount += b.votes;
                votecounts[i] = b.votes;
                Candidate &c = candcopy[b.prefs[0]];

                c.ballots.push_back(i);
                c.sum_votes += b.votes;
                c.fp_votes += b.votes;
            }

            // Simulate hypothetical election. 
            Ints simorderc;
            Ints simordera;

            Config nconfig(config);
            if(nconfig.original_votes != -1)
                nconfig.original_votes -= amount;

            SimSTV(newballots, votecounts, candcopy, nconfig, 
                simorderc,simordera, false, cout);  

            for(int k = 0; k < candcopy.size(); ++k){
                const Candidate &c = candcopy[k];
                if(c.seat != -1 && elected.find(k) == elected.end()){
                    // We have found an alternate outcome
                    alternate = true;
                    cmanip.order_c = simorderc;
                    cmanip.order_a = simordera;
                    break;
                }   
            }
        }

        cout << alternate << endl;
        return alternate;

    }
    catch(STVException &e){
        throw e;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(...){
        throw STVException("Unexpected error in TestRemove.");
    }
}

bool TestAdd(int amount, const Candidate &cd, const Ballots &ballots, 
    const Candidates &cands, const Config &config, 
    const std::set<int> &elected, Manipulation &cmanip,
    const char *ac_format, int ac_seats, const char *tprefix)
{
    try
    {
        cout << "Testing addition of " << amount << " to " <<
            cd.id << " ";
        bool alternate = false;
        if(ac_format != NULL){
            Ints prefs;
            prefs.push_back(cd.index);
            cmanip.toadd.push_back(make_tuple(prefs, amount, ballots.size(),-1));
            
            // Print manipulation to file, run Andrew's code and determine
            // whether a change has taken place. 
            alternate = TestManipulation(cands, cmanip, ac_format, ac_seats,
                tprefix);
        }
        else{
            Ballots newballots(ballots);

            Ballot c2ballot;
            c2ballot.tag = newballots.size();
            c2ballot.votes = amount;
            c2ballot.prefs.push_back(cd.index);
            newballots.push_back(c2ballot);
            cmanip.toadd.push_back(make_tuple(c2ballot.prefs, amount, c2ballot.tag,-1));

            Candidates candcopy(cands);
            for(int i = 0; i < candcopy.size(); ++i){
                candcopy[i].Reset();
            }
            
            Doubles votecounts(newballots.size(), 0);
            double vcount = 0;
            for(int i = 0; i < newballots.size(); ++i){
                const Ballot &b = newballots[i];
                vcount += b.votes;
                votecounts[i] = b.votes;
                Candidate &c = candcopy[b.prefs[0]];

                c.ballots.push_back(i);
                c.sum_votes += b.votes;
                c.fp_votes += b.votes;
            }

            // Simulate hypothetical election. 
            Ints simorderc;
            Ints simordera;

            Config nconfig(config);
            if(nconfig.original_votes != -1)
                nconfig.original_votes += amount;
       
            
            SimSTV(newballots, votecounts, candcopy, nconfig, 
                simorderc,simordera, false, cout);  

            for(int k = 0; k < candcopy.size(); ++k){
                const Candidate &c = candcopy[k];
                if(c.seat != -1 && elected.find(k) == elected.end()){
                    // We have found an alternate outcome
                    alternate = true;
                    cmanip.order_c = simorderc;
                    cmanip.order_a = simordera;
                    break;
                }   
            }
        }

        cout << alternate << endl;
        return alternate;

    }
    catch(STVException &e){
        throw e;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(...){
        throw STVException("Unexpected error in TestAdd.");
    }
}



void FindBestManipulation(const vector<ChangeTest> &totest, 
    Manipulation &changes, int &weub, const Candidates &origcand, 
    const Ballots &ballots, const Config &config, 
    const std::set<int> &elected, int nodeleteafter,
    const char *ac_format, int ac_seats, const char *tprefix)
{
    try
    {
        vector<ChangeTest> cts = totest;
        sort(cts.begin(), cts.end());

        Ints2d history(origcand.size());

        weub = -1;

        //cout << "Sorting ballots" << endl;
        Candidates cand(origcand);
        for(set<int>::const_iterator it = elected.begin();
            it != elected.end(); ++it){
            Candidate &c = cand[*it];
            sort(c.bweights.begin(), c.bweights.end());
            history[*it] = Ints(origcand.size(), 0);
        }
        //cout << "Ballots sorted" << endl;

        Manipulation base(changes);
        for(int i = 0; i < cts.size(); ++i){
            const ChangeTest &ct = cts[i];
            if(history[ct.c1][ct.c2] == 1){
                throw STVException("My assumption was not valid!");
            }
            if(weub != -1 && ct.amount > 3*weub)
                continue;
 
            history[ct.c1][ct.c2] = 1;
            Manipulation cmanip(base);
            int tryvalue = ct.amount;
            if(weub != -1)
                tryvalue = min(tryvalue,weub);

            bool verified = TestShift(tryvalue, cand[ct.c1], 
                cand[ct.c2], ballots, cand, 
                config, elected, cmanip, nodeleteafter,
                ac_format, ac_seats, tprefix);
             
            if(verified){  
                changes = cmanip;
                weub = tryvalue;
                int DELTA = ceil(tryvalue/2.0);

                vector<pair<int,bool> > tested;
                tested.push_back(make_pair(tryvalue, true));

                while(true){
                    Manipulation cmanip1(base);
                    tryvalue = tryvalue-DELTA;
                    if(tryvalue > 0){
                        bool found = false;
                        bool found_v = false;
                        for(int i = 0; i < tested.size(); ++i){
                            if(get<0>(tested[i]) == tryvalue){
                                found = true;
                                found_v = get<1>(tested[i]);
                                break;
                            }
                        }
                        if(found){
                            verified = found_v;
                        }
                        else{ 
                            verified = TestShift(tryvalue, cand[ct.c1], 
                                cand[ct.c2], ballots, cand, 
                                config, elected, cmanip1, nodeleteafter, 
                                ac_format, ac_seats, tprefix);
                            tested.push_back(make_pair(tryvalue, verified));
                        }
                    }
                    else{
                        verified = false;
                    }

                    if(verified){
                        changes = cmanip1;
                        weub = tryvalue;
                    }
                    else{
                        tryvalue += DELTA;
                        if(DELTA == 1)
                            break;
                        DELTA = ceil(DELTA/2.0);
                    }
                } 
            }
        }
    }
    catch(STVException &e){
        throw e;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(...){
        throw STVException("Unexpected error in FindBestManipulation.");
    }
}


bool TestShift(int amount, const Candidate &c1, const Candidate &c2, 
    const Ballots &ballots, const Candidates &cands, const Config &config, 
    const std::set<int> &elected, Manipulation &cmanip, int nodeleteafter,
    const char *ac_format, int ac_seats, const char *tprefix)
{
    try
    {
        cout << "Testing shift of " << amount << " from " <<
            c1.id << " to " << c2.id << " ";
        Ballots newballots(ballots);

        Ballot c2ballot;
        c2ballot.prefs.push_back(c2.index);
 
        int rem = amount;
        int cntr = newballots.size();
        //IDS c1_bweights = c1.bweights;
        //sort(c1_bweights.begin(), c1_bweights.end());

        if(c1.fp_votes >= rem){
            // Do the easy manipulation
            for(int j = 0; j < c1.bweights.size(); ++j){
                if(rem == 0)
                    break;

                int bid = c1.bweights[j].id;
                if(bid >= nodeleteafter)
                    continue;
                const Ballot &bl = newballots[bid];
                if(bl.prefs[0] != c1.index)
                    continue;

                double avail = bl.votes;

                if(avail >= rem){
                    avail -= rem;
                    cmanip.todel.push_back(make_tuple(bl.prefs, rem,bid,-1));
                    rem = 0;
                }
                else{
                    rem -= avail;
                    cmanip.todel.push_back(make_tuple(bl.prefs, avail,bid,-1));
                    avail = 0;
                }
                newballots[bid].votes = avail;
            }
            if(rem > 0){
                cout << "Could not find enough FP votes to shift" << endl;
                cout << "NO" << endl;
                return false;
            }
            
            Ballot nb(c2ballot);
            nb.tag = cntr++;
            nb.votes = amount;
            newballots.push_back(nb);
            cmanip.toadd.push_back(make_tuple(c2ballot.prefs, amount,-1,-1));
        }
        else{
            for(int j = 0; j < c1.bweights.size(); ++j){
                if(rem == 0)
                    break;

                int bid = c1.bweights[j].id;
                if(bid >= nodeleteafter)
                    continue;

                Ballot &bt = newballots[bid];
                bool shift = false;
                for(int k = 0; k < bt.prefs.size(); ++k){
                    if(bt.prefs[k] == c1.index){
                        shift = true;
                        break;
                    }
                    if(bt.prefs[k] == c2.index)
                        break;
                }
                if(shift){
                    if(rem <= bt.votes){
                        Ballot toadd(c2ballot);
                        toadd.tag = cntr++;
                        toadd.votes = rem;
                        cmanip.toadd.push_back(make_tuple(
                            c2ballot.prefs, rem, -1, -1));

                        bt.votes -= rem;
                        cmanip.todel.push_back(make_tuple(bt.prefs,rem,bid,-1));
                        rem = 0;
                        newballots.push_back(toadd);
                    }
                    else{
                        Ballot toadd(c2ballot);
                        toadd.tag = cntr++;
                        toadd.votes = bt.votes;
                        rem -= toadd.votes;
                        cmanip.toadd.push_back(make_tuple(
                            c2ballot.prefs, toadd.votes, -1, -1));
                        cmanip.todel.push_back(make_tuple(
                            bt.prefs, toadd.votes, bid, -1));
                        bt.votes = 0;
                        newballots.push_back(toadd);
                    }
                }
            }
            
            if(rem > 0){
                cout << "Could not find enough votes to shift" << endl;
                cout << "NO" << endl;
                return false;
            }
        }
        bool alternate = false;
        if(ac_format != NULL){
            // Print manipulation to file, run Andrew's code and determine
            // whether a change has taken place. 
            alternate = TestManipulation(cands, cmanip, ac_format, ac_seats,
                tprefix);
        }
        else{
            Candidates candcopy(cands);
            for(int i = 0; i < candcopy.size(); ++i){
                candcopy[i].Reset();
            }

            Doubles votecounts(newballots.size(), 0);
            double vcount = 0;
            for(int i = 0; i < newballots.size(); ++i){
                const Ballot &b = newballots[i];
                vcount += b.votes;
                votecounts[i] = b.votes;
                Candidate &c = candcopy[b.prefs[0]];

                c.ballots.push_back(i);
                c.sum_votes += b.votes;
                c.fp_votes += b.votes;
            }

            // Simulate hypothetical election. 
            Ints simorderc;
            Ints simordera;
            alternate = false;

            SimSTV(newballots, votecounts, candcopy, config, 
                simorderc,simordera, false, cout);  

            for(int k = 0; k < candcopy.size(); ++k){
                const Candidate &c = candcopy[k];
                if(c.seat != -1 && elected.find(k) == elected.end()){
                    // We have found an alternate outcome
                    alternate = true;
                    cmanip.order_c = simorderc;
                    cmanip.order_a = simordera;
                    break;
                }   
            }
        }

        cout << alternate << endl;
        return alternate;
    }
    catch(STVException &e){
        throw e;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(...){
        throw STVException("Unexpected error in TestShift.");
    }

}

double GetWeightOf(int bid, const Candidate &c){
    for(int i = 0; i < c.bweights.size(); ++i){
        const IntDouble &id = c.bweights[i];
        if(id.id == bid)
            return id.weight;
    }
    return -1;
}


bool TestManipulation(const Candidates &cands, const Manipulation &cmanip,
    const char *ac_format, int ac_seats, const char *tprefix)
{
    try
    {
        // Print manipulation to file, run Andrew's code and determine
        // whether a change has taken place. 
        stringstream mfile_name;
        mfile_name << tprefix << "_tmp.manip";
        ofstream mfile(mfile_name.str());
        mfile << "Add Ballots" << endl;
        for(int j = 0; j < cmanip.toadd.size(); ++j){
            const Ints &pfs = get<0>(cmanip.toadd[j]);
            int n = get<1>(cmanip.toadd[j]);
            PrintBallot(mfile, pfs, cands);
            mfile << " : " << n << endl; 
        }
        mfile << "Remove Ballots" << endl;
        for(int j = 0; j < cmanip.todel.size(); ++j){
            const Ints &pfs = get<0>(cmanip.todel[j]);
            int n = get<1>(cmanip.todel[j]);
            PrintBallot(mfile, pfs, cands);
            mfile << " : " << n << endl; 
        }
        mfile.close();

        // run Andrew's code
        stringstream cmd;
        cmd << "java -jar PublicService/CountVotes/target/scala-2.12/"
            << "CountPreferentialVotes-assembly-1.0.jar --stv " << ac_format 
            << " --NumSeats " << ac_seats << " --modify " << mfile_name.str()
            << " > " << tprefix << "_tmp.out"; 
        string result = exec(cmd.str().c_str());
        //cout << result << endl;

        // Read tmp.out to see if a change of winner took place. Read tmp.out
        // and look for line that starts with "Change: "
        bool alternate = false;
        stringstream ifile_name;
        ifile_name << tprefix << "_tmp.out";
        ifstream ifile(ifile_name.str());
        string line;
        while(getline(ifile, line)){
            if(boost::starts_with(line, "Change : List")){
                size_t found1 = line.find('+');
                size_t found2 = line.find('-');
                if(found1 != string::npos && found2 != string::npos){
                    alternate = true;
                }
                break;
            }
        }
    
        ifile.close();
        return alternate;
    }
    catch(STVException &e){
        throw e;
    }
    catch(exception &e){
        throw STVException(string(e.what()));
    }
    catch(...){
        throw STVException("Unexpected error in TestManipulation.");
    }    
}
