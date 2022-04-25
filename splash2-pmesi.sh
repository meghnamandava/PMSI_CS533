#!/bin/bash

# Build X86_MESI_Snooping_One_Level_RT/gem5.opt
# Make sure to have build_opts/X86_MESI_Snooping_One_Level_RT defined
scons ./build/X86_MESI_Snooping_One_Level_RT/gem5.opt -j8

# Set SPLASH2 to point to SPLASH2 benchmark suite
SPLASH2="./gem5-stable/splash2/codes"

./build/X86_MESI_Snooping_One_Level_RT/gem5.opt  --stats-file=pmesi-Cholesky-stats.out ./configs/example/se.py  --ruby -n4 --cpu-type=timing --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1 --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c ${SPLASH2}/kernels/cholesky/cholesky -o "-p4 ${SPLASH2}/kernels/cholesky/inputs/tk14.O" 

./build/X86_MESI_Snooping_One_Level_RT/gem5.opt   --stats-file=pmesi-FFT-stats.out ./configs/example/se.py  --ruby -n4 --cpu-type=timing --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1  --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c ${SPLASH2}/kernels/fft/fft -o "-m 10 -p 4 -t" 

#./build/X86_MESI_Snooping_One_Level_RT/gem5.opt  --stats-file=pmesi-FMM-stats.out ./configs/example/se.py  --ruby -n4 --cpu-type=timing --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1  --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c ${SPLASH2}/apps/fmm/FMM 

./build/X86_MESI_Snooping_One_Level_RT/gem5.opt  --stats-file=pmesi-LU-cont-stats.out ./configs/example/se.py  --ruby -n4 --cpu-type=timing  --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1 --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c ${SPLASH2}/kernels/lu/contiguous_blocks/lu -o "-n128 -p4" 

./build/X86_MESI_Snooping_One_Level_RT/gem5.opt  --stats-file=pmesi-Ocean-stats.out ./configs/example/se.py  --ruby -n4 --cpu-type=timing --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1  --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c ${SPLASH2}/apps/ocean/contiguous_partitions/OCEAN -o "-n18 -p4" 

./build/X86_MESI_Snooping_One_Level_RT/gem5.opt  --stats-file=pmesi-Radiosity-stats.out ./configs/example/se.py  --ruby -n4 --cpu-type=timing --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1  --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c ${SPLASH2}/apps/radiosity/RADIOSITY -o "-p 4 -batch" 

./build/X86_MESI_Snooping_One_Level_RT/gem5.opt  --stats-file=pmesi-Radix-stats.out ./configs/example/se.py  --ruby -n4 --cpu-type=timing --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1  --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c ${SPLASH2}/kernels/radix/RADIX -o "-p4 -n16384" 

./build/X86_MESI_Snooping_One_Level_RT/gem5.opt --stats-file=pmesi-Raytrace-stats.out ./configs/example/se.py  -I 100000000 --ruby -n4 --cpu-type=timing --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1  --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c ${SPLASH2}/apps/raytrace/RAYTRACE -o "-p4 ${SPLASH2}/apps/raytrace/teapot.env"

./build/X86_MESI_Snooping_One_Level_RT/gem5.opt  --stats-file=pmesi-Barnes-stats.out ./configs/example/se.py  --ruby -n4 --cpu-type=timing --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1  --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c ${SPLASH2}/apps/barnes/BARNES
