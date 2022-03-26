# AALTITOAD

![logo](.github/resources/AALTITOAD_LOGO_SMALLER.png)

**Aal**borg **Ti**ck T**o**ck **A**utomata Vali**d**ator is a verification engine and simulator for Tick Tock Automata. 

TTA's (Tick Tock Automatas) are an Automata based theory. [This](.github/resources/SW10__Tick_Tock_Automata.pdf) paper describes the Tick Tock Automata theory, and [this](.github/resources/SW9__AALTITOAD.pdf) paper describes the tool and presents test case results.

The TTA parser assumes models modelled in the [H-UPPAAL](https://github.com/DEIS-Tools/H-Uppaal) tool.  

## Compile (Linux)
Aaltitoad is built using cmake. You must have a C++20 compatible compiler, `flex`, `bison` version 3.5+ and the standard template library installed.
All other dependencies are handled through the wonderful [CPM](https://github.com/cpm-cmake/CPM.cmake) package manager.
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
If the CPM step is taking a long time, try rerunning with `-DCPM_SOURCE_CACHE=~/.cache/CPM`

## Tocker plugins
Aaltitoad supports third party "tocker" libraries, so you can inject custom integrations directly into the runtime.


If you have any questions, you are welcome to reach out to [me](https://github.com/sillydan1) directly.
