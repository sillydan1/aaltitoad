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
This repository provides an example tocker called `pipe_tocker` in the `tocker_plugins` directory to serve as an example project.
A "tocker" must define the following symbols:
```c++
const char* get_plugin_name(); // Return value of this func dictates the name the next two funcs
tocker_t* create_TOCKER_NAME(const std::string& argument, const ntta_t& ntta);
void destroy_TOCKER_NAME(tocker_t* tocker);
```
The `TOCKER_NAME` refers to the C-String provided by the `get_plugin_name` function. 
The function names MUST match, otherwise the plugin will not load.
Tocker plugins must link with the `aaltitoad` shared library, and should be compiled as a shared library, like so:
```cmake
add_library(TOCKER_NAME_tocker SHARED *.cpp)
target_link_libraries(TOCKER_NAME_tocker aaltitoad)
```
Once compiled, you can instantiate the tocker by providing the `--tocker / -t` together with `--tocker-dir / -T` to the aaltitoad command line.
The `--tocker` option should be provided like so:
```shell
--tocker-dir /path/to/tocker-plugins --tocker "TOCKER_NAME(argument string)" 
```
The `"argument string"`-part of the option refers to the input argument string provided to the `create_TOCKER_NAME` function.
Now your tockers should've been instantiated, and it's `tock` function will be called each time the TTA is ready to calculate tocker values.

### Tocker Types
There are two types of tocker interfaces that a third party tocker can extend:
 - Blocking tockers (default `tocker_t`)
 - Asynchronous tockers (`async_tocker_t`)

The blocking tocker will run in a single thread and will block all tick-execution until the tock has completed, 
and a third-party integration should only override the `tock` function.
In contrast, the asynchronous tocker will not block tick execution, but start a new asynchronous task of the `get_tock_values` function. 

When implementing your tocker, you should consider if you want to block the TTA from executing or not.
If you choose to implement an asynchronous tocker, be aware that the input environment can have changed since the tock was issued. 

### Timers
TODO: Think about timers in the context of custom tockers

If you have any questions, you are welcome to reach out to [me](https://github.com/sillydan1) directly.
