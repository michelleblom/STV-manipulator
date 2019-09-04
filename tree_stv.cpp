#include<set>
#include<vector>
#include<list>
#include<iostream>
#include<fstream>
#include<cmath>
#include<boost/thread/thread.hpp>
#include<sstream>

#include "tree_stv.h"
#include "stv_distance.h"

using namespace std;

typedef list<Node> Fringe;
typedef vector<Node> Nodes;
typedef vector<Fringe> Fringes;

void InsertIntoFringe(const Node &n, Fringe &fringe){
    Fringe::iterator it = fringe.begin();
    for( ; it != fringe.end(); ++it){
        if(n.dist < it->dist ||(n.dist <= it->dist && 
            n.dist1 < it->dist1)){
            break;
        }
    }
    
    fringe.insert(it, n);
}

Node& PopNode(Fringe &fringe){
    return *(fringe.begin());
}

void PrintNode(const Node &n, ostream &log){
    for(int i = 0; i < n.order_c.size(); ++i){
        log << n.order_c[i] << "--";
        if(n.order_a[i]){
            log << "q ";
        }
        else {
            log << "e ";
        }
    }

    log << " with distance " << n.dist << " ";
}

void PrintFringe(const Fringe &fringe, ostream &log){
    log << "--------------------------------" << endl;
    log << "Current state of priority queue: " << endl;
    log << "    Node ";
    PrintNode(fringe.front(), log);
    log << endl;
    log << "    .... " << endl;
    log << "    Node ";
    PrintNode(fringe.back(), log);
    log << endl;
    
/*  for(Fringe::const_iterator it = fringe.begin();
        it != fringe.end(); ++it){
        log << "    Node ";
        PrintNode(*it, log);
        log << endl;    
    }*/
    log << "--------------------------------" << endl;
}

bool IgnoreOrder(const set<int> &elected, const Config &config){
    if(!config.elect_only.empty()){
        Ints temp(config.ncandidates);
        set<int>::iterator it = set_intersection(newn.elected.begin(),
            newn.elected.end(), config.elect_only.begin(), 
            config.elect_only.end(), temp.begin());

        temp.resize(it-temp.begin());
        if(temp.empty())
            return true;
    }
    return false;
}
 

