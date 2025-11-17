#include "module_database.hpp"
#include "json/value.h"
#include <atomic>
#include "module_resolver.hpp"
#include "module_type.hpp"
#include "scanner.hpp"
#include "utils/log.hpp"
#include "utils/filesystem.hpp"
#include "abstract_compiler.hpp"
#include "compile_commands_supp.hpp"
#include <queue>
#include <unordered_set>
#include <ranges>

json::value ModuleDatabase::export_db() const {
    auto refjson = [](const Reference &ref) {
                    return json::value{
                        {"type", static_cast<int>(ref.type)},
                        {"name",ref.name},
                    };
    };

    auto flt = _originMap | std::ranges::views::filter([](const auto &x){return x.second != nullptr;});    

    json::value alldata (flt.begin(), flt.end() ,[&](const auto &itm){
        const POriginEnv &org = itm.second;
        if (org) {
            auto flt = _fileIndex | std::ranges::views::filter([&](const auto &x){return x.second->origin == org;});    
            return json::value {
                {"config_file", org->config_file.u8string()},
                {"work_dir", org->working_dir.u8string()},
                {"hash", org->settings_hash},
                {"includes", json::value(org->includes.begin(), org->includes.end(), 
                    [](const std::filesystem::path &p)->json::value_t{return p.u8string();})},
                {"options", json::value(org->options.begin(), org->options.end())},
                {"files", json::value(flt.begin(), flt.end(), [&](const auto &kv){
                    const PSource src = kv.second;
                    if (src->origin != org) return json::value();
                    return json::value {
                        {"source_file",src->source_file.u8string()},
                        {"object_path",src->object_path.u8string()},
                        {"bmi_path",src->bmi_path.u8string()},
                        {"name",src->name},
                        {"type",static_cast<int>(src->type)},
                        {"references",json::value(src->references.begin(), src->references.end(),refjson)},
                        {"exported", json::value(src->exported.begin(), src->exported.end(), refjson)},
                    };

                })}
            };
        } else {
            return json::value();
        }
    });


    return {
        {"timestamp", _import_time.time_since_epoch().count()},
        {"data", alldata},
    };
}

void ModuleDatabase::import_db(json::value db) {


    auto json2ref =  [&](const json::value &val) {
                return Reference{
                    static_cast<ModuleType>(val["type"].as<int>()),
                    val["name"].as<std::string>()
                };
    };

    

    bool d =is_dirty();
    json::value data = db["data"];
    json::value tm = db["timestamp"];
    for (auto v: data) {
        auto inc = v["includes"];
        auto opts = v["options"];

        std::vector<std::filesystem::path> paths(inc.size());
        std::vector<std::string> options(opts.size());

        std::transform(inc.begin(), inc.end(), paths.begin(), [](const json::value &v){
            return std::filesystem::path(v.as<std::u8string>());
        });
        std::transform(opts.begin(), opts.end(), options.begin(), [](const json::value &v){
            return v.as<std::string>();
        });
        auto org =  std::make_shared<OriginEnv>(OriginEnv{
            std::filesystem::path(v["config_file"].as<std::u8string_view>()),
            std::filesystem::path(v["work_dir"].as<std::u8string>()),
            v["hash"].as<std::size_t>(),
            std::move(paths),
            std::move(options)});
        auto files = v["files"];
        for (auto item: files) {
            Source src;
            src.name = item["name"].as<std::string>();
            src.source_file = item["source_file"].as<std::u8string>();
            src.object_path = item["object_path"].as<std::u8string>();
            src.bmi_path = item["bmi_path"].as<std::u8string>();        
            src.type = static_cast<ModuleType>(item["type"].as<int>());
            src.origin = org;
            auto ref = item["references"];
            std::transform(ref.begin(), ref.end(), std::back_inserter(src.references), json2ref);
            auto expr = item["exported"];
            std::transform(expr.begin(), expr.end(), std::back_inserter(src.exported), json2ref);
            put(std::move(src));
        }
    }

    _modify_time = std::chrono::system_clock::time_point(
        std::chrono::system_clock::duration(tm.as<std::chrono::system_clock::duration::rep>()));
    _import_time = std::chrono::system_clock::now();                    
    if (!d) clear_dirty();
}

