/*
    Copyright 2019 Michelle Blom
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include<iostream>
#include<fstream>
#include<sstream>
#include<string.h>
#include<stdlib.h>
#include<algorithm>
#include "model.h"
#include "sim_stv.h"
#include "stv_distance.h"
#include "tree_stv.h"

using namespace std;

int main(int argc, const char * argv[]) 
{
    try
    {
        Candidates candidates;
        Ballots ballots; 
        Config config;

        double timelimit = 86400;
        bool compbounds = false;

        config.quota = -1;
        double tlimit_wsol = -1;
        double tlimit_wosol = -1;
        double tlimit_leaf = -1;

        const char *logf = NULL;
        const char *manip_file = NULL;
        const char *final_manip = NULL;
        bool run_manip = false;
        bool simlog = false;
        bool justsim = false;

        bool apply_half = false;
        double apply_frac = 0.5;

        const char *ac_format = NULL;
        const char *tprefix = NULL;
        int ac_seats = -1;

        for(int i = 1; i < argc; ++i)
        {
            if(strcmp(argv[i], "-ballots") == 0 && i < argc-1)
            {
                if(!ReadBallots(argv[i+1], ballots, candidates, config))
                {
                    cout << "Ballot read error. Exiting." << endl;
                    return 1;
                }

                ++i;
            }
            else if(strcmp(argv[i], "-applyhalf") == 0 && i < argc-1){
                apply_half = true;
                apply_frac = atof(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-simlog") == 0){
                simlog = true;
            }
            else if(strcmp(argv[i], "-run_manip") == 0){
                run_manip = true;
            }
            else if(strcmp(argv[i], "-ignore_first") == 0 && i < argc-1){
                config.ignore_first = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-ovotes") == 0 && i < argc-1){
                config.original_votes = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-oseats") == 0 && i < argc-1){
                config.original_seats = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-just_sim") == 0){
                justsim = true;
            }
            else if(strcmp(argv[i], "-tlimit")== 0 && i < argc-1){
                timelimit = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-sthreads") == 0 && i < argc-1){
                config.subthreads = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-tlimit_leaf")== 0 && i < argc-1){
                tlimit_leaf = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-tlimit_wsol")== 0 && i < argc-1){
                tlimit_wsol = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-tlimit_wosol")== 0 && i < argc-1){
                tlimit_wosol = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-logfile")== 0 && i < argc-1){
                logf = argv[i+1];
                ++i;
            }
            else if(strcmp(argv[i], "-tprefix")== 0 && i < argc-1){
                tprefix = argv[i+1];
                ++i;
            }
            else if(strcmp(argv[i], "-ac_format")== 0 && i < argc-2){
                ac_format = argv[i+1];
                ac_seats = atoi(argv[i+2]);
                i+=2;
            }
            else if(strcmp(argv[i], "-manipfile")== 0 && i < argc-1){
                manip_file = argv[i+1];
                ++i;
            }
            else if(strcmp(argv[i], "-finalmanip")== 0 && i < argc-1){
                final_manip = argv[i+1];
                ++i;
            }
            else if(strcmp(argv[i], "-fixuntil") == 0 && i < argc-1){
                config.FIX_UNTIL = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-quota_override")== 0 && i < argc-1){
                config.quota_override = atof(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-bilin") == 0 && i < argc-1){
                config.DIV_BILIN = atoi(argv[i+1]);
                ++i;
            }
            else if(strcmp(argv[i], "-ao") == 0){
                config.addonly = true;
            }
            else if(strcmp(argv[i], "-do") == 0){
                config.deleteonly = true;
            }
        }

        if(config.addonly && config.deleteonly){
            cout << "Cannot run under both addition only and "
                << "delete only settings." << endl;
            return 1;
        }

        if(tlimit_wsol != -1)
            config.tlimit_wsol = tlimit_wsol;
        if(tlimit_wosol != -1)
            config.tlimit_wosol = tlimit_wosol;
        if(tlimit_leaf != -1)
            config.tlimit_leaf = tlimit_leaf;

        double upperbound = config.totalvotes;
        if(config.quota == -1){
            config.quota = (int)(1.0 + ((double)config.totalvotes/
                (double)(config.nseats+1.0)));
        }

        if(config.quota_override != -1)
            config.quota = config.quota_override;

        mytimespec start;
        GetTime(&start);

        // code to simulate stv
        Ints order_c;
        Ints order_a;
    
        Doubles votecounts(ballots.size(), 0);
        for(int i = 0; i < ballots.size(); ++i){
            votecounts[i] = ballots[i].votes;
        }

        if(!SimSTV(ballots, votecounts, candidates, config, 
            order_c, order_a, simlog, cout))
        {
            cout << "Simulation Failed. Exiting." << endl;
            return 1;
        }

        cout << "Election/Elimination order: ";
        Ints elected(config.ncandidates, 0);

        int seats = 0;
        set<int> elected_list;  
        for(int i = 0; i < order_c.size(); ++i)
        {
            const int indx = order_c[i];
            if(config.ignore_first != -1 && config.ignore_first > i){
                config.ignore_cands.push_back(indx);
            }

            cout << candidates[indx].id << "-";
            if(order_a[i]){
                cout << "q ";
                ++seats;
                elected[indx] =1;
                elected_list.insert(indx);
            }
            else{
                cout << "e ";
            }
        }
        cout << endl;

        if(run_manip && !justsim){
            // Look at each manipulation in the given manip_file
            // Run and compute upper bound on margin.
            Manipulations mps;
            cout << "Loading manipulations" << endl;
            if(!LoadManipulation(manip_file, config, candidates, mps)){
                cout << "Error loading manipulations." << endl;
                return 1;
            }
            cout << "Finished loading manipulations" << endl;

            int best_manipulation = -1;
            Manipulation best;

            for(int i = 0; i < mps.size(); ++i){    
                stringstream ss;
                ss << manip_file << "_" << i << ".log";

                Manipulation cm;
                cout << "Running manipulation " << i << endl;
                int actual = RunManipulation(ballots, candidates, config, 
                    elected_list, mps[i], ss.str().c_str(), order_c, order_a,
                    cm, apply_half, apply_frac, ac_format, ac_seats, tprefix);
                cout << "Completed running manipulation " << i << " with "
                    << "actual required vote changes = " << actual << endl;
                if(actual == -1) 
                    continue;

                if(best_manipulation == -1 || actual < best_manipulation){
                    best_manipulation = actual;
                    best = cm;
                }   
            }

            cout << "Best Manipulation = " << 
                best_manipulation << " votes." << endl;

            bool verify = TestManipulation(candidates, best,
                ac_format, ac_seats, tprefix);
            cout << "    Verify? " << verify << endl;

            // Print best manipulation to file
            int total_added = 0;
            int total_deleted = 0;
            if(final_manip != NULL){
                ofstream mpstream(final_manip);
                mpstream << "Add Ballots" << endl;
                for(int j = 0; j < best.toadd.size(); ++j){
                    const Ints &pfs = get<0>(best.toadd[j]);
                    int n = get<1>(best.toadd[j]);
                    PrintBallot(mpstream, pfs, candidates);
                    mpstream << " : " << n << endl; 
                    total_added += n;
                }
                mpstream << "Remove Ballots" << endl;
                for(int j = 0; j < best.todel.size(); ++j){
                    const Ints &pfs = get<0>(best.todel[j]);
                    int n = get<1>(best.todel[j]);
                    PrintBallot(mpstream, pfs, candidates);
                    mpstream << " : " << n << endl;
                    total_deleted += n; 
                }
                mpstream << "Total Added," << total_added << endl;
                mpstream << "Total Removed," << total_deleted << endl;
                cout <<"    (total added " << total_added << ", removed "
                    << total_deleted << ")" << endl;
                mpstream.close();
            }

            return 0;
        }

        if(config.gub != -1){
            cout << "Provided upper bound = " << config.gub << endl;
            upperbound = min(upperbound, config.gub);
        }

        Manipulation temp;
        cout << "Computing Greedy Manipulation" << endl;
        double weub = WEUB(ballots,votecounts, candidates, config,
            elected_list, order_c, order_a, false, cout, temp, ballots.size(),
            ac_format, ac_seats, tprefix);

        cout << "Greedy manipulation size = " << weub << endl;
        if(weub >= 0){
            upperbound = min(weub, upperbound);
        }
        if(justsim){
            if(manip_file != NULL){
                // Record manipulation
                ofstream mpstream(manip_file);
                mpstream << "Add Ballots" << endl;
                for(int j = 0; j < temp.toadd.size(); ++j){
                    const Ints &pfs = get<0>(temp.toadd[j]);
                    int n = get<1>(temp.toadd[j]);
                    PrintBallot(mpstream, pfs, candidates);
                    mpstream << " : " << n << endl; 
                }
                mpstream << "Remove Ballots" << endl;
                for(int j = 0; j < temp.todel.size(); ++j){
                    const Ints &pfs = get<0>(temp.todel[j]);
                    int n = get<1>(temp.todel[j]);
                    PrintBallot(mpstream, pfs, candidates);
                    mpstream << " : " << n << endl; 
                }
                mpstream.close();
            }
 
            return 0;
        }
  
 
        cout << "Upper bound used = " << upperbound << endl;
        cout << "STARTING TREE SEARCH" << endl; 

        Result r = RunTreeSTV(ballots, candidates, config,
            order_c, order_a, upperbound, timelimit, 
            compbounds, logf);

        cout << "Number of manipulations found = " << r.mps.size() << endl;
        cout << "Lower bound on margin = " << r.curr_lbound << endl;
        cout << "Additional Info: " << r.err_message << endl;

        if(r.mps.size() > 0 && manip_file != NULL){
            // Sort manipulations in order of required changes
            sort(r.mps.begin(), r.mps.end(), CompareManipulations);

            cout << "Recording Manipulations " << endl;
            ofstream mpstream(manip_file);
            for(int i = 0; i < r.mps.size(); ++i){
                const Manipulation &mp = r.mps[i];
                mpstream << "Manipulation " << i << endl;
                for(int j = 0; j < mp.order_c.size(); ++j)
                {
                    int cid = candidates[mp.order_c[j]].id;
                    if(mp.order_a[j] == 0){
                        mpstream << cid << "-e ";
                    }
                    else{
                        mpstream << cid << "-q ";
                    }
                }
                mpstream << endl;
                mpstream << "Add Ballots" << endl;
                for(int j = 0; j < mp.toadd.size(); ++j){
                    const Ints &pfs = get<0>(mp.toadd[j]);
                    int n = get<1>(mp.toadd[j]);
                    PrintBallot(mpstream, pfs, candidates);
                    mpstream << " : " << n << endl; 
                }
                mpstream << "Remove Ballots" << endl;
                for(int j = 0; j < mp.todel.size(); ++j){
                    const Ints &pfs = get<0>(mp.todel[j]);
                    int n = get<1>(mp.todel[j]);
                    PrintBallot(mpstream, pfs, candidates);
                    mpstream << " : " << n << endl; 
                }
                mpstream << "End of Manipulation " << i << endl;
            }
            mpstream.close();
        }    

        mytimespec tend;
        GetTime(&tend);
        cout << "Total time: " << tend.seconds - start.seconds << endl;
    }
    catch(exception &e)
    {
        cout << e.what() << endl;
        cout << "Exiting." << endl;
        return 1;
    }
    catch(STVException &e)
    {
        cout << e.what() << endl;
        cout << "Exiting." << endl;
        return 1;
    }   
    catch(...)
    {
        cout << "Unexpected error. Exiting." << endl;
        return 1;
    }

    return 0;
}




