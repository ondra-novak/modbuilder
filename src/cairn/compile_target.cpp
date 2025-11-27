export module cairn.compile_target;

import <filesystem>;


export struct CompileTarget {
    std::filesystem::path target;
    std::filesystem::path source;    
};