void ModuleDatabase::clear() {
    _fileIndex.clear();
    _moduleIndex.clear();
    _modify_time = {};
    _import_time = std::chrono::system_clock::now();
    _modified = false;
}

ModuleDatabase::PSource ModuleDatabase::find(Reference ref) const { 

    auto iter = _moduleIndex.find(Reference{ref});
    if (iter == _moduleIndex.end()) return {};
    return iter->second.front();

}

ModuleDatabase::PSource ModuleDatabase::find(std::filesystem::path file) const {
    auto iter = _fileIndex.find(file);
    if (iter == _fileIndex.end()) return {};
    return iter->second;

}

ModuleDatabase::PSource ModuleDatabase::put(Source src) {

  auto iter = _fileIndex.find(src.source_file);
  if (iter != _fileIndex.end())
    return iter->second;

  PSource psrc = std::make_unique<Source>(src);
  _fileIndex.emplace(psrc->source_file, psrc);
  auto refiter = _moduleIndex.try_emplace(Reference{src.type, src.name}, 1, psrc);
  if (!refiter.second) {
    refiter.first->second.push_back(psrc);
  }
  if (psrc->origin) _originMap.try_emplace(psrc->origin->config_file, psrc->origin);
  set_dirty();
  return {};
}

void ModuleDatabase::erase(std::filesystem::path file) {
    auto it1 = _fileIndex.find(file);
    if (it1 == _fileIndex.end()) return;

    PSource src = it1->second;
    _fileIndex.erase(it1);
    auto it2 = _moduleIndex.find(Reference{src->type, src->name});
    if (it2 == _moduleIndex.end()) return;

    auto iter = std::remove(it2->second.begin(), it2->second.end(), src);
    it2->second.erase(iter, it2->second.end());
    if (it2->second.empty()) {
        _moduleIndex.erase(it2);
    }
    set_dirty();
    
}



std::vector<ModuleDatabase::PSource> ModuleDatabase::find_multi(Reference ref) const {    
    auto iter = _moduleIndex.find(ref);
    if (iter == _moduleIndex.end()) return {};
    else return iter->second;
}

bool ModuleDatabase::is_dirty() const { 
    return _modified.load(std::memory_order_relaxed);
}

void ModuleDatabase::clear_dirty() const {
    _modified.store(false, std::memory_order_relaxed);
}

void ModuleDatabase::set_dirty() const {
    _modified.store(true, std::memory_order_relaxed);

}

