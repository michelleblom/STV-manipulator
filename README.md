# STV-manipulator
An implementation of two heuristics (denoted Greedy and Guided-Greedy) for computing candidate manipulations of Single Transferable Vote (STV) elections. These manipulations provide an upper bound on the margin of victory for such elections, and give us an indication of whether certain known problems arising from ballot mis-management could have influenced the outcome of the election.

STV is a complex preferential voting system for multi-seat elections. It is used widely in Australia at both state and federal levels to elect candidates to the upper houses of Parliament. 

To understand how STV works, refer to the following resources:

https://en.wikipedia.org/wiki/Single_transferable_vote

https://www.electoral-reform.org.uk/voting-systems/types-of-voting-system/single-transferable-vote/

https://www.fairvote.ca/stvbc/


Usage
------

Both of the implemented heuristics rely on an STV simulator that implements the specific model of STV applied in the elections being studied. Our implementation uses the STV simulator written by Andrew Conway, which models the variant of STV used in Australian Federal Senate elections. This simulator can be downloaded from:

https://github.com/SiliconEconometrics/PublicService. 

Follow the instructions provided to compile and run the simulator.

To use our heuristics, download the STV-Manipulator repository and place the PublicService folder (downloaded via Andrew's repository, above) in the STV-Manipulator directory. Compile with the provided Makefile (you may need to modify this with alternate directories for dependences such as CPLEX, and if you are not using Linux). You will need to also run the Makefile in the SimPartial subfolder.

The repository includes two sets of data files for the 2016 and 2019 Australian Federal Senate elections -- one in a standardised format developed by Andrew Conway; and one in a simpler format used by STV-Manipulator. We plan to migrate STV-Manipulator to use the one standardised format at some point in the future. These data files are located in the sub-directories: 2016, 2016/ACFORMAT, 2019, and 2019/ACFORMAT.

The greedy heuristic is run with the following command:

./stv -ballots [STV_manipulator_format] [-do/-ao] -just_sim -ac_format [Andrew's ballot format] [Number of seats] -tprefix [Election identifier]

The -do/-ao flags are specified if you want to find manipulations in the addition-only (manipulations can only add ballots) or deletion-only settings (manipulations can only remove ballots). The heuristics will operate in the shift-only (ballots can only be replaced) setting if -do or -ao are not specified.

For example, consider the 2019 NSW STV election to fill 6 seats in the Australian Federal Senate. To run the greedy heuristic on this election, we use the command:

./stv -ballots 2019/federal_2019_NSW.txt -just_sim -ac_format 2019/ACFORMAT/federal_2019_NSW.stv 6 -tprefix nsw

While running the heuristic, two temporary files will be created (nsw_tmp.manip and nsw_tmp.out) to store manipulations to be applied by Andrew's simulator, and the result of these simulations. The tprefix flag is used to name these temporary files (so that you can run multiple instances of the program in parallel without overwriting these files inappropriately). 

The guided-greedy heuristic involves several phases:
1. Simulate the election of interest down to about 10 candidates, recording the state of the election at this point in a new ballot file. The election tail, with 10 candidates remaining, will be treated as a stand-alone STV election.

2. Taking the ballot data file of the smaller 10 candidate election, we run an existing algorithm (margin-stv) by Blom, Stuckey, and Teague (2019) to both: find a lower bound on the margin of victory of this 10 candidate election; and a set of candidate manipulations for changing the outcome of the 10 candidate election. These manipulations are recorded in a file.

3. The manipulation file generated in Step 2 is then used by STV-manipulator and the guided-greedy heuristic to find a manipulation that changes the outcome of the original election.

Example scripts for running both greedy and guided-greedy are provided in the SCRIPTS folder. 
