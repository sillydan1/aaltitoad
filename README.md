# AALTITOAD

![logo](AALTITOAD_LOGO_SMALLER.png)

(**Aal**borg **Ti**ck T**o**ck **A**utomata Vali**d**ator) is a verification engine and simulator for Tick Tock Automata. 

TTA's (Tick Tock Automatas) are an Automata based theory. [This](https://github.com/sillydan1/AALTITOAD/blob/master/SW10__Tick_Tock_Automata.pdf) paper describes the Tick Tock Automata theory, and [this](https://github.com/sillydan1/AALTITOAD/blob/master/SW9__AALTITOAD.pdf) paper describes the tool and presents test case results.

The TTA parser assumes models modelled in the [H-UPPAAL](https://github.com/DEIS-Tools/H-Uppaal) tool.  

## Compile (Linux)
First, make sure that you have cloned the repo with git submodules initialized.
```
git clone --recursive https://github.com/sillydan1/AALTITOAD
```
If the repository was cloned non-recursively previously, use `git submodule update --init` to initialize the necessary submodules.

Then you should be able to build the tool like so:
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
If you want to compile release version, simply do:
```
make RELEASE=1
```

## Install
To install the `aaltitoad` commandline tool and the associated libraries to `/usr/local/bin/` do the following:
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make install
```

If you want to uninstall again, run `cat install_manifest.txt | xargs sudo rm`.

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
 -h, --help                     | Print this message
```

## Contributing
If you want to contribute to the project either raise an issue, fork the repo and register a pull request or reach out to [me](https://github.com/sillydan1) directly.
