#include<fstream>
#include<algorithm>
#include<iostream>
#include<sstream>
#include<time.h>
#include<boost/algorithm/string/predicate.hpp>
#include "model.h"

using namespace std;
typedef boost::char_separator<char> boostcharsep;


void GetTime(struct mytimespec *t)
{
	#ifndef _WIN32
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t->seconds = tv.tv_sec + (tv.tv_usec/1E6);
	#else
	t->seconds = clock()/CLOCKS_PER_SEC;
	#endif
}

template <typename T>
T ToType(const std::string &s) 
{ 
	try
	{
		return boost::lexical_cast<T>(s); 
	}
	catch(...)
	{
		throw STVException("Lexical cast problem");
	}
}


void PrintBallot(ostream &of, const Ints &prefs, const Candidates &cands){
    of << "(";
    for(int i = 0; i < prefs.size()-1; ++i){
        int cid = cands[prefs[i]].id;
        of << cid << ",";
    }
    int cid = cands[prefs.back()].id;
    of << cid << ")";
}

bool CompareManipulations(const Manipulation &m1, const Manipulation &m2){
    if(m1.toadd.size() < m2.toadd.size())
        return true;
    return false;
}

void ReduceToEQ(const Config &config, const Ints &prefs, const Ints &c2pos,
    Ints &eqc)
{
    // Reduce prefs to an equivalence class given desired elimination order
    Ints prefs_mod;
    int maxpos = -1;
    
    for(int i = 0; i < prefs.size(); ++i){
        int cidx = prefs[i];
        if(find(config.ignore_cands.begin(), config.ignore_cands.end(), cidx)
            != config.ignore_cands.end()){
            continue;
        }
        int pos = c2pos[cidx];
        if(pos == -1)
            pos = prefs.size()+1;
        if(maxpos == -1 || pos > maxpos){
            eqc.push_back(prefs[i]);
            maxpos = pos;
        }

        if(pos == prefs.size()+1)
            break;
    }
    
    return;
}


void Split(const string &line, const boostcharsep &sep, vector<string> &r)
{
	try
	{
		boost::tokenizer<boostcharsep> tokens(line, sep);
		r.assign(tokens.begin(), tokens.end());

		for(int i = 0; i < r.size(); ++i)
		{
			boost::algorithm::trim(r[i]);
		}
	}
	catch(exception &e)
	{
		stringstream ss;
		ss << e.what();
		throw STVException(ss.str());
	}
	catch(STVException &e)
	{
		throw e;
	}
	catch(...)
	{
		throw STVException("Error in Split Function.");
	}
}

// Assumed input format:
// (first_id, second_id, third_id, ...) : #appears
bool ReadBallots(const char *path, Ballots &ballots, Candidates &candidates,
	Config &config)
{
	try
	{
		ifstream infile(path);

		// First line is list of candidates.
		//cout << "Reading Candidates" << endl;
		boostcharsep spcom(",");
		string line;
		getline(infile, line);

		vector<string> columns;
		Split(line, spcom, columns);

		config.ncandidates = columns.size();
		for(int i = 0; i < columns.size(); ++i)
		{
			int id = ToType<int>(columns[i]);
			Candidate c;
			c.index = i;
			c.id = id;

			candidates.push_back(c);

			config.id2index.insert(pair<int,int>(id,i));
		}

        getline(infile,line);
        columns.clear();
        Split(line, spcom, columns);
        config.nseats = ToType<int>(columns[1]);

		// Skip next line (separator)
		getline(infile, line);

		//cout << "Reading Ballots" << endl;
		boostcharsep sp(",():");
		
		int cntr = ballots.size();
        config.totalvotes = 0;
		while(getline(infile, line))
		{
			vector<string> columns;
			Split(line, sp, columns);

			Ballot b;
			b.tag = cntr;
			b.votes = ToType<double>(columns.back());

			for(int i = 0; i < columns.size()-1; ++i)
			{
				if(columns[i] == "") continue;
				int ccode = ToType<int>(columns[i]);
				int index = config.id2index.find(ccode)->second;
					
				if(find(b.prefs.begin(),
					b.prefs.end(), index) != b.prefs.end())
				{
					continue;
				}

				b.prefs.push_back(index);
			}

			if(b.prefs.empty()) continue;

			Candidate &cand = candidates[b.prefs.front()];
			bool found_ballot = false;
			int bid = -1;
			/*for(int j = 0; j < cand.ballots.size(); ++j){
				const Ballot &bf = ballots[cand.ballots[j]];
				if(bf.prefs == b.prefs){
					bid = bf.tag;
					found_ballot = true;
					break;
				}
			}*/
			if(found_ballot){
				ballots[bid].votes += b.votes;
				cand.sum_votes += b.votes;
				config.totalvotes += b.votes;
				continue;
			}
			else{
				cand.sum_votes += b.votes;
				cand.ballots.push_back(b.tag);

				config.totalvotes += b.votes;
	
				ballots.push_back(b);
				++cntr;
			}
		}

		//cout << "Finished reading ballots (" << config.totalvotes <<
		//	" ballots read)." << endl;
		infile.close();
	}
	catch(exception &e)
	{
		throw e;
	}
	catch(STVException &e)
	{
		throw e;
	}
	catch(...)
	{
		cout << "Unexpected error reading in ballots." << endl;
		return false;
	}

	return true;
}

