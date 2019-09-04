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


struct Config
{
    int ncandidates;
    int nseats;
    double totalvotes;
    double quota;
    double quota_override;

    std::map<int,int> id2index;

    Config() : ncandidates(0), nseats(0), totalvotes(0),
        quota(0), quota_override(-1) {}
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
};

typedef std::vector<Ballot> Ballots;

template <typename T>
T ToType(const std::string &s);

typedef std::map<std::vector<int>,int> I2Map;

void PrintBallot(std::ostream &of, const Ints &prefs, const Candidates &cands);

bool ReadBallots(const char *path, Ballots &ballots,
    Candidates &candidates, Config &config);

#endif
