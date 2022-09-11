# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [v0.11.0](https://github.com/sillydan1/AALTITOAD/releases/tag/v0.11.0) - 2022-09-11

<small>[Compare with v0.10.2](https://github.com/sillydan1/AALTITOAD/compare/v0.10.2...v0.11.0)</small>

### Bug Fixes
- Add huppaal_parser unit tests ([e7d2697](https://github.com/sillydan1/AALTITOAD/commit/e7d26974f297e536eca9ab0ebbaa4efe05ef1187) by Asger Gitz-Johansen).
- Handle edge case when query is reachable after initial tock ([79f5904](https://github.com/sillydan1/AALTITOAD/commit/79f59048ff1f9271588712f61ff2d1dcb9b8cff4) by Asger Gitz-Johansen).
- Forgot about external_variables in operator<< and operator+ for ntta_t ([b1bb2bb](https://github.com/sillydan1/AALTITOAD/commit/b1bb2bbe64c4d5ce87894be23391df1e71d216f4) by Asger Gitz-Johansen).
- Ntta_builder should be able to take external variables ([432ed67](https://github.com/sillydan1/AALTITOAD/commit/432ed6759351e0e0ecef38bcaeba186fcea4c06f) by Asger Gitz-Johansen).
- Forgot to check for satisfaction in a specific edge case ([d60ed02](https://github.com/sillydan1/AALTITOAD/commit/d60ed022242e08a45f8d576d85e6b264b992c1bf) by Asger Gitz-Johansen).
- Add cmake additions of --coverage flags when -dcode_coverage is provided ([b7aeedf](https://github.com/sillydan1/AALTITOAD/commit/b7aeedfa2c150c98442c7c38539d64adde086cc1) by Asger Gitz-Johansen).
- Use the correct uniform distribution in random.cpp ([4fe5124](https://github.com/sillydan1/AALTITOAD/commit/4fe512415f5e40db0d73b2a203bbe3c2e7b7b5b1) by Asger Gitz-Johansen).
- Code coverage setup ([5c53f86](https://github.com/sillydan1/AALTITOAD/commit/5c53f8694d0ee4d38a03ba38d0f682a1dcb62923) by Asger Gitz-Johansen).
- Logic bug where unnecessary states were added to the waiting list ([1e8229e](https://github.com/sillydan1/AALTITOAD/commit/1e8229e4556753518c639154219197c778e042de) by Asger Gitz-Johansen).
- Increase performance for interesting_tocker ([46a54e5](https://github.com/sillydan1/AALTITOAD/commit/46a54e5a6c4312a08e6326c00c7e0ccbc6d22ef8) by Asger Gitz-Johansen).
- Moved ntta_builder implementations into the cpp-file ([22d86b5](https://github.com/sillydan1/AALTITOAD/commit/22d86b510263971b12c9056e6cfb6b08bd2ee8c3) by Asger Gitz-Johansen).
- Component location manipulation was malformed ([57c6f7d](https://github.com/sillydan1/AALTITOAD/commit/57c6f7d957a062605971074dcb5bb5bb3d47d8c2) by Asger Gitz-Johansen).

### Code Refactoring
- Comment cleanup ([dd501f5](https://github.com/sillydan1/AALTITOAD/commit/dd501f54d5339a6f56218dcdaf1074768245eaaa) by Asger Gitz-Johansen).
- Reformulated a todo and cleaned up ([41d2a1e](https://github.com/sillydan1/AALTITOAD/commit/41d2a1e8957cf27d9e8f5ba30b815192b35196aa) by Asger Gitz-Johansen).
- Add some extra tocker information trace logs ([33cef48](https://github.com/sillydan1/AALTITOAD/commit/33cef48f8c7619c000dfee714cab946e1b8662e2) by Asger Gitz-Johansen).

### Features
- Add verifier cli ([0015d9e](https://github.com/sillydan1/AALTITOAD/commit/0015d9e98d2d1ca6b002c02f2ebc2b97ddc5cee7) by Asger Gitz-Johansen).
- Add regex-style ignore lists ([c864251](https://github.com/sillydan1/AALTITOAD/commit/c8642513265b6bc4876d101616b80b4ea7f1bdfe) by Asger Gitz-Johansen).
- Add ntta builder that works on strings ([54f73dd](https://github.com/sillydan1/AALTITOAD/commit/54f73dd9f954be41e973352e986f029bf1e11588) by Asger Gitz-Johansen).
- Add basic huppaal_parser ([fed8d2e](https://github.com/sillydan1/AALTITOAD/commit/fed8d2ec855af1c4fc803c658ba52377c25c79e3) by Asger Gitz-Johansen).
- Add to_string function to ntta_t ([2d3bed6](https://github.com/sillydan1/AALTITOAD/commit/2d3bed66c7957fc286856fe737b53cb290fd5709) by Asger Gitz-Johansen).
- Made traceable multimap iterable ([3078d2b](https://github.com/sillydan1/AALTITOAD/commit/3078d2b5ead6afeef845d8865f05795b9775a906) by Asger Gitz-Johansen).
- Add waiting list pick strategy injection to frs ([33cb23c](https://github.com/sillydan1/AALTITOAD/commit/33cb23c117aea70afd7acf029ee26c359817efb0) by Asger Gitz-Johansen).
- Add simple reachability test for the forwad_reachability_searcher ([bbf57ea](https://github.com/sillydan1/AALTITOAD/commit/bbf57eabf7373df6f83d22227a4c626284ba9d94) by Asger Gitz-Johansen).
- Add traceable return types and support for multiple query-checking ([2af3bde](https://github.com/sillydan1/AALTITOAD/commit/2af3bde104d38c9f961dcfce22bc756bec07b6ec) by Asger Gitz-Johansen).
- Add traceability to waiting/passed sets to the forward reachability searcher ([3446da7](https://github.com/sillydan1/AALTITOAD/commit/3446da7b816f0216fd6e91e91f3854384de5bcdb) by Asger Gitz-Johansen).
- Add ctl satisfiability relation ([7400f61](https://github.com/sillydan1/AALTITOAD/commit/7400f6190122df54d58967ab561cdb6c3f175cc9) by Asger Gitz-Johansen).


## [v0.10.2](https://github.com/sillydan1/AALTITOAD/releases/tag/v0.10.2) - 2022-05-05

<small>[Compare with v0.10.1](https://github.com/sillydan1/AALTITOAD/compare/v0.10.1...v0.10.2)</small>


## [v0.10.1](https://github.com/sillydan1/AALTITOAD/releases/tag/v0.10.1) - 2022-04-29

<small>[Compare with v0.10.0](https://github.com/sillydan1/AALTITOAD/compare/v0.10.0...v0.10.1)</small>


## [v0.10.0](https://github.com/sillydan1/AALTITOAD/releases/tag/v0.10.0) - 2022-04-21

<small>[Compare with v0.9.2](https://github.com/sillydan1/AALTITOAD/compare/v0.9.2...v0.10.0)</small>


## [v0.9.2](https://github.com/sillydan1/AALTITOAD/releases/tag/v0.9.2) - 2022-04-07

<small>[Compare with 0.9b](https://github.com/sillydan1/AALTITOAD/compare/0.9b...v0.9.2)</small>


## [0.9b](https://github.com/sillydan1/AALTITOAD/releases/tag/0.9b) - 2020-12-10

<small>[Compare with 0.9](https://github.com/sillydan1/AALTITOAD/compare/0.9...0.9b)</small>


## [0.9](https://github.com/sillydan1/AALTITOAD/releases/tag/0.9) - 2020-12-09

<small>[Compare with first commit](https://github.com/sillydan1/AALTITOAD/compare/ac0690494b01038e19b5cac8226b1e6ae7a715e5...0.9)</small>


