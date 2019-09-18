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


#include<iostream>
#include<iomanip>
#include<fstream>
#include<sstream>
#include<string.h>
#include<stdlib.h>
#include<algorithm>
#include "model.h"
#include "sim_stv.h"

using namespace std;

int main(int argc, const char * argv[]) 
{
	try
	{
		Candidates candidates;
		Ballots ballots; 
		Config config;

		config.quota = -1;

		const char *logf = NULL;

        int sim_until = -1;
        const char *newballot_file = NULL;
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
			else if(strcmp(argv[i], "-logfile")== 0 && i < argc-1){
				logf = argv[i+1];
				++i;
			}
			else if(strcmp(argv[i], "-quota_override")== 0 && i < argc-1){
				config.quota_override = atof(argv[i+1]);
				++i;
			}
			else if(strcmp(argv[i], "-simuntil")== 0 && i < argc-2){
				sim_until = atoi(argv[i+1]);
				newballot_file = argv[i+2];
				i += 2;
			}
		}

		// code to simulate stv
		Ints order_c;
		Ints order_a;
	
		Doubles votecounts(ballots.size(), 0);
		for(int i = 0; i < ballots.size(); ++i){
			votecounts[i] = ballots[i].votes;
		}

        if(sim_until && newballot_file != NULL){
            if(!SimSTVUntil(ballots, candidates, config, 
			    sim_until, newballot_file))
		    {
			    cout << "Simulation Failed. Exiting." << endl;
			    return 1;
            }
            return 0;
		}

        ofstream ofile(logf);
        ofile << "SIMULATION OF ORIGINAL ELECTION" << endl;
        ofile << "Total votes," << setprecision(12) << config.totalvotes << endl;
		if(!SimSTV(ballots, votecounts, candidates, config, 
			order_c, order_a, true, ofile, -1))
		{
			cout << "Initial simulation Failed. Exiting." << endl;
			return 1;
		}

		ofile << "Election/Elimination order: ";
		Ints elected(config.ncandidates, 0);

		int seats = 0;
		set<int> elected_list;	
		for(int i = 0; i < order_c.size(); ++i)
		{
			const int indx = order_c[i];
			ofile << candidates[indx].id << "-";
			if(order_a[i]){
				ofile << "q ";
				++seats;
				elected[indx] =1;
				elected_list.insert(indx);
			}
			else{
				ofile << "e ";
			}
		}

		ofile << endl;
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




