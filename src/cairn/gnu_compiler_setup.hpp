#include "preprocess.hpp"
#include "utils/arguments.hpp"
#include "utils/process.hpp"
#include "utils/thread_pool.hpp"
#include <filesystem>
#include <iterator>
#include <sstream>
#include <vector>

inline  std::vector<std::filesystem::path> extract_include_path(std::string text, const std::filesystem::path working_dir) {    
    std::vector<std::filesystem::path> res;

    std::string ln;

    std::istringstream strm(std::move(text));
    bool record = false;
    while (!!strm) {
        std::getline(strm, ln);
        std::string_view lnw(ln);
        while (!lnw.empty() && isspace(lnw.front())) lnw = lnw.substr(1);
        while (!lnw.empty() && isspace(lnw.back())) lnw = lnw.substr(0,lnw.size()-1);
        if (record) {
            if (lnw == "End of search list.") break;
            res.emplace_back((working_dir/lnw).lexically_normal());
        } else if (lnw == "#include <...> search starts here:") {
            record = true;
        }
    }
    return res;
}




inline StupidPreprocessor initialize_preprocesor_using_gnu_compiler(std::filesystem::path program_path, ThreadPool &tp) {

    std::vector<ArgumentString> args;
    auto curdir =std::filesystem::current_path();
    append_arguments(args, {"-v","-E","-dM","-"},{});
    Process p = Process::spawn(program_path, curdir, args, Process::input_output_error);
    p.stdin_stream.reset(); 
    p.child_stdin_buf.reset();  //close stream;
    std::string outstr;
    std::string errstr;
    std::atomic<bool> done(false);
    tp.push([&]() noexcept {
        errstr = std::string(std::istreambuf_iterator<char>(*p.stderr_stream),std::istreambuf_iterator<char>());
        done = true;
        done.notify_all();
    });
    outstr = std::string(std::istreambuf_iterator<char>(*p.stdout_stream),std::istreambuf_iterator<char>());
    done.wait(false);
    p.waitpid_status();

    auto paths = extract_include_path(errstr, curdir);
    StupidPreprocessor preproc;
    preproc.append_includes(paths);
    std::istringstream instr(outstr);
    std::ostringstream dummy;
    preproc.run(curdir, instr, StupidPreprocessor::ScanMode::collect, dummy);
    return preproc;
}   

//preprocessor options
static constexpr auto preproc_D = ArgumentConstant("-D");
static constexpr auto preproc_U = ArgumentConstant("-U");
static constexpr auto preproc_I = ArgumentConstant("-I");
static constexpr auto preproc_define_macro = ArgumentConstant("--define-macro");
static constexpr auto preproc_undefine_macro = ArgumentConstant("--undefine-macro");
static constexpr auto preproc_include_directory = ArgumentConstant("--include-directory");



inline std::string run_preprocess(StupidPreprocessor &preproc,
         std::span<const ArgumentString> args,
         const std::filesystem::path &workdir,
         const std::filesystem::path &file)  {

    int a = -1;
    for (ArgumentStringView itm: args) {
        if (itm == preproc_D || itm == preproc_define_macro) {
            a = 0;
            continue;
        } else if (itm == preproc_I || itm == preproc_include_directory){
            a = 1;
            continue;
        } else if (itm == preproc_U || itm == preproc_define_macro) {
            a = 2;
            continue;
        } else if (itm.substr(0, preproc_D.length()) == preproc_D) {
            a = 0;
            itm = itm.substr(preproc_D.length());            
        } else if (itm.substr(0, preproc_I.length()) == preproc_I) {
            a = 1;
            itm = itm.substr(preproc_U.length());            
        } else if (itm.substr(0, preproc_U.length()) == preproc_U) {
            a = 2;
            itm = itm.substr(preproc_U.length());            
        }
        switch (a) {
            case 0: {
                auto sep = std::min(itm.find(' '), itm.length());
                std::string key;
                std::string value;
                to_utf8(itm.data(), itm.data()+sep, std::back_inserter(key));
                if (sep < itm.length()) ++sep;
                to_utf8(itm.data()+sep, itm.data()+itm.length(), std::back_inserter(value));
                preproc.define_symbol(key,value);
                break;
            } 
            case 1: {
                preproc.append_includes(workdir/itm);
                break;
            }
            case 2: {
                std::string value;
                to_utf8(itm.begin(), itm.end(), std::back_inserter(value));
                preproc.undef_symbol(value);
                break;
            }
            default:break;
        }
        a = -1;
    }

    return preproc.run(workdir, file);
}
