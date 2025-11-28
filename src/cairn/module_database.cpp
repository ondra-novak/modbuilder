module cairn.module_database;

import cairn.module_type;
import cairn.module_resolver;
import cairn.source_scanner;
import cairn.source_def;
import cairn.utils.arguments;
import cairn.utils.hash;
import cairn.utils.log;
import cairn.abstract_compiler;
import cairn.compile_commands;
import cairn.utils.serializer;
import cairn.utils.serializer.rules;

import <atomic>;
import <ostream>;
import <queue>;
import <unordered_set>;
import <ranges>;
import <format>;



void ModuleDatabase::clear() {
    _fileIndex.clear();
    _moduleIndex.clear();
    _originMap.clear();    
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
            Log::debug("{} - changed origin", [&]{return p.string();});
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
            to_remove.push_back(p);
            Log::debug("{} - removed file because origin", [&]{return p.string();});
        } else {
            auto st = compiler.source_status(f->type, p, cmptm);    
            if (st != AbstractCompiler::SourceStatus::not_modified) {
                Log::debug("{} - modified", [&]{return p.string();});
                rescan.push_back(f);
            }
        }
    }

    for (const auto &p: to_remove) _fileIndex.erase(p);

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
                    Log::debug("{} - missing one of its products - scheduled for recompile", [&]{return f->source_file.string();});

                    mod = true;
                } else  for (const auto &r: f->references) {
                    auto rf = find(r);
                    if (rf && rf->state.recompile) { 
                        f->state.recompile = true;
                        Log::debug("{} - depends on recompiled file - scheduled for recompile", [&]{return f->source_file.string();});
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

POriginEnv ModuleDatabase::add_origin_no_discovery(const ModuleResolver::Result &origin, AbstractCompiler &compiler, Unsatisfied &missing) {

    //attempt to find origin to this path
    //in database
    auto iter =  _originMap.find(origin.env.config_file);

    if (iter == _originMap.end()) {
        //really don't known, create it
        POriginEnv env = std::make_shared<OriginEnv>(origin.env);
        //add to known origins
        _originMap.emplace(env->config_file, env);
        //load allo files
        for (auto &x: origin.files) {
            //rescan this file and find unknown references
            missing = merge_references(std::move(missing),
                                 rescan_file(env, x, compiler));
        }
        return env;
    }  else {
        POriginEnv o = iter->second;
        for (const auto &f: origin.files) {
            if (!find(f)) {
                missing = merge_references(missing, rescan_file(o, f, compiler));
            }
        }


        return o;
    }

}

POriginEnv ModuleDatabase::add_origin_no_discovery(const std::filesystem::path &origin_path, AbstractCompiler &compiler, Unsatisfied &missing) {


    ModuleResolver::Result mres = ModuleResolver::loadMap(origin_path);
    return add_origin_no_discovery(mres, compiler, missing);
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

        auto &front = to_explore.front();
        Log::debug("Running discovery for {} . Missing: {}", [&]{return front.string();},
            [&]{
                char sep = ' ';
                std::ostringstream s;
                for (auto &x: missing_ordered) {
                    s << sep << " " << x.name ;
                    sep = ',';
                };
                return std::move(s).str();
            });

        //adds files to database, but can extend dependencies
         add_origin_no_discovery(front, compiler, missing_ordered);
        to_explore.pop();
    }
}
POriginEnv ModuleDatabase::add_origin(const ModuleResolver::Result &origin, AbstractCompiler &compiler) {
    Unsatisfied missing;
    auto r = add_origin_no_discovery(origin, compiler, missing);
    run_discovery(missing, compiler);
    return r;

}

bool ModuleDatabase::add_file(const std::filesystem::path &source_file, AbstractCompiler &compiler) {
    auto f = find(source_file);    
    if (f) return false;
    auto parent =source_file.parent_path();
    POriginEnv env;
    auto iter = _originMap.find(parent);
    if (iter != _originMap.end()) {
        env = iter->second;
    } else {
        auto r = ModuleResolver::loadMap(source_file.parent_path());
        env = add_origin(r, compiler);
    }
    Unsatisfied missing =  rescan_file(env, source_file, compiler);
    run_discovery(missing, compiler);
    return true;
}

ModuleDatabase::Unsatisfied ModuleDatabase::rescan_file(
        POriginEnv origin,
        const std::filesystem::path &source_file,
        AbstractCompiler &compiler
    ) {
  
    Log::debug("Scanning file: {}", [&]{return source_file.string();});

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
        } else {
            Log::error("Reference {} not found in database", r.name);
        }
    }
    while (!q.empty()) {
        auto f= std::move(q.front());
        q.pop();
        for (const auto &r: f->exported) {
            auto ef = find(r);
            if (ef) {
                if (result.emplace(ef ).second) q.push(ef);
            } else {
                Log::error("Reference {} not found in database", r.name);
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
            std::unordered_set<std::filesystem::path> objs;
            objs.reserve(lnk.first.size());
            for (auto &f: lnk.first) {
                if (!objs.insert(f->object_path).second) {
                    for (auto &g: lnk.first) if (g->object_path == f->object_path) {
                            Log::warning("Duplicate object file: {} for {} origin {}", 
                                g->object_path.string(), 
                                g->source_file.string(), 
                                g->origin->config_file.string());
                    }
                }
            }
            auto objs_vec = std::vector(objs.begin(), objs.end());
            int res = compiler.link(objs_vec, lnk.second);
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
        compiler.update_compile_commands(cctable, env, {f->type, f->name, f->source_file}, get_references(f));
    } else if (std::holds_alternative<LinkStep>(step)) {
        const LinkStep &lnk = std::get<LinkStep>(step);
        std::unordered_set<std::filesystem::path> objs;
        objs.reserve(lnk.first.size());
        for (auto &f: lnk.first) {
            objs.insert(f->object_path);
        }
        auto objs_vec = std::vector(objs.begin(), objs.end());
        compiler.update_link_command(cctable, objs_vec, lnk.second);
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


void ModuleDatabase::export_database(std::ostream &s) const {
    serialize_to_stream(s, *this);
}

bool ModuleDatabase::check_database_version(const std::filesystem::path &compiler, std::span<const ArgumentString> arguments) {
    std::hash<std::filesystem::path> hsh1;
    std::hash<ArgumentString> hsh2;
    auto h = hsh1(compiler);
    for (const auto &a: arguments) {
        h = hash_combine(h, hsh2(a));
    }
    if (_hash_settings != h) {
        clear();
        _hash_settings = h;
        return false;
    }
    return true;
}

void ModuleDatabase::import_database(std::istream &s) {
    clear();
    deserialize_from_stream(s, *this);
}

void ModuleDatabase::update_compile_commands(CompileCommandsTable &cc, AbstractCompiler &compiler) {
    std::vector<SourceDef> modules;
    for (const auto &[p,f]: _fileIndex) {
        modules.clear();
        collect_bmi_references(f, [&](auto beg, auto end){
            for (auto &f: std::ranges::subrange(beg,end)) {
                if (!f->bmi_path.empty()) { //can't handle this case, so skip it 
                    modules.push_back({f->type, f->name, f->bmi_path});
                }
            }            
        });
        compiler.update_compile_commands(cc, *f->origin, 
            {f->type, f->name, f->source_file},
            modules);            
    }
}