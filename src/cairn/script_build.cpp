export module cairn.script_build;

import cairn.module_database;
import cairn.build_plan;
import cairn.utils.arguments;
import cairn.utils.utf8;

import <fstream>;
import <set>;
import <map>;
import <unordered_map>;
import <filesystem>;
import <string>;
import <string_view>;
import <span>;
import <vector>;

/*
static void output_cmd_line(const std::filesystem::path &base_dir,
                            const std::filesystem::path &workdir,
                            const std::span<const ArgumentString> &arguments) {
    
}

*/

static ArgumentString splice_string(ArgumentStringView src_text, ArgumentStringView remove_seq, ArgumentStringView add_seq) {
    auto n = src_text.find(remove_seq);    
    ArgumentString out;
    if (n == src_text.npos) {
        out.append(src_text);
    } else {
        out.append(src_text.substr(0,n));
        out.append(add_seq);
        out.append(splice_string(src_text.substr(n+remove_seq.size()), remove_seq, add_seq));        
    }    
    return out;
}

constexpr auto special_bash =  ArgumentConstant( " \t\n\"'\\$*?[]{}()<>;&|!");


struct escape_arg_bash {
    ArgumentString s;
    escape_arg_bash(ArgumentString s):s(std::move(s)) {}

    constexpr static bool needs_shell_quoting(ArgumentStringView s) {
        return s.find_first_of(special_bash) != s.npos;
    }

    template<typename IO>
    friend IO &operator << (IO &out, const escape_arg_bash &me) {
        bool q = needs_shell_quoting(me.s);        
        if (q) out << '\'';
        to_utf8(me.s.begin(), me.s.end(), std::ostreambuf_iterator<char>(out));
        if (q) out << '\'';
        return out;
    }
};

struct escape_arg_bat {
    ArgumentString s;
    escape_arg_bat(ArgumentString s):s(std::move(s)) {}
    
    static std::string escapeForBatArg(std::string s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '^': out += "^^"; break;
            case '&': out += "^&"; break;
            case '|': out += "^|"; break;
            case '<': out += "^<"; break;
            case '>': out += "^>"; break;
            case '(': out += "^("; break;
            case ')': out += "^)"; break;
            case '%': out += "%%"; break;  // pozor při delayed expansion
            case '"': out += "\\\""; break;  // escapování uvnitř uvozovek
            default: out += c;
        }
    }
    if (out.find_first_of(" \"") == out.npos) return out;
    return "\"" + out + "\""; // celé uzavřít do uvozovek
}

    template<typename IO>
    friend IO &operator << (IO &out, const escape_arg_bat &me) {
        std::string buff;
        to_utf8(me.s.begin(), me.s.end(), std::back_inserter(buff));
        buff = escapeForBatArg(buff);
        out << buff;
        return out;
    }
};


