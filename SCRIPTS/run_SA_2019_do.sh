echo "SA 2019 Delete Only Analysis"

# Run the greedy heuristic to determine how many ballot removals are required
# to alter the election outcome.
./stv -ballots "2019/federal_2019_SA.txt" -do -just_sim -ac_format "2019/ACFORMAT/federal_2019_SA.stv" 6 -tprefix sa

# To run the guided-greedy heuristic, we run our partial STV simulator to 
# generate an election representing the tail of the election
./SimPartial/stv -ballots "2019/federal_2019_SA.txt" -logfile sa_tmp.log -simuntil 35 federal_2019_SA_simUntil35.txt

# Now we need to compute some candidate manipulations for our election tail.
# 
# The 'bilin' flag specifies how much relaxation we want to apply to the 
# problem of finding a lower bound on the election MOV (small values = more
# relaxation, larger values = less). If unsure of what value to use, leave 
# it at 10 and increase is a candidate manipulation of size 0 is found. 
# If the latter case arises, it is because we have 'over-relaxed' the problem 
# and it thinks we can alter the outcome without actually changing any ballots! 
#
# The 'sthreads' flag indicates how many parallel threads of computation
# we want to allow the manipulation-finder to use. 
#
# The MOV lower bound finder applies branch-and-bound to search through
# a tree of possible alternate election outcomes for one requiring the
# smallest number of ballot changes to realise. The -tlimit_leaf, 
# -tlimit_wsol, and -tlimit_wosol apply time limits (in seconds) on the 
# MIP solves of leaf nodes, of nodes for which a solution has been found, 
# and for nodes for which no solution has been found.
#
# The 'oseats' and 'ovotes' flags indicate the number of seats and 
# formal votes in the original (non-shortend) election.
 
./stv -ballots "federal_2019_SA_simUntil35.txt" -do -bilin 10 -tlimit_leaf 5000 -tlimit_wsol 1000 -tlimit_wosol 2000 -logfile DO_SA_2019_simUntil35_manip.log -sthreads 5 -manipfile DO_SA_2019_manipulation_simUntil35.txt -oseats 6 -ovotes 1094823 

# Now that we have some candidate manipulations, we are going to 
# use guided-greedy to apply some percentage of them to our election,
# and then use greedy to make some additional changes. We look at
# each manipulation found in the above step and find the one 
# requiring the least number of ballot removals to change the 
# election outcome.

echo "Apply 100% of candidate manipulations"
./stv -ballots "2019/federal_2019_SA.txt" -do -ignore_first 35 -logfile tmp.log -run_manip -manipfile DO_SA_2019_manipulation_simUntil35.txt -finalmanip DO_SA_2019_best_manip_simUntil35.txt -ac_format "2019/ACFORMAT/federal_2019_SA.stv" 6 -tprefix sa

rm DO_SA_2019_manipulation_simUntil35.txt_*.log

echo "Apply 50% of candidate manipulations"
./stv -ballots "2019/federal_2019_SA.txt" -do -ignore_first 35 -logfile tmp.log -run_manip -manipfile DO_SA_2019_manipulation_simUntil35.txt -finalmanip DO_SA_2019_best_manip_simUntil35_apply50pc.txt -ac_format "2019/ACFORMAT/federal_2019_SA.stv" 6 -tprefix sa -applyhalf 0.5

rm DO_SA_2019_manipulation_simUntil35.txt_*.log

echo "Apply 25% of candidate manipulations"
./stv -ballots "2019/federal_2019_SA.txt" -do -ignore_first 35 -logfile tmp.log -run_manip -manipfile DO_SA_2019_manipulation_simUntil35.txt -finalmanip DO_SA_2019_best_manip_simUntil35_apply25pc.txt -ac_format "2019/ACFORMAT/federal_2019_SA.stv" 6 -tprefix sa -applyhalf 0.25

rm DO_SA_2019_manipulation_simUntil35.txt_*.log


echo "Apply 15% of candidate manipulations"
./stv -ballots "2019/federal_2019_SA.txt" -do -ignore_first 35 -logfile tmp.log -run_manip -manipfile DO_SA_2019_manipulation_simUntil35.txt -finalmanip DO_SA_2019_best_manip_simUntil35_apply15pc.txt -ac_format "2019/ACFORMAT/federal_2019_SA.stv" 6 -tprefix sa -applyhalf 0.15

rm DO_SA_2019_manipulation_simUntil35.txt_*.log

echo "Apply 5% of candidate manipulations"
./stv -ballots "2019/federal_2019_SA.txt" -do -ignore_first 35 -logfile tmp.log -run_manip -manipfile DO_SA_2019_manipulation_simUntil35.txt -finalmanip DO_SA_2019_best_manip_simUntil35_apply5pc.txt -ac_format "2019/ACFORMAT/federal_2019_SA.stv" 6 -tprefix sa -applyhalf 0.05

rm DO_SA_2019_manipulation_simUntil35.txt_*.log

echo "Apply 1% of candidate manipulations"
./stv -ballots "2019/federal_2019_SA.txt" -do -ignore_first 35 -logfile tmp.log -run_manip -manipfile DO_SA_2019_manipulation_simUntil35.txt -finalmanip DO_SA_2019_best_manip_simUntil35_apply1pc.txt -ac_format "2019/ACFORMAT/federal_2019_SA.stv" 6 -tprefix sa -applyhalf 0.01

rm DO_SA_2019_manipulation_simUntil35.txt_*.log


echo "Apply 0.5% of candidate manipulations"
./stv -ballots "2019/federal_2019_SA.txt" -do -ignore_first 35 -logfile tmp.log -run_manip -manipfile DO_SA_2019_manipulation_simUntil35.txt -finalmanip DO_SA_2019_best_manip_simUntil35_apply0.5pc.txt -ac_format "2019/ACFORMAT/federal_2019_SA.stv" 6 -tprefix sa -applyhalf 0.005

rm DO_SA_2019_manipulation_simUntil35.txt_*.log