bool LoadManipulation(const char *path, const Config &config, 
    const Candidates &candidates, Manipulations &manips)
{
	try
	{
		ifstream infile(path);
        string line;
        
		boostcharsep sp(",():");
		boostcharsep ws(" -");
	
		while(getline(infile, line))
		{
            if(boost::starts_with(line, "Manipulation")){
                getline(infile, line);
                vector<string> tokens;
                Split(line, ws, tokens);
                
                Manipulation mp;
                for(int i = 0; i < tokens.size(); i+=2){
                    int cid = ToType<int>(tokens[i]);
                    int act = (tokens[i+1] == "e") ? 0 : 1;
                    mp.order_c.push_back(config.id2index.find(cid)->second); 
                    mp.order_a.push_back(act);
                }

                bool add_ballots = true;
                getline(infile, line);
                while(getline(infile, line)){
                    if(boost::starts_with(line, "Remove")){
                        add_ballots = false;
                        continue;
                    }
                    if(boost::starts_with(line, "End")){
                        break;
                    }

                    // Parse ballot and add to add/remove lists
                    vector<string> columns;
			        Split(line, sp, columns);

                    Ints prefs;
			        for(int i = 0; i < columns.size()-1; ++i)
			        {
				        if(columns[i] == "") continue;
				        int ccode = ToType<int>(columns[i]);
				        int index = config.id2index.find(ccode)->second;
					
	        			if(find(prefs.begin(),prefs.end(),index)!=prefs.end())
				        {
					        continue;
			        	}

				        prefs.push_back(index);
			        }
                    int nvotes = ToType<int>(columns.back());

			        if(!prefs.empty()){
                        if(add_ballots){
                            mp.toadd.push_back(make_tuple(prefs,nvotes,-1,-1));
                        }
                        else{
                            mp.todel.push_back(make_tuple(prefs,nvotes,-1,-1));
                        }
                    }
                    
                }
                manips.push_back(mp);
                /*cout << "Read manipulation with order " << endl;
                for(int i = 0; i < mp.order_c.size(); ++i){
                    cout << candidates[mp.order_c[i]].id << " ";
                    cout << ((mp.order_a[i] == 0) ? "e" : "q") <<  " ";
                }
                cout << endl;
                cout << "Ballots To Add" << endl;
                for(int i = 0; i < mp.toadd.size(); ++i){
                    const Ints &pfs = get<0>(mp.toadd[i]);
                    const int nv = get<1>(mp.toadd[i]);
                    PrintBallot(cout, pfs, candidates);
                    cout << " : " << nv << endl;
                }
                cout << "Ballots To Remove" << endl;
                for(int i = 0; i < mp.todel.size(); ++i){
                    const Ints &pfs = get<0>(mp.todel[i]);
                    const int nv = get<1>(mp.todel[i]);
                    PrintBallot(cout, pfs, candidates);
                    cout << " : " << nv << endl;
                }*/
            }    
        }

		infile.close();
	}
	catch(exception &e)
	{
		throw e;
	}
	catch(STVException &e)
	{
		throw e;
	}
	catch(...)
	{
		cout << "Unexpected error reading in ballots." << endl;
		return false;
	}

	return true;
}