void ModuleDatabase::update_files_state(AbstractCompiler &compiler) {
    std::vector<std::filesystem::path> to_remove;
    auto cmptm = std::chrono::clock_cast<std::filesystem::file_time_type::clock>(_modify_time);
    


    //locate all modified files    
    for(const auto &[k,f]: _fileIndex) {
        f->state = {false,false};
        auto acmptm = cmptm;
        if (generates_object(f->type)) {
            if (f->object_path.empty()) f->state.recompile = true;
            else {
                std::error_code ec;
                acmptm = std::filesystem::last_write_time(f->object_path, ec);
                if (ec != std::error_code{}) f->state.recompile = true;                
            }
        }
        if (generates_bmi(f->type)) {
            if (f->bmi_path.empty()) f->state.recompile = true;        
            else {
                std::error_code ec;
                acmptm = std::filesystem::last_write_time(f->bmi_path, ec);
                if (ec != std::error_code{}) f->state.recompile = true;                
            }
        }
        if (acmptm > cmptm) acmptm = cmptm;

        auto st = compiler.source_status(f->type, k, acmptm);
        switch (st) {
            case AbstractCompiler::SourceStatus::not_exist: 
                to_remove.push_back(k); 
                break;
            case AbstractCompiler::SourceStatus::modified:
                f->state.recompile = true;
                if (!is_header_module(f->type)) f->state.rescan = true;
                break;
            default:
                break;
        }
    }
    //remove all marked files
    for (const auto &x: to_remove) erase(x);    

    //paths to rescan
    std::vector<std::filesystem::path> rescan_path;
    //new files to rescan
    std::vector<std::pair<POriginEnv, std::filesystem::path> > new_files;
    //list of files to rescan (existing)
    std::vector<PSource> rescan_existing;

    //find changes in origins
    for (const auto &[_,org]: _originMap) {
        if (org && ModuleResolver::detect_change(*org, cmptm)) {
            auto mp = ModuleResolver::loadMap(org->config_file);
            //changes origin settings
            if (mp.env.settings_hash != org->settings_hash) {
                //get new settings
                *org = mp.env;
                //find all files and mark them for recompile an rescan                
                for (const auto &[_, f]: _fileIndex) {
                    if (f->origin == org) {
                        f->state.recompile = true;                        
                        f->state.rescan = !is_header_module(f->type);
                    }
                }
            }
            //schedule for rescan directory
            rescan_path.push_back(org->config_file);
        }
    }
    //rescan all scheduled directories
    for (auto &p:rescan_path) rescan_directories({},compiler, p);

    //spread recompile flag to other files (by reference)
    //modified?
    bool mod = true;
    //repeat
    while (mod) {
        //nothing modified
        mod = false;
        //process all files
        for(const auto &[_,f]: _fileIndex) {
            //if there is still file for rescaning
            if (f->state.rescan) {
                //schedule it
                rescan_existing.push_back(f);                
                mod = true;
            }
            //spread recompile flag
            if (!f->state.recompile) {
                for (const auto &r: f->references) {
                    auto rf = find(r);
                    if (rf && rf->state.recompile) { 
                        f->state.recompile = true;
                        mod = true;
                        break;
                    }
                }
            }
        }
        //rescan all scheduled files
        for (auto r: rescan_existing) {
            rescan_file_discovery(r->origin, r->source_file, compiler);
        }
        //clear scheduled
        rescan_existing.clear();
        //repeat while modified
    }
}


ModuleDatabase::Source ModuleDatabase::from_scanner(const std::filesystem::path &source_file,
                                    const SourceScanner::Info &nfo) {
  

    Source out;
    out.name = nfo.name;
    out.type = nfo.type;
    for (const auto &r: nfo.required) {
        if (r.type == ModuleType::system_header) {
            out.references.push_back(Reference{r.type, r.name});
        } else if (r.type == ModuleType::user_header) {
            out.references.push_back(Reference{r.type, r.name});
        } else {
            out.references.push_back(Reference{r.type, r.name});
        }
    }
    out.source_file = source_file;
    return out;
}

bool ModuleDatabase::add_file(const std::filesystem::path &source_file, AbstractCompiler &compiler) {
    auto f = find(source_file);    
    if (f) return false;
    auto mm = ModuleResolver::loadMap(source_file.parent_path());
    POriginEnv env;
    auto iter = _originMap.find(mm.env.config_file);
    if (iter == _originMap.end()) {
        env = std::make_shared<OriginEnv>(mm.env);
    } else {
        env = iter->second;
    }
    rescan_file_discovery(env, source_file, compiler);
    return true;
}

