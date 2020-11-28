# AALTITOAD

![logo](AALTITOAD_LOGO_SMALLER.png)

(**Aal**borg **Ti**ck T**o**ck **A**utomata Vali**d**ator) is a verification engine and simulator for Tick Tock Automata. 

TTA's (Tick Tock Automatas) are an Automata based theory. [This](https://projekter.aau.dk/projekter/da/studentthesis/tick-tock-automata-a-modelling-formalism-for-real-world-industrial-systems(8cb83e04-9b9a-4261-b457-1d09d85e593e).html) paper describes the theory behind it all, but the verification itself is not described there. (paper for that is WIP)

The TTA parser assumes models modelled in the [H-UPPAAL](https://github.com/DEIS-Tools/H-Uppaal) tool.  

## Compile
First, make sure that you have initialized all git submodules.
```
mkdir build && cd build
cmake ..
make
```
If you want to compile release version, simply do:
```
make RELEASE=1
```

## Install
This will install the `aaltitoad` commandline tool to `/usr/local/bin/` or `REDACTED` on windows/macOS  
```
mkdir build && cd build
cmake ..
make install
```

## How to use
```
USAGE: aaltitoad -i /path/to/project/dir/ [OPTIONS]

OPTIONS:
 -i, --input	                | [DIR] Input directory
 -o, --output	                | [DIR]/[FILENAME] Output file. Will be created, if not already exists
 -q, --query	                | [DIR]/[FILENAME] File with queries to be verified. This flag is required for verification
 -v, --verbosity                | [0-6] The level of verbosity. Default is 2
 -n, --nondeterminism-strategy	| [0-3] Determines which edge to pick when encountering nondeterministic choice. Default is 0 (0: PANIC | 1: PICK_FIRST | 2: PICK_LAST | 3: PICK_RANDOM)
 -d, --parser-disable-check     | Disables proper formation checks.
 -t, --trace	                | [1-N] Outputs a trace of the input automata of provided amount of steps. Use with '--trace-output' option
 -u, --trace-output             | [DIR]/[FILENAME] Output file for traces. Use together with '--trace' option
 -f, --timing-info              | [DIR]/[FILENAME] Input file for timing information/instructions. Use together with '--trace' option
 -z, --ignore-update-influence	| Forces transitions to be taken, no matter if they have an overlapping update influence. NOTE: Does not disable the warning
```