export void generate_makefile(const BuildPlan<ModuleDatabase::CompileAction> &plan,            
            std::filesystem::path output            
        ) {
    auto cur_dir = output.parent_path();
    std::ofstream mk(output, std::ios::out|std::ios::trunc);
    std::size_t idx = 0;
    std::set<unsigned int> all_targets;
    mk << ".PHONY: all";
    for (idx = 0; idx < plan.get_plan().size(); ++idx) {
        mk << " t_" << idx;
        all_targets.insert(static_cast<unsigned int>(idx));
    }    
    for (idx = 0; idx < plan.get_plan().size(); ++idx) {
        for (const auto &x: plan.get_plan()[idx].dependencies) {
            all_targets.erase(static_cast<unsigned int>(x));
        }
    }
    
    std::set<std::filesystem::path> workdirs;

    mk << "\nall:";
    for (auto &x: all_targets) {
        mk << " t_"<< x;
    }
    mk << "\n";
    mk << "\n";
    idx = 0;
    ArgumentString srchpath = path_arg(cur_dir);

    if (plan.begin() != plan.end()) {
        std::string compiler_fname;
        plan.begin()->action.generate_compile_commands([&](const std::filesystem::path &,
                                          const std::filesystem::path &,
                                          const std::filesystem::path &,
                                          const std::filesystem::path &compiler,
                                          std::vector<ArgumentString> &&) {
            compiler_fname = compiler.filename().string();
        });
        mk << "CLANG ?= " << compiler_fname << "\n\n";
        

    }

    for (const auto &p:plan) {
        mk << "t_" << idx << ":";
        for (auto &d: p.dependencies) {
            mk << " t_" << d;
        }
        mk << "| workdir \n";

        p.action.generate_compile_commands([&](const std::filesystem::path &directory,
                                          const std::filesystem::path &,
                                          const std::filesystem::path &output,
                                          const std::filesystem::path &,
                                          std::vector<ArgumentString> &&arguments) {
            auto relpath = path_arg(std::filesystem::relative(directory/"~", cur_dir).parent_path());
            auto relpath_back = path_arg(std::filesystem::relative(cur_dir/"~", directory).parent_path());
            mk << "\tcd " << escape_arg_bash(relpath) << "; ${CLANG} ";
            for (auto &a: arguments) mk << " " <<  escape_arg_bash(splice_string(a, srchpath, relpath_back));
            mk << "\n";
            workdirs.insert(output.parent_path());
        });
        ++idx;
        mk << "\n";
    }
    mk << "\nworkdir:\n";
    for (auto &w: workdirs) {
        auto p = splice_string(path_arg(w),srchpath, string_arg("."));
        mk << "\tmkdir -p " << escape_arg_bash(p) << "\n";
    }
}

export void generate_batch(const BuildPlan<ModuleDatabase::CompileAction> &plan,            
                        std::filesystem::path output) {
    
    std::ofstream out(output, std::ios::trunc|std::ios::out);
    out << "@echo off\n";
    if (plan.begin() != plan.end()) {
        plan.begin()->action.generate_compile_commands([&](const std::filesystem::path &,
                                          const std::filesystem::path &,
                                          const std::filesystem::path &,
                                          const std::filesystem::path &compiler,
                                          std::vector<ArgumentString> &&) {
            out << "SET CLANG=" << escape_arg_bat(path_arg(compiler.filename())) << "\n"
                  "IF NOT \"%1\" == \"\"  SET CLANG=%1\n";
            
        });

    }

    out << "goto :init\n\n"
           ":compile\n";

 
    auto cur_dir = output.parent_path();
    ArgumentString srchpath = path_arg(cur_dir);
    std::set<std::filesystem::path> workdirs;



    auto state = plan.initialize_state();
    while (!plan.prepare_actions(state, [&](auto idx, const ModuleDatabase::CompileAction &a) {
        a.generate_compile_commands([&](const std::filesystem::path &directory,
                                          const std::filesystem::path &,
                                          const std::filesystem::path &output,
                                          const std::filesystem::path &,
                                          std::vector<ArgumentString> &&arguments) {
            auto relpath = path_arg(std::filesystem::relative(directory/"~", cur_dir).parent_path());
            auto relpath_back = path_arg(std::filesystem::relative(cur_dir/"~", directory).parent_path());
            out << "pushd " << escape_arg_bat(relpath) << "\n%CLANG%";
            for (auto &a: arguments) out << " " <<  escape_arg_bat(splice_string(a, srchpath, relpath_back));
            out <<"\n"                                        
                "popd\n";
            workdirs.insert(output.parent_path());
        });
        plan.mark_done(state, idx);
        return true;
    }));   
    out << "exit /b 0\n\n"
           ":init\n";
    for (auto &w: workdirs) {
        auto p = splice_string(path_arg(w),srchpath, string_arg("."));
        out << "md  " << escape_arg_bat(p) << "\n";
    }

    out << "goto :compile\n";


}
   