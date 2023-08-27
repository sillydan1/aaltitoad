<!---
aaltitoad - a verification engine for tick tock automata models
  Copyright (C) 2023 Asger Gitz-Johansen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
--->

<p align="center">
   <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/sillydan1/AALTITOAD/master/.github/resources/logo/toad_title_darkmode.svg">
      <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/sillydan1/AALTITOAD/master/.github/resources/logo/toad_title_lightmode.svg">
      <img alt="aaltitoad" width="256" src="https://raw.githubusercontent.com/sillydan1/AALTITOAD/master/.github/resources/toad_title_darkmode.svg" style="max-width: 100%;">
   </picture>
</p>

<p align="center">
   <b>Aal</b>borg <b>Ti</b>ck <b>To</b>ck <b>A</b>utomata Vali<b>d</b>ator - an extendable verification engine and simulator for Tick Tock Automata. 
</p>

------

## What is a Tick Tock Automata?
TTA's (Tick Tock Automata) are an Automata based theory that can model parallel business logic for all kinds of automation systems.
A TTA consists of a set of locations and transitions between them. 
Each transition can be guarded by a _boolean expression_ and have a set of _variable assignments_ as a "consequence" of _taking_ the transition.

A good analogy to TTA is a _statemachine_, where a system can transition from one state to another. 
Below is an example of a simple TTA that controls a light based on a button input.

<p align="center">
   <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/sillydan1/AALTITOAD/master/.github/resources/docs/simple_tta_darkmode.svg">
      <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/sillydan1/AALTITOAD/master/.github/resources/docs/simple_tta_lightmode.svg">
      <img alt="simple tta" src="https://raw.githubusercontent.com/sillydan1/AALTITOAD/master/.github/resources/simple_tta_darkmode.svg" style="max-width: 100%;">
   </picture>
</p>

Starting in the OFF location, there's only one outgoing edge with the guard that checks if `btn_is_pressed` is _true_.
If it is, we move to the ON location and set the variable `light` to _true_ as a consequence of _taking_ the edge/transition.
Then, when the button is released we return to the OFF locaiton and `light` is set to _false_ again. 

What makes TTAs different from any other transition system is that the `btn_is_pressed` variable is an _external_ variable, 
meaning that it cannot be assigned to any value by the transition system, it can only change based on an external input.
External inputs are read when we are not taking transitions.
This split of the semantics is where the name Tick Tock Automata comes from:
 - Tick-step: evaluate guards, apply updates, change locations
 - Tock-step: read external inputs, write external outputs

Taking the syntax one step further, you can have many of these TTAs running in parallel, sharing the same pool of internal and external variables.
Such a network is called simply a **n**etwork of **t**ick **t**ock **a**utomata (NTTA).

### Further Reading
 - [This](.github/resources/docs/SW10__Tick_Tock_Automata.pdf) paper describes the base Tick Tock Automata theory
 - [This](.github/resources/docs/aaltitoad-v1.0.0.pdf) paper describes in detail the newest iteration of the tool (v1.0.0+)
 - [This](.github/resources/docs/SW9__AALTITOAD.pdf) paper describes the first iteration of the tool (v0.10.x)
 - Take a look at [H-UPPAAL](https://github.com/DEIS-Tools/H-Uppaal) if you want to create some TTAs yourself

------

## Compile (Linux)
Aaltitoad is built using cmake. You must have a C++20 compatible compiler, `flex`, `bison` version 3.5+ and the standard template library installed.
All other dependencies are handled through the wonderful [CPM](https://github.com/cpm-cmake/CPM.cmake) package manager.
```shell
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
If the CPM step is taking a long time, try rerunning with `-DCPM_SOURCE_CACHE=~/.cache/CPM`

### Test
To run the unit tests, compile the `aaltitoad_tests` target
```shell
mkdir build-test && cd build-test
cmake -DCMAKE_BUILD_TYPE=Debug ..
make aaltitoad_tests
# run the tests
./aaltitoad_test -r xml -d yes --order lex
```
If you want to include code-coverage stats, provide the `-DCODE_COVERAGE=ON` cmake flag.

## How To Use
Aaltitoad provides three primary compilation targets. All commandline interfaces have a help page that you can summon with the `--help` argument.
 - `verifier`: A verification engine command line interface
   - use this if you want to analyze your NTTA
 - `simulator`: A runtime command line interface
   - use this if you want to execute your NTTA and link with your custom tockers (see below)
 - `aaltitoad`: A library with all things aaltitoad
   - use this to create your own NTTA-based applications e.g. another verifier, runtime or even compiler

------

## Tocker plugins
Aaltitoad supports third party "tocker" libraries, so you can inject custom integrations directly into the runtime.
This repository provides an example tocker called `pipe_tocker` in the `tocker_plugins` directory to serve as an example project.
A "tocker" must define the following symbols:
```c++
#include <plugin_system.h> // aaltitoad::* typedefs

extern "C" {
    const char* get_plugin_name(); // Return value of this func dictates the name the next two funcs
    aaltitoad::tocker_t* create_TOCKER_NAME(const std::string& argument, const aaltitoad::ntta_t& ntta);
    void destroy_TOCKER_NAME(tocker_t* tocker);
    plugin_type get_plugin_type() {
        return plugin_type::tocker;
    }
}
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

------

## Parser Plugins
If you don't want to use the included parser, you can always supply your own. 
As long as you provide the following symbols (remember `extern "C"`):

```c++
#include <plugin_system.h> // aaltitoad::* typedefs 

extern "C" {
    const char* get_plugin_name();
    const char* get_plugin_version();
    aaltitoad::ntta_t* load(const std::vector<std::string>& folders, const std::vector<std::string>& ignore_list);
    plugin_type get_plugin_type() {
        return plugin_type::parser;
    }
}
```

To make it a bit easier constructing the `aaltitoad::ntta_t` object, we provide some builder classes: `aaltitoad::tta_builder` and `aaltitoad::ntta_builder`
