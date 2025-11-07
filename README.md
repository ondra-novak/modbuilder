Very experimental C++ module build tool

Goal: zero conf module builder

intended use:

```
modbuilder <switches> main_file.cpp <compiler> <args> /link <linker args>

example:

modbuilder main_file.cpp cl.exe /Iincludes /std:c++latest /nologo /W2 /O0 /link /SUBSYSTEM:CONSOLE /DEBUG

```

- discovers all modules
- creates build plan
- invokes compiler
- invokes linker

## module discovery

- current directory
- modules.json

## modules.json

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

