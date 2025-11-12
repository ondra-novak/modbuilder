#include "module_database.hpp"
#include <atomic>
#include "module_resolver.hpp"
#include "module_type.hpp"
#include "scanner.hpp"
#include "utils/log.hpp"
#include "utils/filesystem.hpp"
#include <queue>
#include <unordered_set>

json::value ModuleDatabase::export_db() const {
    auto refjson = [](const Reference &ref) {
                    return json::value{
                        {"type", static_cast<int>(ref.type)},
                        {"name",ref.name},
                    };
    };
    std::unordered_map<POriginEnv, int> orgmap;
    std::vector<POriginEnv> origins;
    origins.reserve(_originMap.size()+1);
    json::value data(_fileIndex.begin(), _fileIndex.end(), [&](const auto &kv){
        const PSource src = kv.second;
        std::size_t orgid = 0;
        auto oins = orgmap.try_emplace(src->origin, origins.size());
        if (oins.second) origins.push_back(src->origin); else orgid = oins.first->second;

        return json::value {
            {"source_file",src->source_file.u8string()},
            {"object_path",src->object_path.u8string()},
            {"bmi_path",src->bmi_path.u8string()},
            {"name",src->name},
            {"type",static_cast<int>(src->type)},
            {"references",json::value(src->references.begin(), src->references.end(),refjson)},
            {"exported", json::value(src->exported.begin(), src->exported.end(), refjson)},
            {"origin", orgid}
        };
    });

    return {
        {"timestamp", _import_time.time_since_epoch().count()},
        {"data", data},
        {"origins", json::value(origins.begin(), origins.end(), [](const POriginEnv &org){
            if (org) return json::value {
                {"config_file", org->config_file.u8string()},
                {"work_dir", org->working_dir.u8string()},
                {"hash", org->settings_hash},
                {"includes", json::value(org->includes.begin(), org->includes.end(), 
                    [](const std::filesystem::path &p)->json::value_t{return p.u8string();})},
                {"options", json::value(org->options.begin(), org->options.end())}
            }; else return json::value(nullptr);
        })}
    };
}

