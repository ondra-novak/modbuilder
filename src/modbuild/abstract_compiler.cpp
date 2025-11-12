#include "abstract_compiler.hpp"
#include "utils/process.hpp"

int AbstractCompiler::spawn_compiler(const Config &cfg, const std::filesystem::path &workdir, std::span<const ArgumentString> arguments, std::vector<ArgumentString> *dump_cmdline)
{
    if (dump_cmdline) {
        dump_cmdline->push_back(path_arg(cfg.program_path));
        dump_cmdline->insert(dump_cmdline->end(), arguments.begin(), arguments.end());
    }

    if (cfg.dry_run) return 0;
    Process p = Process::spawn(cfg.program_path, workdir, arguments, true);
    return p.waitpid_status();

}
