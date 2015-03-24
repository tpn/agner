#!/bin/bash
# alltests.sh                                            2014-01-27 Agner Fog
# (c) Copyright 2013-2014 by Agner Fog. GNU General Public License www.gnu.org/licenses


# initalization
./init.sh

echo -e "Running all tests\n`date`\n`./cpugetinfo brand`.\nFamily `./cpugetinfo family hex`, model `./cpugetinfo model hex`"  >> results2/statistics.txt

Starttime=`date +%s`

# measure latencies and throughputs of all instructions
./allsh1.sh

# other microarchitecture tests
./allsh2.sh

Endtime=`date +%s`
Elapsedtime=$(($Endtime - $Starttime))
Minutes=$(($Elapsedtime/60))
Seconds=$(($Elapsedtime-($Minutes*60)))
Numscripts=$( ls *.sh1 *.sh2 | wc -w)

echo Executed $Numscripts scripts. Elapsed time $Minutes m, $Seconds s
echo -e "\nExecuted $Numscripts scripts. Elapsed time $Minutes m, $Seconds s\n\n"  >> results2/statistics.txt

# pack all results into zipfile
./pack_results.sh
