


Very experimental C++ module build tool

# Goal: zero conf module builder

# State

experimental / in progresss

# Usage

```
This software is released under the MIT License. https://opensource.org/licenses/MIT

Usage:

cairn [...switches...] <output1=file1.cpp> [<output2=file2.cpp>... ] <compiler> [...compiler/linker..flags...]

Switches
========
-jN       specify count of thread
-p<type>  select compiler type: gcc|clang|msvc
-c<path>  generate compile_commands.json (path=where)
-f<file>  specify environment file (modules.json) for this build
-b<dir>   specify build directory
-C        compile only (doesn't run linker)
-L        link only (requires compiled files in build directory)
-k        keep going
-r        recompile whole database even if modules are not referenced
-y        dry-run don't compile (but can generate compile_commans.json)
-s        output only errors (silent)
-d        debug mode (output everyting)

outputN   specifies path/name of output executable
fileN.cpp specifies path/name of main file for this executable

          you can specify multiple targets 
          target1=file1 target2=file2 target3=file3 ....
          there are no spaces before and after '='


compiler  path to compiler's binary. PATH is used to search binary

Compiler arguments
==================
All arguments listed here are copied to the compiler command line when it is run.
However, there are switches that have special meanings and are not copied. These
switches end with a colon.

--compile:  following arguments are used only during compilation phase
--link:     following arguments are used only during link phase
--lib:      produces library (will not link), following arguments are
            used by librarian. This also activates library build 

Example: gcc -DSPECIAL -I/usr/local/include --compile: -O2 -march=native --link: -o example -lthread

Module discovery
================
A modules.json file can be defined in each module directory. The file is in JSON Object format 
with the following fields

files       array[] : list of files to be scanned for modules
prefixes    object{} : key - prefix (for module name)
                                example: "A" - for all modules matching pattern A.xxx.yyy...
                                example: "A%" - for all modules matching pattern Axxx.yyy...
                                example: "" - all other modules 
                       value - string or array - contains path(s) to other directories
                                 can be relative
work_dir    string : optional - can specify different working directory
includes    array[] : list of include paths (required for -I switch)
options     array[] : list of other compile options specific for this project

If modules.json is missing, then all *.cpp files in current directory are scanned for
modules
```
## example usage

```
cairn -j10 -cbuild/compile_commands.json build/test=module_example/basic/test.cpp g++-14 -std=c++20 -O0 -ggdb
```


# features
- discovers all modules
- creates build plan
- invokes compiler
- invokes linker

# module discovery

- current directory
- modules.json

# modules.json

```json
{
    "files":["file1.cpp","file2.cpp","list of active files current directory"],
    "prefixes":{
        "Module1":"../module1_dir",
        "Module2":"module2_dir",
    }
}
```

each directory can have own modules.json
The key "prefixes" includes all modules which name starts by prefix. For example Module1.A, Module1.B