ModuleDatabase::Unsatisfied ModuleDatabase::rescan_file(
        POriginEnv origin,
        const std::filesystem::path &source_file,
        AbstractCompiler &compiler
    ) {
  
    Log::debug("Scanning file: {}", source_file);

    std::vector<ModuleDatabase::Reference> unsatisfied;

    std::optional<OriginEnv> tmpenv;
    const OriginEnv *orgptr = origin.get();
    if (!orgptr) {
        tmpenv.emplace(OriginEnv::default_env());
        orgptr = &tmpenv.value();
    }
     
    
    
    //erase file from db
    erase(source_file);
    //run scanner
    auto info = compiler.scan(*orgptr, source_file);
    Source srcinfo = from_scanner(source_file, info);    
    srcinfo.origin = origin;
    auto refs = srcinfo.references;
    //put new registration
    put(std::move(srcinfo));


    //register header references
    for (auto &r: refs) {
        if (is_header_module(r.type)) {
            put(Source{r.name, r.type, r.name, origin});
        } else {
            //check reference 
            auto fs = find(r)   ;
            if (!fs || fs->state.rescan)  {
                unsatisfied.push_back(std::move(r));
            }
        }
    }
    return unsatisfied;
}

ModuleDatabase::Unsatisfied ModuleDatabase::rescan_file_discovery(POriginEnv origin, const std::filesystem::path &source_file,
           AbstractCompiler &compiler) {

    auto unsatisfied = rescan_file(origin, source_file, compiler);
    if (!unsatisfied.empty()) {
        auto startdir = origin?origin->config_file:source_file.parent_path();
        unsatisfied = rescan_directories(unsatisfied, compiler, startdir);
    }
    if (!unsatisfied.empty()) {
        Log::debug("Some modules still not resolved, performing deep search");
        for (const auto &[p,o]:  _originMap) {
            auto startdir = o->config_file;
            unsatisfied = rescan_directories(unsatisfied, compiler, startdir);
            if (unsatisfied.empty()) break;
        }
    }
    return unsatisfied;
}

ModuleDatabase::Unsatisfied ModuleDatabase::rescan_directories(
            std::span<const Reference> unsatisfied, 
            AbstractCompiler &compiler,
            std::filesystem::path start_directory) {
  
    //list of directories to process
    std::queue<std::filesystem::path> to_process;
    //list of directories already enqueued or processed
    std::unordered_set<std::filesystem::path> enqueued;
    //unsatisfied references
    std::unordered_set<Reference, MethodHash> need(unsatisfied.begin(), unsatisfied.end());    
    
    //push start directory
    to_process.push(std::move(start_directory));
    enqueued.insert(to_process.front());

    //until everything processed or unsatisfied
    while (!to_process.empty() && (unsatisfied.empty() || !need.empty())) {
        auto dir = to_process.front();
        auto scnres = ModuleResolver::loadMap(dir);
        to_process.pop();

        //try to find origin - or create
        auto orgite = _originMap.find(scnres.env.config_file);
        POriginEnv origin = orgite == _originMap.end()?std::make_shared<OriginEnv>(scnres.env):orgite->second;
            
        //process all files
        for (const auto &f: scnres.files) {
            PSource sinfo = find(f);
            //scan only if new file or rescan scheduled
            if (!sinfo || sinfo->state.rescan) {       
                //rescan it, but dont discaver         
                auto refs = rescan_file(origin, f,   compiler);
                //include unsatisfied
                for (auto &r: refs) need.insert(std::move(r));                
            }
        }
        //erase satisfied references
        for (auto iter = need.begin(); iter != need.end();) {
            PSource sinfo = find(*iter);
            if (sinfo && !sinfo->state.rescan) {
                iter = need.erase(iter);
            } else {
                ++iter;
            }
        }
        //process prefixes and add for processing
        for (auto &itm: need) {
            for (auto &[pfx, dirs]: scnres.mapping) {
                if (ModuleResolver::match_prefix(pfx, itm.name)) {
                    for (const auto &d: dirs) {
                        if (enqueued.insert(d).second) {
                            to_process.push(d);
                        }
                    }
                }
            }
        }
    }
    //return still unsatisfied
    return {need.begin(), need.end()};
}

