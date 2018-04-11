#!/bin/bash
#                                                       2016-10-01 Agner Fog
# disasm.sh
# Script file for disassembling test code in order to verify it
# (c) Copyright 2016 by Agner Fog. GNU General Public License www.gnu.org/licenses
#
# Requires objconv program executable file to be available in same directory
# (compile objconv in separate directory with g++ -O2 -o objconv *.cpp)
#################################################################################################################

./objconv -fnasm b64.o
cat b64.asm