void GetChildren(const Node &n, Nodes &children, const set<int> &elected_o,
    const Config &config)
{
    if(n.seatsleft == n.remcand.size()){
        Node newn(n.cand_proc.size(), n.bid2newid.size());
        newn.order_c = n.order_c;
        newn.order_a = n.order_a;
        newn.elected = n.elected;

        for(int i = 0; i < n.seatsleft; ++i){
            newn.order_c.push_back(n.remcand[i]);
            newn.order_a.push_back(1);
            newn.elected.insert(n.remcand[i]);
        }

        newn.seatsleft = 0;
        newn.dist = n.dist; 
        newn.dist1 = n.dist1; 

        if(!IgnoreOrder(newn.elected,config) && 
            newn.elected != elected_o){
            children.push_back(newn);
        }

        return;
    }

    if(n.seatsleft == 1 && n.remcand.size() == 2){
        const int fc = n.remcand[0];
        const int sc = n.remcand[1];

        for(int o = 0; o <= 1; ++o){
            Node newn(n.cand_proc.size(), n.bid2newid.size());
            newn.order_c = n.order_c;
            newn.order_a = n.order_a;
            newn.elected = n.elected;
            newn.seatsleft = 0;
            newn.dist = n.dist;
            newn.dist1 = n.dist1;

            newn.order_c.push_back(fc);
            newn.order_a.push_back(o);

            newn.order_c.push_back(sc);
            newn.order_a.push_back(1-o);    
            
            if(o == 1){
                newn.elected.insert(fc);
            }
            else{
                newn.elected.insert(sc);
            }

            if(!IgnoreOrder(newn.elected,config) && 
                newn.elected != elected_o){
                children.push_back(newn);           
            }
        }

        for(int o = 0; o <= 1; ++o){
            Node newn(n.cand_proc.size(), n.bid2newid.size());
            newn.order_c = n.order_c;
            newn.order_a = n.order_a;
            newn.elected = n.elected;
            newn.seatsleft = 0;
            newn.dist = n.dist;
            newn.dist1 = n.dist1;

            newn.order_c.push_back(sc);
            newn.order_a.push_back(o);

            newn.order_c.push_back(fc);
            newn.order_a.push_back(1-o);    
            
            if(o == 1){
                newn.elected.insert(sc);
            }
            else{
                newn.elected.insert(fc);
            }

            if(!IgnoreOrder(newn.elected,config) && 
                newn.elected != elected_o){
                children.push_back(newn);           
            }
        }

    }
    else if(n.remcand.size() == 2 && n.seatsleft == 0){
        const int fc = n.remcand[0];  
        const int sc = n.remcand[1];

        Node newn(n.cand_proc.size(), n.bid2newid.size());
        newn.order_c = n.order_c;
        newn.order_a = n.order_a;
        newn.elected = n.elected;
        newn.seatsleft = n.seatsleft;
        newn.dist = n.dist;
        newn.dist1 = n.dist1;

        newn.order_c.push_back(fc);
        newn.order_a.push_back(0);
        newn.order_c.push_back(sc);
        newn.order_a.push_back(0); 

        if(!IgnoreOrder(newn.elected,config)){ 
            children.push_back(newn);       

            newn.order_c[newn.order_c.size()-2] = sc;   
            newn.order_c[newn.order_c.size()-1] = fc;   
            children.push_back(newn); 
        }      
    }
    else{
        for(int j = 0; j < n.remcand.size(); ++j){
            const int cand = n.remcand[j];

            // candidate can be elected or eliminated
            for(int o = 0; o <= 1; ++o){
                Node newn(n.cand_proc.size(), n.bid2newid.size());
                newn.order_c = n.order_c;
                newn.order_a = n.order_a;
                newn.elected = n.elected;
                newn.seatsleft = n.seatsleft;
                newn.dist = n.dist;
                newn.dist1 = n.dist1;

                for(int k = 0; k < n.remcand.size(); ++k){
                    if(j != k){
                        newn.remcand.push_back(n.remcand[k]);
                    }
                }

                newn.order_c.push_back(cand);
                newn.order_a.push_back(o);  
            
                if(o == 1){
                    newn.elected.insert(cand);
                    --newn.seatsleft;

                    if(newn.seatsleft == 0){
                        for(int k = 0; k < newn.remcand.size(); ++k){
                            newn.order_c.push_back(newn.remcand[k]);
                            newn.order_a.push_back(0);
                        }
                        newn.remcand.clear();
                    }
                }

                if(!(newn.seatsleft == 0 && (newn.elected == elected_o ||
                    IgnoreOrder(newn.elected,config))){
                    children.push_back(newn);
                }
            }
        }
    }
}

void PruneFringe(Fringe &fringe, bool foundmp, double ubound, ostream &log){
    for(Fringe::iterator it = fringe.begin(); it != fringe.end(); ){
        if(it->dist > ubound || (it->dist == ubound && foundmp)){
            log << "Pruning node ";
            PrintNode(*it, log);
            log << endl;

            if(fringe.size() == 1){
                fringe.clear();
                return;
            }

            fringe.erase(it++);
        }
        else{
            ++it;       
        }
    }
}

struct SubThreadInputData{
    set<int> elected_o;
    double upperbound;
    double timelimit;
    Ints order_c;
    Ints order_a;
};


void SolveDistanceTo(Node &n, const Ballots &ballots,
	const Candidates &cands, const Config &config, 
	const SubThreadInputData &sidata,
	const string &logf, double &dtvalue, double &defub,
    Result &result)
{
	const Ints &order_c = sidata.order_c;
	const Ints &order_a = sidata.order_a;

	ofstream log;
	log.open(logf.c_str(), ofstream::app);

	log << "Solving DT for ";
	PrintNode(n, log);
	log << endl;

	dtvalue = distance(ballots, cands, config,
		n, sidata.upperbound,  sidata.timelimit, 
		false, sidata.elected_o, log, order_c, order_a, defub, result);
	
	log << "Solved DT for ";
	PrintNode(n, log);
	log << " value " << dtvalue << endl;

	log.close();
}




Result RunTreeSTV(const Ballots &ballots, const Candidates &cands,
    const Config &config, const Ints &order_c, const Ints &order_a, 
    double upperbound, double timelimit, bool compbounds, const char *logf)
{
    try{
        mytimespec start;
        GetTime(&start);

        Fringe fringe;

        Result result;

        int dtcounter = 0;
    
        set<int> elected_o;
        for(int j = 0; j < order_c.size(); ++j){
            if(order_a[j]){
                elected_o.insert(order_c[j]);
            }
        }

        ofstream log(logf);

        bool foundmp = false;

        double curr_ubound = upperbound;
        if(config.FIX_UNTIL > 0){
            Node newn(config.ncandidates, ballots.size());
            newn.seatsleft = config.nseats;
            Ints temp_order_c(config.FIX_UNTIL);
            Ints temp_order_a(config.FIX_UNTIL);
            Ints remcand;

            newn.dist = 0;
            newn.dist1 = 0;
            Preliminaries(ballots, cands, config, newn,
                compbounds, elected_o, log);

            for(int i = 0; i < config.FIX_UNTIL; ++i){
                temp_order_c[i] = order_c[i];   
                temp_order_a[i] = order_a[i];
                if(order_a[i] == 1){
                    --newn.seatsleft;
                    newn.elected.insert(order_c[i]);
                }   
            }

            for(int j = config.FIX_UNTIL; j < config.ncandidates; ++j){
                remcand.push_back(order_c[j]);
            }

            newn.order_c = temp_order_c;
            newn.order_a = temp_order_a;
            newn.remcand = remcand;

            InsertIntoFringe(newn, fringe);
        }
        else{
            // BUILD FRINGE
            for(int i = 0; i < config.ncandidates; ++i){
                // candidate can either be elected or eliminated
                Ints temp_order_c(1);
                temp_order_c[0] = i;
                Ints remcand;
                for(int j = 0; j < config.ncandidates; ++j){
                    if(i != j) remcand.push_back(j);    
                }

                for(int o = 0; o <= 1; ++o){
                    Ints temp_order_a(1);
                    temp_order_a[0] = o;

                    mytimespec tnow;
                    GetTime(&tnow);
                    double tleft = timelimit - (tnow.seconds - start.seconds);
                    if(tleft < 10){
                        log << "RunTreeSTV timed out" << endl;
                        log.close();
                        result.err_message = "RunTreeSTV timed out";
                        return result;
                    }

                    Node newn(config.ncandidates, ballots.size());
                    newn.order_c = temp_order_c;
                    newn.order_a = temp_order_a;
                    newn.remcand = remcand;
                    if(o == 1) newn.elected.insert(i);
                    newn.seatsleft = config.nseats - o;
            
                    Preliminaries(ballots, cands, config, newn,
                        compbounds, elected_o, log);

                    if(newn.dist > curr_ubound) continue;
                
                    double dfub = curr_ubound;
                    newn.dist1 = distance(ballots,  cands, config,
                        newn, upperbound, tleft, false, 
                        elected_o, log, order_c, order_a, dfub, result);
                    foundmp = !result.mps.empty();

                    if(newn.remcand.empty()){
                        curr_ubound = min(curr_ubound, dfub);
                    }

                    ++dtcounter;
    
                    if(newn.dist1 >= 0) 
                        newn.dist = max(newn.dist1, newn.dist);
                    else
                        newn.dist = newn.dist1;

                    if(newn.dist >= 0 && (newn.dist < curr_ubound ||
                        (newn.dist == curr_ubound && !foundmp))){
                        InsertIntoFringe(newn, fringe);
                    }
                }    
            }
        }

        Ints best_order_c;
        Ints best_order_a;

        double lowerbound = 0;
        while(fringe.size() > 0){
            foundmp = !result.mps.empty();
            PrintFringe(fringe, log);

            log << "CURRENT UPPER BOUND = " << curr_ubound << endl;
            bool all_leaves = true; 
            double blower = curr_ubound;
            for(Fringe::const_iterator it = fringe.begin();
                it != fringe.end(); ++it){
                blower = min(blower, it->dist);
                if(!it->remcand.empty()){
                    all_leaves = false;
                }
            }

            log << "BEST LOWER BOUND = " << blower << endl;
            lowerbound = blower;

            result.curr_ubound = curr_ubound;
            result.curr_lbound = lowerbound;

            mytimespec tnow;
            GetTime(&tnow);
            log << "TOTAL TIME USED SO FAR: " << tnow.seconds -
                start.seconds << endl;

            log << "DistanceTo's solved: " << dtcounter << endl;

            if(all_leaves){
                log << "Fringe contains only leaves we couldn't handle" << endl;
                log << "LB " << blower << " UB " << curr_ubound << endl;
                result.err_message =  "Fringe contains only leaves we couldn't handle"; 
                break;
            }

            if(ceil(blower) == ceil(curr_ubound) && foundmp){
                log << "Lower and Upper bound are close enough." << endl;
                log << "LB " << blower << " UB " << curr_ubound <<endl;
                break;
            }

            double tleft = timelimit - (tnow.seconds - start.seconds);
            if(tleft < 10){
                log << "RunTreeSTV timed out" << endl;
                log.close();
                result.err_message = "RunTreeSTV timed out";
                return result;
            }

            // Expand first node in fringe: Get/evaluate children 
            Node node = *(fringe.begin());
            fringe.erase(fringe.begin());

            log << "Expanding node: ";
            PrintNode(node, log);
            log << endl; 

            Nodes children;
            GetChildren(node, children, elected_o);

            Nodes toadd;

	        int ca = 0;
	        int cprocessed = 0;
	        while(cprocessed < children.size()){
		        boost::thread **tasks = new boost::thread*[config.subthreads];
		        Doubles dtvalues(config.subthreads, -1);
		        Doubles dfvalues(config.subthreads, curr_ubound);
                Results tresults(config.subthreads);
		        Ints corr_child(config.subthreads, -1);
		        vector<string> logfilenames(config.subthreads, "");

		        SubThreadInputData sidata;
		        sidata.elected_o = elected_o;
		        sidata.timelimit = tleft;
		        sidata.upperbound = curr_ubound;
		        sidata.order_c = order_c;
		        sidata.order_a = order_a;
                
		        int tcntr = 0;
		        for(int j = cprocessed; j < children.size(); ++j){
                    tresults[tcntr] = result;
                    tresults[tcntr].mps.clear();
			        Node &n = children[j];

			        Preliminaries(ballots, cands, config, n,
				        compbounds, elected_o, log);

			        if(n.dist >= 0 && n.dist > curr_ubound){
				        ++cprocessed;
				        continue;
			        }

			        stringstream ss;
			        ss << tcntr << "_" << logf;
			
			        logfilenames[tcntr] = ss.str();
			        tasks[tcntr] = new boost::thread(
				        SolveDistanceTo,
				        boost::ref(n),
				        boost::ref(ballots),
				        boost::ref(cands),
				        boost::ref(config),
				        boost::ref(sidata),
				        boost::ref(logfilenames[tcntr]),
				        boost::ref(dtvalues[tcntr]),
				        boost::ref(dfvalues[tcntr]),
                        boost::ref(tresults[tcntr])
			        );

			        corr_child[tcntr] = j;
			        ++tcntr;
			        ++cprocessed;

			        if(tcntr == config.subthreads)
				        break;
		        }

	        	for(int j = 0; j < tcntr; ++j)
			        tasks[j]->join();

	        	for(int j = 0; j < tcntr; ++j){
        			Node &n = children[corr_child[j]];
		        	++dtcounter;

			        n.dist1 = dtvalues[j];
			        if(n.dist1 >= 0) 
				        n.dist = max(n.dist1, n.dist);
			        else
				        n.dist = n.dist1;

                    PrintNode(n, log);
                    log << endl;

                    // Add candidate manipulations
                    const Result &tres = tresults[j];
                    if(tres.mps.size() > 0){
                        for(int k = 0; k < tres.mps.size(); ++k)
                            result.mps.push_back(tres.mps[k]);
                    }

                    foundmp = !result.mps.empty();

                    if(n.remcand.empty()){
                        curr_ubound = min(curr_ubound, dfvalues[j]);
                    }

                    if(n.dist >= 0 && (n.dist < curr_ubound ||
                        (n.dist == curr_ubound && !foundmp))){
                        toadd.push_back(n);
                        ++ca;
                    }

                    if(n.dist >= 0 && n.dist <= curr_ubound &&
                        n.remcand.empty()){
                        log << "Leaf node ";
                        PrintNode(n, log);
                        log << " found " << endl;
                        log << "Current upper bound: "<<curr_ubound << endl;

                        best_order_c = n.order_c; 
                        best_order_a = n.order_a;
                    }
 		        }

		        delete[] tasks;
	        }
		 
            if(ca == 0){
                log << "No children, node pruned" << endl;  
            }
            else{
                for(Nodes::iterator it = toadd.begin();
                    it != toadd.end(); ++it){

                    if(it->dist >= 0 && (it->dist < curr_ubound-0.01 ||
                        (it->dist == curr_ubound && !foundmp))){ 
                        log << "Adding node to fringe: ";
                        PrintNode(*it, log);
                        log << endl;

                        InsertIntoFringe(*it, fringe);
                    }
                }
            }

            // Remove all nodes from queue with lower bound 
            // greater than or equal to upperbound
            PruneFringe(fringe, foundmp, curr_ubound, log);
            log << "Size of fringe: " << fringe.size() << endl;
        }

        lowerbound = curr_ubound;
        for(Fringe::const_iterator it = fringe.begin();
            it != fringe.end(); ++it){
            lowerbound = min(lowerbound, it->dist);
        }

        mytimespec tnow;
        GetTime(&tnow);
        log << "TOTAL TIME USED SO FAR: " << tnow.seconds -
            start.seconds << endl;

        if(!best_order_c.empty()){
            log << "====================================" <<endl;
            log << "Minimal manipulation: " << curr_ubound << endl;
            log << "Manipulated order: ";
            for(int i = 0; i < best_order_c.size(); ++i){
                log << cands[best_order_c[i]].id << "--";
                if(best_order_a[i])
                    log << "q ";
               else
                    log << "e "; 
            }
            log << endl;
        }
        else{
            log << "All nodes pruned " << endl;
        }

       log << "Distance calls: " << dtcounter << endl;
       log << "CURRENT UPPER BOUND: " << curr_ubound << endl;
       log << "CURRENT LOWER BOUND: " << lowerbound << endl;
       result.curr_ubound = curr_ubound;
       result.curr_lbound = lowerbound;

       log << "====================================" <<endl;
       log.close();
       return result;
    }
    catch(exception &e)
    {
        cout << "Exception raised in RunTreeIRV" << endl;
        cout << e.what() << endl;
        Result res;
        res.err_message = "Exception raised in RunTreeIRV";
        return res;
    }
    catch(STVException &e)
    {
        cout << "Exception raised in RunTreeIRV" << endl;
        cout << e.what() << endl;
        Result res;
        res.err_message = "STVException raised in RunTreeIRV";
        return res;
    }   
    catch(...)
    {
        cout << "Exception raised in RunTreeIRV" << endl;
        cout << "Unexpected error." << endl;
        Result res;
        res.err_message = "Exception raised in RunTreeIRV. Unexpected Error";
        return res;
    }
}



