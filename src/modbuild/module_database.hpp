#pragma once

#include "module_type.hpp"
#include "scanner.hpp"
#include "utils/hash.hpp"
#include <chrono>
#include <json/value.h>
#include <filesystem>
#include <shared_mutex>
#include <unordered_map>
#include <vector>



class ModuleDatabase {
public:

    struct Reference {
        ModuleType type;
        std::string name;
        constexpr bool operator==(const Reference &other) const  = default;

        constexpr std::size_t hash() const {
            std::hash<std::string> hasher;
            return hash_combine(hasher(name), static_cast<std::size_t>(type));
        }
    };

    struct State {
        bool rescan = false;            ///<this file needs to be rescanned              
        bool recompile = true;         ///<this file needs to be recompiled
    };

    struct Source {
        std::filesystem::path source_file ={};
        ModuleType type = {};
        std::string name = {};
        std::vector<Reference> references = {};
        std::vector<Reference> exported = {};
        std::filesystem::path object_path = {};
        std::filesystem::path bmi_path = {};
        std::filesystem::path origin = {};      //directory/file where file was found
        State state = {};
    };

    using PSource = std::shared_ptr<Source>;

    using FileIndex = std::unordered_map<std::filesystem::path, PSource>;
    using ModuleIndex = std::unordered_map<Reference, std::vector<PSource>, MethodHash >;


    json::value export_db() const;
    void import_db(json::value db);
    void clear();

  
    PSource find(Reference ref) const;
    PSource find(std::filesystem::path file) const;
    ///Inserts src int database
    /**
     * @param src source info
     * @retval nullptr successfuly put
     * @retval valid_pointer conflict, returns pointer to existing item
     * 
     */
    PSource put(Source src);
    void erase(std::filesystem::path file);
    std::vector<PSource> get_all_imps(std::string module_name) const;
    bool is_dirty() const;
    void clear_dirty();
    void set_dirty();

    ///checks whether files are modified and updates their states accordingly
    /** It also populates recompile state to ensure correct dependency */
    void update_files_state();

    ///Create Source from scanner informations
    static Source from_scanner(const std::filesystem::path &source_file, const SourceScanner::Info &nfo);

    using Unsatisfied = std::vector<Reference>;

    ///Rescan simple file
    /**
     * @param source_file path
     * @param compiler compiler to use
     * @param discovery set true if you need to discover all 
     */
    Unsatisfied rescan_file(const std::filesystem::path &source_file,
            const std::filesystem::path &origin, 
            AbstractCompiler &compiler, bool discovery);

    Unsatisfied rescan_directories(std::span<const Reference> unsatisfied, 
            AbstractCompiler &compiler,
            std::filesystem::path start_directory
        );
    ///runs discover process for added files. It creates map of directories from current files
    void discover_new_files(AbstractCompiler &compiler);

    struct CompilePlan {
        PSource sourceInfo; //this file will be compiled
        std::vector<PSource> references;    //this files will be added as reference
    };

    std::vector<CompilePlan> create_compile_plan(const std::filesystem::path &source_file) const;

protected:
    FileIndex _fileIndex;
    ModuleIndex _moduleIndex;
    std::chrono::system_clock::time_point _modify_time; //time when database was modified
    std::chrono::system_clock::time_point _import_time;   //time when database was imported
    std::atomic<bool> _modified;     //database has been modified
    mutable std::shared_mutex _mx;

    void collectReexports(PSource src, std::vector<PSource> &exports) const;


};