template<typename FnRanged>
void ModuleDatabase::collect_bmi_references(PSource from, FnRanged &&ret) const {
    std::unordered_set<PSource> result;
    std::queue<PSource> q;
    for (const auto &r: from->references) {
        auto f = find(r);
        if (f) {
            if (result.emplace(f).second) q.push(f);
        }
    }
    while (!q.empty()) {
        auto f= std::move(q.front());
        q.pop();
        for (const auto &r: f->exported) {
            auto ef = find(r);
            if (ef) {
                if (result.emplace(ef ).second) q.push(ef);
            }
        }
    }
    result.erase(from);
    ret(result.begin(), result.end());    

}

template<typename FnRanged>
void ModuleDatabase::transitive_closure(PSource from, FnRanged &&ret) const {
    std::unordered_set<PSource> result;
    std::queue<PSource> q;
    result.insert(from);
    q.push(from);
    while (!q.empty()) {
        auto c = std::move(q.front());
        q.pop();
        for (auto &r: c->references) {
            auto rp = find(r);
            if (rp) {
                if (result.insert(rp).second) {
                    q.push(rp);
                }
            }
            auto impl = find_multi({ModuleType::implementation, r.name});
            for (const auto &f: impl) {
                if (result.insert(f).second) {
                    q.push(f);
                }
            }
        }
    }
    result.erase(from);
    ret(result.begin(), result.end());
}

BuildPlan<ModuleDatabase::CompileAction> ModuleDatabase::create_build_plan(
    AbstractCompiler &compiler, const OriginEnv &env, 
    std::span<const CompileTarget> targets,
    bool recompile, bool build_library) const
{
    auto getenv = [&](const PSource &src) -> const OriginEnv & {
        return src->origin?*src->origin:env;
    };
    auto ncompiled = [&](const PSource &src) ->std::string {
        return "Compiled: " + src->source_file.string();
    };
    auto nlinked = [&](const std::filesystem::path &n) ->std::string {
        return "Linked: " + n.string();
    };

    if (build_library) throw std::runtime_error("library mode is not supported yet!");
    BuildPlan<CompileAction> plan;
    std::unordered_map<PSource, BuildPlan<CompileAction>::TargetID> target_ids;
    std::queue<PSource> to_process;

    std::vector<PSource> tmp;


    //add link steps for all targets
    for (const auto &[t, s]: targets) {
        PSource sinfo = find(s);
        if (sinfo) {
            //collect all references 
            transitive_closure(sinfo, [&](auto beg, auto end) {
                                    tmp.insert(tmp.end(), beg, end);});

            tmp.push_back(sinfo); //include self to dependencies for link step

            CompileAction::LinkStep lnk{{},t};
            for (const PSource &s: tmp) {
                //filter only sources which generates objects
                if (generates_object(s->type)) {
                    lnk.first.push_back(s);
                    //test for need recompile, if need, create targets
                    if (s->state.recompile || s->object_path.empty() || !std::filesystem::exists(s->object_path)) {
                        auto ref = plan.create_target({*this, compiler, getenv(s), s},ncompiled(s));
                        target_ids.emplace(s, ref);
                        //add to process this target
                        to_process.push(s);
                    }
                }
            }
            //add link step target
            auto ref = plan.create_target({*this, compiler, getenv(sinfo), std::move(lnk)},nlinked(t));
            //add dependencies for this target
            for (const PSource &s: tmp) {
                auto iter = target_ids.find(s);
                if (iter != target_ids.end()) plan.add_dependency(ref, iter->second);
            }
        }
    }

    ///when flag recompile - recompile everything marked as recompile
    if (recompile) {
        for (const auto &[_,f]: _fileIndex) {
            if (f->state.recompile) {
                auto iter = target_ids.find(f);
                if (iter == target_ids.end())  {
                    auto t = plan.create_target({*this, compiler, getenv(f), f},ncompiled(f))    ;
                    target_ids.emplace(f, t);
                    to_process.push(f);
                }            
            }
        }
    }

    //now process all created targets
    while (!to_process.empty()) {
        //pick first
        auto f = std::move(to_process.front());
        to_process.pop();
        //find target id
        auto tid = target_ids[f];
        //clear temporary array
        //tmp.clear();
        //collect all bmis required for this target (direct and reexports)
        collect_bmi_references(f, [&](auto beg, auto end){
            //process all bmi
            for (const auto &s: std::ranges::subrange(beg, end)) {
                //find whether we already know this target
                auto iter = target_ids.find(s);
                //we don't. create it
                if (iter == target_ids.end()) {
                    //test whether this is bmi
                    //and file marked as recompile or has empty bmi path or the file doesn't exists
                    if (generates_bmi(s->type) && (s->state.recompile 
                            || s->bmi_path.empty() 
                            || !std::filesystem::exists(s->bmi_path))) {
                        //create target
                        auto ref = plan.create_target({*this, compiler, getenv(s), s},ncompiled(s));                    
                        target_ids.emplace(s,ref);
                        to_process.push(s);
                        //add to dependency
                        plan.add_dependency(tid, ref);
                    }
                } else {
                    //add to dependency
                    plan.add_dependency(tid, iter->second);
                }
            }
        });
    }
    return plan;
}


