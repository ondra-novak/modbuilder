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
#include <string_view>
#include <unordered_set>
#include <ranges>

json::value export_module_map(const ModuleMap &mp) {
    return json::value(mp.begin(), mp.end(), [](const auto &item) {
        return json::key_value_t(
            item.prefix, json::value(item.paths.begin(), item.paths.end(), [](const std::filesystem::path &p){
                return json::value(p.u8string());
            })
        );
    });
}

ModuleMap import_module_map(const json::value &val) {
    ModuleMap ret;
    ret.reserve(val.size());
    for (const auto &v: val) {
        const std::string_view k = v.key();
        std::vector<std::filesystem::path> lst;
        lst.reserve(v.size());
        for (const auto &w: v) {
            lst.push_back(std::filesystem::path(w.as<std::u8string_view>()));
        }
        ret.push_back(ModuleMapItem{std::string(k), std::move(lst)});
    }
    return ret;
}



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
                {"map",export_module_map(org->maps)},
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
        auto map = import_module_map(v["map"]);
        auto org =  std::make_shared<OriginEnv>(OriginEnv{
            std::filesystem::path(v["config_file"].as<std::u8string_view>()),
            std::filesystem::path(v["work_dir"].as<std::u8string>()),
            v["hash"].as<std::size_t>(),
            std::move(paths),
            std::move(options),
            std::move(map)});
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




void ModuleDatabase::check_for_modifications(AbstractCompiler &compiler) {

    auto cmptm = std::chrono::clock_cast<std::filesystem::file_time_type::clock>(_modify_time);

    // check all origins - modified origins will be removed including their files
    std::vector<std::filesystem::path> to_remove;
    for (const auto &[p, org] : _originMap) {
        if (ModuleResolver::detect_change(*org, cmptm)) {
            to_remove.push_back(p);
        }
    }
    for (const auto &p: to_remove) _originMap.erase(p);
    to_remove.clear();

    //collect all missing references
    std::vector<Reference> missing;


    std::vector<PSource> rescan;

    // check all files of removed origins
    //also check, whether files has been removed
    //because they need to be rescaned, it is better to remove them from the database all together
    for (const auto &[p, f] : _fileIndex) {
        auto iter = _originMap.find(f->origin->config_file);
        if (iter == _originMap.end()) {
            rescan.push_back(f);
        } else {
            auto st = compiler.source_status(f->type, p, cmptm);    
            if (st != AbstractCompiler::SourceStatus::not_modified) {
                rescan.push_back(f);
            }
        }
    }

    for (const auto &f: rescan) {
        if (!is_header_module(f->type)) {
            missing = merge_references(std::move(missing), rescan_file(f->origin, f->source_file, compiler));
        } else {
            f->state.recompile = true;
        }
    }

    for (const auto &[p, f] : _fileIndex) {
        for (auto &r: f->references) {
            if (find(r) == nullptr) {
                auto iter = std::lower_bound(missing.begin(), missing.end(), r);
                if (iter == missing.end()) missing.push_back(r);
                else if (*iter != r) missing.insert(iter, r);
            }
        }
    }
    run_discovery(missing, compiler);
}

void ModuleDatabase::check_for_recompile() {
    //spread recompile flag to other files (by reference)
    //modified?
    bool mod = true;
    //repeat
    while (mod) {
        //nothing modified
        mod = false;
        //process all files
        for(const auto &[_,f]: _fileIndex) {
            //spread recompile flag
            if (!f->state.recompile) {
            //check whether targets exist
                if ((generates_bmi(f->type) && f->bmi_path.empty()) || (generates_object(f->type) && f->object_path.empty())) {
                    f->state.recompile = true;
                    mod = true;
                } else  for (const auto &r: f->references) {
                    auto rf = find(r);
                    if (rf && rf->state.recompile) { 
                        f->state.recompile = true;
                        mod = true;
                        break;
                    }
                }
            }
        }
    }
}

void ModuleDatabase:: recompile_all() {
    for (const auto &[_,f]: _fileIndex) {
        f->state.recompile = true;
    }
}

