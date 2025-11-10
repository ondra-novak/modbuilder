#pragma once


enum class ModuleType {
    ///Interface module
    /**
     Interface module has "export module Name;"
    */
    interface,

    ///Implementation module
    /**
    Implementation module has "module Name;"
    */
    implementation,

    ///Module partition includes both interface and implementation
    /**
    generates BMI and O
    but are considered as implementation
    but can be imported
     */
    partition,

    ///Ordinary source file
    /**
     Just a source file not declared as module, but still can import
    */
    source,

    ///Module is precompiled system header
    system_header,


    ///Module is precompiled user header
    user_header,
};

