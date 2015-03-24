#!/bin/bash
# alltests.sh                                            2013-07-05 Agner Fog
# (c) Copyright 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses

# initalization
source init.sh

# measure latencies and throughputs of all instructions
./allsh1.sh

# other microarchitecture tests
./allsh2.sh

# pack all results into zipfile
./pack_results.sh
