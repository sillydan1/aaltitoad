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