void ModuleDatabase::import_db(json::value db) {
    bool d =is_dirty();
    json::value data = db["data"];
    json::value tm = db["timestamp"];
    json::value origins = db["origins"];
    std::vector<POriginEnv> origmap(origins.size());
    std::transform(origins.begin(),origins.end(), origmap.begin(), [](const json::value_t &v){
        if (v.type() == json::type::null) return POriginEnv();
        else {
            auto inc = v["includes"];
            auto opts = v["options"];

            std::vector<std::filesystem::path> paths(inc.size());
            std::vector<std::string> options(inc.size());

            std::transform(inc.begin(), inc.end(), paths.begin(), [](const json::value &v){
                return std::filesystem::path(v.as<std::u8string>());
            });
            std::transform(opts.begin(), opts.end(), options.begin(), [](const json::value &v){
                return v.as<std::string>();
            });
            return std::make_shared<OriginEnv>(OriginEnv{
            std::filesystem::path(v["config_file"].as<std::u8string_view>()),
            std::filesystem::path(v["work_dir"].as<std::u8string>()),
            v["hash"].as<std::size_t>(),
            std::move(paths),
            std::move(options)});
        }
    });
    

    auto json2ref =  [&](const json::value &val) {
                return Reference{
                    static_cast<ModuleType>(val["type"].as<int>()),
                    val["name"].as<std::string>()
                };
    };

    

    for (auto item: data) {
        Source src;
        src.name = data["name"].as<std::string>();
        src.source_file = data["source_file"].as<std::u8string>();
        src.object_path = data["object_path"].as<std::u8string>();
        src.bmi_path = data["bmi_path"].as<std::u8string>();        
        src.type = static_cast<ModuleType>(data["type"].as<int>());
        std::size_t orgid = data["origin"].as<std::size_t>();
        if (orgid < origmap.size()) src.origin = origmap[orgid];
        auto ref = data["references"];
        std::transform(ref.begin(), ref.end(), std::back_inserter(src.references), json2ref);
        auto expr = data["exported"];
        std::transform(expr.begin(), expr.end(), std::back_inserter(src.exported), json2ref);
        put(std::move(src));
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
    refiter.first->second.push_back(std::move(psrc));
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

void ModuleDatabase::clear_dirty() {
    _modified.store(false, std::memory_order_relaxed);
}

void ModuleDatabase::set_dirty() {
    _modified.store(true, std::memory_order_relaxed);

}

void ModuleDatabase::update_files_state(AbstractCompiler &compiler) {
    std::vector<std::filesystem::path> to_remove;
    auto cmptm = std::chrono::clock_cast<std::filesystem::file_time_type::clock>(_modify_time);

    auto need_rescan = [](ModuleType type) {
        return type != ModuleType::user_header && type == ModuleType::system_header;
    };

    //locate all modified files    
    for(const auto &[k,f]: _fileIndex) {
        std::error_code ec;
        auto lwt = std::filesystem::last_write_time(k, ec);
        if (ec != std::error_code{}) {
            //fail to retrieve time - remove this file
            to_remove.push_back(k);        
        } else {
            //recompile if modified
            f->state.recompile = lwt > cmptm;
            //rescan if modified and not header
            f->state.rescan = f->state.recompile && need_rescan(f->type);    
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
                        f->state.rescan = need_rescan(f->type);
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
  
    auto reftype = [](std::string_view name) {
        return name.find(':') != name.npos?ModuleType::partition:ModuleType::interface;
    };

    Source out;
    out.name = nfo.name;
    out.type = nfo.type;
    for (const auto &r: nfo.required) {
        out.references.push_back(Reference{reftype(r), r});
    }
    for (const auto &r: nfo.exported) {
        out.exported.push_back(Reference{reftype(r), r});
    }
    for (const auto &r: nfo.system_headers) {
        out.references.push_back(Reference{ModuleType::system_header, r});
    }
    for (const auto &r: nfo.user_headers) {
        out.references.push_back(Reference{ModuleType::user_header, source_file.parent_path()/r});
    }
    out.source_file = source_file;
    return out;
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
    SourceScanner scn(compiler);
    auto info = scn.scan_file(*orgptr, source_file);    
    Source srcinfo = from_scanner(source_file, info);    
    srcinfo.origin = origin;
    auto refs = srcinfo.references;
    //put new registration
    put(std::move(srcinfo));

    //register header references
    for (auto &r: refs) {
        if (r.type == ModuleType::system_header || r.type == ModuleType::user_header) {
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
    while (!to_process.empty() && !need.empty()) {
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



std::vector<ModuleDatabase::CompilePlan> ModuleDatabase::create_compile_plan(const std::filesystem::path &source_file) const {    

    //collect all required files transitive

    std::unordered_set<PSource> all_files;
    std::vector<CompilePlan> out;
    auto iter = find(source_file);
    if (!iter) return out;
    all_files.insert(iter);
    std::queue<PSource> q;
    q.push(iter);
    while (!q.empty()) {
        iter = std::move(q.front());
        q.pop();
        for (auto &r: iter->references) {
            auto fs = find_multi(r);
            for (auto f: fs) {
                if (all_files.insert(f).second) {
                    q.push(std::move(f));
                }
            }
            //include all known implementations
            if (r.type == ModuleType::interface) {
                auto imps = find_multi(Reference{ModuleType::implementation,r.name});
                for (auto &i: imps) {
                    if (all_files.insert(i).second) {
                        q.push(std::move(i));
                    }
                }
            }
        }
    }
    
    //all files collected, let's build the plan

    for (auto &f: all_files) {
        CompilePlan c;
        c.sourceInfo = f;
        for (auto &r: c.sourceInfo->references) {
            auto g= find(r);
            if (g) {
                c.references.push_back(g);
                collectReexports(g, c.references);
            }
        }
        out.push_back(std::move(c));
    }

    return out;
}

std::vector<ModuleDatabase::CompilePlan> ModuleDatabase::create_recompile_plan() const
{
    std::vector<CompilePlan> out;
    for (auto &[_,f]: _fileIndex) {
        CompilePlan c;
        c.sourceInfo = f;
        for (auto &r: c.sourceInfo->references) {
            auto g= find(r);
            if (g) {
                c.references.push_back(g);
                collectReexports(g, c.references);
            }
        }
        out.push_back(std::move(c));
    }
    return out;

}

void ModuleDatabase::collectReexports(PSource src,
                                      std::vector<PSource> &exports) const {
    for (auto &r: src->exported) {
        auto f = find(r);
        if (f) {
            exports.push_back(f);            
            auto iter = std::find(exports.begin(), exports.end(), f);
            if (iter == exports.end()) {
                exports.push_back(f);
                collectReexports(f, exports);
            }
        }
    }

}