auto ModuleDatabase::CompileAction::get_references(const PSource &f) const {
    std::vector<SourceDef> references;
    db.collect_bmi_references(f, [&](auto beg, auto end){
        for (auto iter = beg; iter != end;++iter) {
            PSource s = *iter;
            references.push_back({
                s->type, s->name, s->bmi_path
            });   
        }
    });
    return references;
}

bool ModuleDatabase::CompileAction::operator()() const noexcept 
{
    try {
        if (std::holds_alternative<PSource>(step)) {
            const PSource &f = std::get<PSource>(step);
            AbstractCompiler::CompileResult result;
            int res = compiler.compile(env, {f->type, f->name, f->source_file}, get_references(f), result);
            f->bmi_path = result.interface;
            f->object_path = result.object;
            db.set_dirty();
            return res == 0;
        } else {
            const LinkStep  &lnk = std::get<LinkStep>(step);
            std::vector<std::filesystem::path> objs;
            objs.reserve(lnk.first.size());
            for (const auto &f: lnk.first) objs.push_back(f->object_path);
            int res = compiler.link(objs, lnk.second);
            return res == 0;
        }
    } catch (std::exception &e) {
        Log::error("EXCEPTION: {}", e.what());
        return false;
    }
}




void ModuleDatabase::CompileAction::add_to_cctable(CompileCommandsTable &cctable) const
{
    if (std::holds_alternative<PSource>(step)) {
        const PSource &f = std::get<PSource>(step);
        std::vector<ArgumentString> result;
        compiler.generate_compile_command(env, {f->type, f->name, f->source_file}, get_references(f), result);
        cctable.update(cctable.record(env.working_dir, f->source_file, result));
    } 
}

template<>
void ModuleDatabase::extract_module_mapping(const BuildPlan<CompileAction> &plan, std::vector<AbstractCompiler::ModuleMapping> &out) {
    out.clear();
    std::unordered_set<Reference, MethodHash> processed;
    for (const auto &itm: plan) {
        if (std::holds_alternative<PSource>(itm.action.step)) {
            const PSource &f = std::get<PSource>(itm.action.step);            
            if (generates_bmi(f->type) && !is_header_module(f->type)) {
                if (processed.insert({f->type, f->name}).second) {
                    out.push_back({SourceDef{f->type, f->name, f->source_file}, 
                        f->origin?f->origin->working_dir:std::filesystem::path()});
                }
            }
            
            itm.action.db.collect_bmi_references(f, [&](auto beg, auto end){
                for (const auto &f: std::ranges::subrange(beg,end)) {
                    if (processed.insert({f->type, f->name}).second) {
                        out.push_back({SourceDef{f->type, f->name, f->source_file}, 
                            f->origin?f->origin->working_dir:std::filesystem::path()});                        
                    }                
                }
            });
        }
    }

}
