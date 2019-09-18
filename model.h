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


#ifndef _MODEL_H
#define _MODEL_H

#include<vector>
#include<set>
#include<exception>
#include<boost/lexical_cast.hpp>
#include<boost/filesystem.hpp>
#include<boost/tokenizer.hpp>
#include<boost/algorithm/string.hpp>
#include<map>
#include<iostream>

#ifndef _WIN32
#include<sys/time.h>
#endif


typedef std::vector<int> Ints;
typedef std::set<int> SInts;
typedef std::vector<Ints> Ints2d;
typedef std::vector<SInts> SInts2d;
typedef std::vector<Ints2d> Ints3d;
typedef std::vector<SInts2d> SInts3d;

typedef std::vector<double> Doubles;
typedef std::vector<Doubles> Doubles2d;

struct mytimespec
{
    double seconds;
};

void GetTime(struct mytimespec* t);

typedef std::tuple<Ints,int,int,double> Change;
typedef std::vector<Change> Changes;
typedef std::vector<Changes> Changes2d;
typedef std::vector<Changes2d> Changes3d;




struct Manipulation
{
    Ints order_c;
    Ints order_a;

    Changes toadd;
    Changes todel;

    Manipulation(){}
    Manipulation(const Manipulation &m) : order_c(m.order_c),
        order_a(m.order_a) {
        toadd.clear();
        todel.clear();
        toadd.insert(toadd.begin(), m.toadd.begin(), m.toadd.end());
        todel.insert(todel.begin(), m.todel.begin(), m.todel.end());
    }
};

typedef std::vector<Manipulation> Manipulations;

bool CompareManipulations(const Manipulation &m1, const Manipulation &m2);

struct Result
{
    double curr_ubound;
    double curr_lbound;
    Manipulations mps;

    std::string err_message;

    Result() : curr_ubound(-1), curr_lbound(-1), err_message("") {}
    Result(double _ub, double _lb) : curr_ubound(_ub), curr_lbound(_lb), 
        err_message("")  {}
};

typedef std::vector<Result> Results;

struct Config
{
    int ncandidates;
    int nseats;
    double totalvotes;
    double quota;
    double quota_override;

    int original_votes;
    int original_seats;

    double glb;
    double gub;

    double tlimit_wsol;
    double tlimit_wosol;
    double tlimit_leaf;

    int DIV_BILIN;
    int FIX_UNTIL;

    int subthreads;
    
    bool deleteonly;
    bool addonly;
    int ignore_first;
    Ints ignore_cands;

    std::map<int,int> id2index;

    Config() : ncandidates(0), nseats(0), totalvotes(0),
        quota(0), quota_override(-1), original_votes(-1), 
        original_seats(-1), glb(0), gub(-1), tlimit_wsol(-1),
        tlimit_wosol(-1), tlimit_leaf(-1), DIV_BILIN(1), FIX_UNTIL(0), 
        subthreads(1), deleteonly(false), addonly(false),ignore_first(-1){}
};


class STVException
{
    private:
        const std::string message;

    public:
        STVException(const STVException &me) : message(me.what()){}
        STVException(const std::string &str) : message(str) {}
        const std::string& what() const { return message; }
};

struct IntDouble
{
    int id;
    double weight;

    IntDouble(int _id, double _weight) : 
        id(_id), weight(_weight) {}

    bool operator<(const IntDouble &other) {
        return weight > other.weight;
    }
};

typedef std::vector<IntDouble> IDS;
typedef std::vector<IDS> IDS2d;
typedef std::list<IntDouble> L_IDS;

struct Candidate
{
    int id;
    int index;
    double sum_votes;
    double fp_votes;    

    Ints ballots;

    // For simulation
    double sim_votes;
    IDS bweights;

    int standing;
    int seat;
    int surplus;

    double max_votes;

    Candidate() : id(0), index(0), sum_votes(0), fp_votes(0), sim_votes(0), 
        standing(1), seat(-1), surplus(0), max_votes(0) {}

    void Reset(){
        fp_votes = 0;
        sum_votes = 0;
        ballots.clear();
        sim_votes = 0;
        bweights.clear();
        standing = 1;
        seat = -1;
        surplus = 0;
        max_votes = 0;
    }
};

typedef std::vector<Candidate> Candidates;

struct Ballot
{
    int tag;
    double votes;
    Ints prefs;

    double wval;

    bool operator<(const Ballot &other) {
        return wval > other.wval;
    }

};

typedef std::vector<Ballot> Ballots;

template <typename T>
T ToType(const std::string &s);

typedef std::map<std::vector<int>,int> I2Map;

struct Node{
    double dist;
    double dist1;

    Ints order_c;
    Ints order_a;
    Ints remcand;
    std::set<int> elected;

    int seatsleft;

    Ballots rev_ballots;
    I2Map ballotmap;
    Ints bid2newid;
	Ints2d newid2bids;

    Ints cand_proc;
    Ints cand_act;
    Ints cand_equota;

    SInts3d poss_tally;
    Ints3d d_ij_slist;

    int last_elim;
    int last_q;

    Doubles2d max_votes;

    SInts2d c_clumped_order;
    Ints a_clumped_order; 

    Node(int ncand, int nballots) {
        bid2newid.resize(nballots, -1);
        max_votes.resize(ncand);
        poss_tally.resize(ncand);
        d_ij_slist.resize(ncand);
        last_elim = -1;
        last_q = -1;
        dist = -1;
        seatsleft = 0;
        dist1 = 0;

        cand_proc.resize(ncand, -1);
        cand_act.resize(ncand, -1);
        cand_equota.resize(ncand, -1);
    }

    void Reset(){
        last_elim = -1;
        last_q = -1;
        dist = -1;
        seatsleft = 0;
        dist1 = 0;

        rev_ballots.clear();
        ballotmap.clear();
        elected.clear();
        remcand.clear();
        newid2bids.clear();

        for(int i = 0; i < cand_proc.size(); ++i){
            cand_proc[i] = -1;
            cand_act[i] = -1;
            cand_equota[i] = -1;

            max_votes[i].clear();
            poss_tally[i].clear();
            d_ij_slist[i].clear();
        }

        for(int i = 0; i < bid2newid.size(); ++i){
            bid2newid[i] = -1;
        }
    }

    void ClearEqClassData(){
        rev_ballots.clear();
        ballotmap.clear();
        for(int i = 0; i < cand_proc.size(); ++i){
            cand_proc[i] = -1;
            cand_act[i] = -1;
            cand_equota[i] = -1;

            max_votes[i].clear();
            poss_tally[i].clear();
            d_ij_slist[i].clear();
        }
    }
};


void PrintBallot(std::ostream &of, const Ints &prefs, const Candidates &cands);

bool ReadBallots(const char *path, Ballots &ballots,
    Candidates &candidates, Config &config);

bool LoadManipulation(const char *path, const Config &config, 
    const Candidates &cands, Manipulations &mps);

void ReduceToEQ(const Config &config, const Ints &prefs, const Ints &c2pos, Ints &eqc);
#endif
