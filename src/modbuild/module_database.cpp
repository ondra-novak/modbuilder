#include "module_database.hpp"
#include <atomic>
#include <mutex>
#include "module_resolver.hpp"
#include "utils/log.hpp"
#include "utils/filesystem.hpp"
#include <queue>
#include <unordered_set>

json::value ModuleDatabase::export_db() const {
    std::shared_lock _(_mx);
    auto refjson = [](const Reference &ref) {
                    return json::value{
                        {"type", static_cast<int>(ref.type)},
                        {"name",ref.name},
                    };
    };
    json::value data(_fileIndex.begin(), _fileIndex.end(), [&](const auto &kv){
        const PSource src = kv.second;
        return json::value {
            {"source_file",src->source_file.u8string()},
            {"object_path",src->object_path.u8string()},
            {"bmi_path",src->bmi_path.u8string()},
            {"name",src->name},
            {"type",static_cast<int>(src->type)},
            {"references",json::value(src->references.begin(), src->references.end(),refjson)},
            {"exported", json::value(src->exported.begin(), src->exported.end(), refjson)},
            {"origin", src->origin.u8string()}
        };
    });

    return {
        {"timestamp", _import_time.time_since_epoch().count()},
        {"data", data}
    };
}

void ModuleDatabase::import_db(json::value db) {
    bool d =is_dirty();
    json::value data = db["data"];
    json::value tm = db["timestamp"];
    std::unique_lock _(_mx);

    auto json2ref =              [&](const json::value &val) {
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
        src.origin = data["origin"].as<std::u8string>();
        src.type = static_cast<ModuleType>(data.as<int>());
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
    std::unique_lock _(_mx);
    _fileIndex.clear();
    _moduleIndex.clear();
    _modify_time = {};
    _import_time = std::chrono::system_clock::now();
    _modified = false;
}

ModuleDatabase::PSource ModuleDatabase::find(Reference ref) const { 

    std::shared_lock _(_mx);
    auto iter = _moduleIndex.find(Reference{ref});
    if (iter == _moduleIndex.end()) return {};
    return iter->second.front();

}

ModuleDatabase::PSource ModuleDatabase::find(std::filesystem::path file) const {
      std::shared_lock _(_mx);
    auto iter = _fileIndex.find(file);
    if (iter == _fileIndex.end()) return {};
    return iter->second;

}

ModuleDatabase::PSource ModuleDatabase::put(Source src) {

  std::unique_lock _(_mx);
  auto iter = _fileIndex.find(src.source_file);
  if (iter != _fileIndex.end())
    return iter->second;;

  PSource psrc = std::make_unique<Source>(src);
  _fileIndex.emplace(psrc->source_file, psrc);
  auto refiter = _moduleIndex.try_emplace(Reference{src.type, src.name}, 1, psrc);
  if (!refiter.second) {
    refiter.first->second.push_back(std::move(psrc));
  }
  set_dirty();
  return {};
}

void ModuleDatabase::erase(std::filesystem::path file) {
    std::unique_lock _(_mx);
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

std::vector<ModuleDatabase::PSource> ModuleDatabase::get_all_imps(std::string module_name) const {
    std::shared_lock _(_mx);
    auto iter = _moduleIndex.find(Reference{ModuleType::implementation, std::move(module_name)});
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

void ModuleDatabase::update_files_state() {
    std::unique_lock _(_mx);
    std::vector<std::filesystem::path> to_remove;
    auto cmptm = std::chrono::clock_cast<std::filesystem::file_time_type::clock>(_modify_time);

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
            f->state.rescan = f->state.recompile &&
                            (f->type != ModuleType::user_header && f->type == ModuleType::system_header);
        }
    }
    //remove all marked files
    for (const auto &x: to_remove) erase(x);    

    //spread recompile flag to other files (by reference)
    bool mod = true;
    while (mod) {
        mod = false;
        for(const auto &[_,f]: _fileIndex) {
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
    }
}

void ModuleDatabase::discover_new_files(AbstractCompiler &compiler) {
    std::unordered_set<std::filesystem::path> origins;
    for (auto &x: _fileIndex) {
        if (x.second->type == ModuleType::interface) {
            origins.insert(x.second->origin);
        }
    }
    auto tr= std::chrono::clock_cast<std::filesystem::file_time_type::clock>(_modify_time);
    for (auto &x: origins) {
        std::error_code ec;
        auto wrtm = std::filesystem::last_write_time(x, ec);
        if (ec == std::error_code{} && wrtm >= tr) {
            ModuleResolver rsvl;
            auto info = rsvl.loadMap(x);
            for (auto &f: info.files) {
                PSource sinfo = find(f);
                if (!sinfo || sinfo->state.rescan) {                
                    rescan_file(f, x,  compiler, true);

                }
            }
        }

    }
}



ModuleDatabase::Source ModuleDatabase::from_scanner(const std::filesystem::path &source_file,
                                    const SourceScanner::Info &nfo) {
  
    Source out;
    out.name = nfo.name;
    out.type = nfo.type;
    for (const auto &r: nfo.required) {
        out.references.push_back(Reference{ModuleType::interface, r});
    }
    for (const auto &r: nfo.exported) {
        out.exported.push_back(Reference{ModuleType::interface, r});
    }
    for (const auto &r: nfo.include_a) {
        out.references.push_back(Reference{ModuleType::system_header, r});
    }
    for (const auto &r: nfo.include_q) {
        out.references.push_back(Reference{ModuleType::user_header, source_file.parent_path()/r});
    }
    out.source_file = source_file;
    return out;
}

std::vector<ModuleDatabase::Reference> ModuleDatabase::rescan_file(
        const std::filesystem::path &source_file,
        const std::filesystem::path &origin,
        AbstractCompiler &compiler,
        bool discovery
    ) {
  
    Log::debug("Scanning file: {}", source_file);

    std::vector<ModuleDatabase::Reference> unsatisfied;
  
    erase(source_file);
    SourceScanner scn(compiler);
    auto info = scn.scan_file(source_file);
    Source srcinfo = from_scanner(source_file, info);    
    srcinfo.origin = origin;
    auto refs = srcinfo.references;
    put(std::move(srcinfo));

    for (auto &r: refs) {
        if (r.type == ModuleType::system_header || r.type == ModuleType::user_header) {
            put(Source{r.name, r.type, r.name});            
        } else {
            auto fs = find(r)   ;
            if (!fs || fs->state.rescan)  {
                unsatisfied.push_back(std::move(r));
            }
        }
    }

    if (discovery) unsatisfied = rescan_directories(unsatisfied, compiler, source_file.parent_path());
    return unsatisfied;
}

ModuleDatabase::Unsatisfied ModuleDatabase::rescan_directories(
            std::span<const Reference> unsatisfied, 
            AbstractCompiler &compiler,
            std::filesystem::path start_directory) {
  
    std::queue<std::filesystem::path> to_process;
    std::unordered_set<std::filesystem::path> enqueued;
    std::unordered_set<Reference, MethodHash> need(unsatisfied.begin(), unsatisfied.end());    
    
    to_process.push(std::move(start_directory));
    enqueued.insert(to_process.front());

    while (!to_process.empty() && !need.empty()) {
        auto dir = to_process.front();
        auto scnres = ModuleResolver::loadMap(dir);
        to_process.pop();

        for (const auto &f: scnres.files) {
            PSource sinfo = find(f);
            if (!sinfo || sinfo->state.rescan) {                
                auto refs = rescan_file(f, scnres.origin,  compiler, false);
                for (auto &r: refs) need.insert(std::move(r));                
            }
        }
        for (auto iter = need.begin(); iter != need.end();) {
            PSource sinfo = find(*iter);
            if (sinfo && !sinfo->state.rescan) {
                iter = need.erase(iter);
            } else {
                ++iter;
            }
        }
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
    return {need.begin(), need.end()};
}



std::vector<ModuleDatabase::CompilePlan> ModuleDatabase::create_compile_plan(const std::filesystem::path &source_file) const {
    std::shared_lock _(_mx);

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
            auto f = find(r);
            if (f) {
                if (all_files.insert(f).second) {
                    q.push(std::move(f));
                }
            }
            //include all known implementations
            if (r.type == ModuleType::interface) {
                auto imps = get_all_imps(r.name);
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

void ModuleDatabase::collectReexports(PSource src,
                                      std::vector<PSource> &exports) const {
    for (auto &r: src->exported) {
        auto f = find(r);
        if (f) {
            auto iter = std::find(exports.begin(), exports.end(), f);
            if (iter != exports.end()) {
                exports.push_back(f);
                collectReexports(f, exports);
            }
        }
    }

}
