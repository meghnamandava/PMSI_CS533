# PMSI: Predictable MSI cache coherence protocol

This is the gem5 code repository for Predictable MSI (PMSI) presented in our RTAS'17 paper: [Predictable Cache Coherence for Multi-core Real-time Systems] ().

# Getting started
* The Gem5 simulator has been modified to support predictable snoopy bus architecture, and trace based simulation. 
* `$GEM5` refers to the top level directory where gem5 resides. 
* File `$GEM5/build_opts/PMSI` contains information for building the PMSI system, and `$GEM5/configs/ruby/PMSI.py` provides information about the component (caches, network, DRAM) connections in the system. Note that these files do not need to be changed unless the cache hierarchy is extended. 
* To build PMSI, execute `scons $GEM5/build/PMSI/gem5.opt -j8`. The `-j8` is to accelerate the build process.


# Predictable bus arbitration
* We extend Gem5 to support a snoopy bus with a predictable arbiter. The predictable aribiter operates in a time division multiplexed (TDM) manner.
* The `NPROC` and `SLOT_WIDTH` parameters in `src/cpu/testers/rubyTest/Trace.hh` control the number of requestors and the TDM slot width per requestor.

# Memory trace simulation
* Gem5 has been extended to run memory trace based execution.
* The trace based simulation injects traces into the ruby memory model thereby bypassing the core/processor. 
* Trace based simulation can be enabled by setting the `TRACE` flag in `src/cpu/testers/rubytest/Trace.hh`.	
* The trace based simulation reads from a file `trace.trc` that consists of lines of requests of the form `Addr OP time`. OP is of type RD for read and WR for write, and time is a positive integer value that denotes the arrival time of the memory request to the memory hierarchy.
* The trace simulation automatically replicates the memory operations in `trace.trc` across multiple cores (specified by `-n` flag) with the same time-stamp. The issue of the request to the bus is handled by the TDM arbiter. To see a complete trace of the memory requests issued by the cores, enable the protocol trace by using the following command line option: `--debug-flags=ProtocolTrace`.
* A sample trace file can be found in `traces/`.

# Running PMSI
* To run PMSI, the following command line options need to be used for correct functioning: `--ruby, --cpu-type=timing, --topology=Crossbar --cpu-clock=xGHz --ruby-clock=yGHz`.
* Other options such as cache sizes, memory size, cache associativity, and number of cores can be configured accordingly.
* For binary/normal execution, supply the binary name and the associated arguments using the `-c` and `-o` flags respectively. For example, to run FFT of the Splash2 benchmark: `./build/X86_MSI_Snooping_One_Level_RT/gem5.opt --stats-file=FFT-pmsi-stats.out ./configs/example/se.py --ruby -n4 --cpu-type=timing --l1d_size=16kB --l1i_size=16kB --l1d_assoc=1 --l1i_assoc=1  --mem-size=4194304kB --topology=Crossbar --cpu-clock=2GHz --ruby-clock=2GHz --mem-type=SimpleMemory -c $SPLASH/splash2/codes/kernels/fft/fft -o "-m 10 -p 4 -t"`
* For trace based execution, populate the `trace.trc` file with the necessary memory operations and execute: `./build/X86_MSI_Snooping_One_Level_RT/gem5.opt ./configs/example/ruby_random_test.py --ruby-clock=2GHz --ruby --cpu-clock=2GHz --topology=Crossbar --mem-type=SimpleMemory -n 4 --mem-size=4194304kB --wakeup_freq=1`. 
* We provide a script to run the SPLASH2 benchmarks in the `scripts/` directory.

# Contact
* Feel free to contact [us](mailto:amkaushi@uwaterloo.ca) for questions regarding PMSI.