ModuleDatabase::Unsatisfied ModuleDatabase::merge_references(Unsatisfied a1, Unsatisfied a2) {    
    Unsatisfied out;
    if (a1.empty()) out = std::move(a2);
    else if (a2.empty()) out = std::move(a1);
    else {
        out.reserve(a1.size()+a2.size());
        std::set_union(a1.begin(), a1.end(), a2.begin(), a2.end(), std::back_inserter(out));
    }
    return out;
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

std::pair<POriginEnv,bool> ModuleDatabase::add_origin_no_discovery(const std::filesystem::path &origin_path, AbstractCompiler &compiler, Unsatisfied &missing) {
    

    ModuleResolver::Result mres;
    bool loaded = false;
    //attempt to find origin to this path
    //in database
    auto iter =  _originMap.find(origin_path);
    if (iter == _originMap.end()) {
        //indirecty
        mres = ModuleResolver::loadMap(origin_path);
        iter = _originMap.find(mres.env.config_file);       
        loaded = true;
    }

    if (iter == _originMap.end()) {
        //really don't known, create it
        POriginEnv env = std::make_shared<OriginEnv>(mres.env);
        //add to known origins
        _originMap.emplace(env->config_file, env);
        //load allo files
        for (auto &x: mres.files) {
            //rescan this file and find unknown references
            missing = merge_references(std::move(missing),
                                 rescan_file(env, x, compiler));
        }
        return {env, true};
    }  else {
        POriginEnv o = iter->second;
        if (!loaded) {
            mres = ModuleResolver::loadMap(origin_path);
        }
        for (const auto &f: mres.files) {
            if (!find(f)) {
                missing = merge_references(missing, rescan_file(o, f, compiler));
            }
        }


        return {o, false};
    }
}


void ModuleDatabase::run_discovery(Unsatisfied &missing_ordered, AbstractCompiler &compiler) {
    std::queue<std::filesystem::path> to_explore;
    std::unordered_set<std::filesystem::path> explored;
    while (true) {
        missing_ordered.erase(std::remove_if(missing_ordered.begin(), missing_ordered.end(),[&](const Reference &ref){
            return !!find(ref);
        }), missing_ordered.end());

        for (auto &m: missing_ordered) {
            for (const auto &[_,o]: _originMap) {
                for (const auto &[pfx, paths]: o->maps) {
                    if (ModuleResolver::match_prefix(pfx,m.name)) {
                        for (const auto &p: paths) {
                            if (explored.insert(p).second) {
                                //attempt to spread searching
                                to_explore.push(p);
                            }
                        }
                    }
                }
            }
        }

        if (to_explore.empty() || missing_ordered.empty()) break;
        //adds files to database, but can extend dependencies
         add_origin_no_discovery(to_explore.front(), compiler, missing_ordered);
        to_explore.pop();
    }
}
POriginEnv ModuleDatabase::add_origin(const std::filesystem::path &origin_path, AbstractCompiler &compiler) {

    Unsatisfied missing;
    auto r = add_origin_no_discovery(origin_path, compiler, missing);
    run_discovery(missing, compiler);
    return r.first;
}

bool ModuleDatabase::add_file(const std::filesystem::path &source_file, AbstractCompiler &compiler) {
    auto f = find(source_file);    
    if (f) return false;
    POriginEnv env = add_origin(source_file.parent_path(), compiler);
    Unsatisfied missing =  rescan_file(env, source_file, compiler);
    run_discovery(missing, compiler);
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
    srcinfo.state.recompile = true;
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
            if (!fs)  {
                unsatisfied.push_back(std::move(r));
            }
        }
    }
    std::sort(unsatisfied.begin(), unsatisfied.end());
    return unsatisfied;
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
            } else {
                Log::error("Reference not found in database: {}", r.name);
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
            for (const PSource &ss: tmp) {
                //filter only sources which generates objects
                if (generates_object(ss->type)) {
                    lnk.first.push_back(ss);
                    //test for need recompile, if need, create targets
                    if (ss->state.recompile || ss->object_path.empty() || !std::filesystem::exists(ss->object_path)) {
                        auto ref = plan.create_target({*this, compiler, getenv(ss), ss},ncompiled(ss));
                        target_ids.emplace(ss, ref);
                        //add to process this target
                        to_process.push(ss);
                    }
                }
            }
            //add link step target
            auto ref = plan.create_target({*this, compiler, getenv(sinfo), std::move(lnk)},nlinked(t));
            //add dependencies for this target
            for (const PSource &ss: tmp) {
                auto iter = target_ids.find(ss);
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
