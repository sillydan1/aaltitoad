# AALTITOAD Changelog

## [v1.0.0](https://github.com/sillydan1/AALTITOAD/issues/37)
 - A complete rewrite of the `NTTA` semantics
   - The tick-choice resolution algorithm is much more researched and in-depth. See the [documentation]() for more info
   - The tock-choice resolution algorithm is now based on SAT-solver technology and should be much faster and produce less choices
   - All `TTA` syntax has been refined using [yalibs](https://github.com/yalibs) libraries as a base
 - Huge overhaul to the general structure of `aaltitoad`
   - The `NTTA` semantics and verification algorithms are implemented as libraries  
   - Command line interfaces are shallow executables that wrap library function interactions, such that humans can interact with them
   - Plugin-able things such as **tockers** and **syntax parsers** are now dynamically-linkable, so third-parties can inject their own modules without licensing issues

## v0.10.2 and below
 - [Added](https://github.com/sillydan1/AALTITOAD/pull/26) CI via github actions
 - [Added](https://github.com/sillydan1/AALTITOAD/pull/27) fischer-2/5/10 mutex variants for testing
 - [Added](https://github.com/sillydan1/AALTITOAD/pull/13) an attempt to fix hash-collision issue
 - [Added](https://github.com/sillydan1/AALTITOAD/pull/28) version info through CMAKE and this changelog
