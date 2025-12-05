export module cairn.script_build;

import cairn.module_database;
import cairn.build_plan;
import cairn.utils.arguments;
import cairn.utils.utf8;

import <fstream>;
import <set>;
import <filesystem>;
import <string>;
import <string_view>;

static void output_cmd_line(const std::filesystem::path &base_dir,
                            const std::filesystem::path &workdir,
                            const std::span<const ArgumentString> &arguments) {
    
}


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
    for (const auto &p:plan) {
        CompileCommandsTable cctmp;
        p.action.add_to_cctable(cctmp);
        mk << "t_" << idx << ":";
        for (auto &d: p.dependencies) {
            mk << " t_" << d;
        }
        mk << "| workdir \n";
        for (auto &[k,v]: cctmp._table) {
            
            auto relpath = std::filesystem::relative(v.directory/"~", cur_dir).parent_path();
            auto relpath_back = std::filesystem::relative(cur_dir/"~", v.directory).parent_path();
            ArgumentString arg = splice_string(v.command, srchpath, path_arg(relpath_back)); 
            mk << "\tcd " << relpath << "; ";
            to_utf8(arg.begin(), arg.end(), std::ostreambuf_iterator<char>(mk));
            mk << "\n";
            workdirs.insert(v.output.parent_path());
        }
        ++idx;
        mk << "\n";
    }
    mk << "\nworkdir:\n";
    for (auto &w: workdirs) {
        auto p = splice_string(path_arg(w),srchpath, string_arg("."));
        mk << "\tmkdir -p ";
        to_utf8(p.begin(), p.end(), std::ostreambuf_iterator<char>(mk));
        mk << "\n";
    }
}

export void generate_batch(const BuildPlan<ModuleDatabase::CompileAction> &plan,            
                        std::filesystem::path output) {
    
 
    auto cur_dir = output.parent_path();
    ArgumentString srchpath = path_arg(cur_dir);

    std::vector<CompileCommandsTable::CCRecord> seq_plan;
    std::set<std::filesystem::path> workdirs;
    auto state = plan.initialize_state();
    while (plan.prepare_actions(state, [&](auto , const ModuleDatabase::CompileAction &a) {
        CompileCommandsTable cctmp;
        a.add_to_cctable(cctmp);
        for (auto &[k,v]: cctmp._table) {
            seq_plan.push_back(v);
            workdirs.insert(v.output.parent_path());
        }
        return true;
    }));
    
    std::ofstream out(output, std::ios::trunc|std::ios::out);
    if (!out) throw std::runtime_error("Can't open output file: " + output.string());
    for (auto &d: workdirs) {
        auto a = splice_string(path_arg(d), srchpath, string_arg("."));
        out << "mkdir \"";
        to_utf8(a.begin(), a.end(), std::ostreambuf_iterator<char>(out));
        out << "\"\n";
    }
    for (auto &v: seq_plan) {
        auto relpath = std::filesystem::relative(v.directory/"~", cur_dir).parent_path();
        auto relpath_back = std::filesystem::relative(cur_dir/"~", v.directory).parent_path();
        ArgumentString arg = splice_string(v.command, srchpath, path_arg(relpath_back)); 
        out << "pushd \"" << relpath.u8string() << "\"\n";
        to_utf8(arg.begin(), arg.end(), std::ostreambuf_iterator<char>(out));
        out << "\n";
        out << "popd\n";
   }       


}