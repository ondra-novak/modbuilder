#pragma once

#include "module_type.hpp"
#include "scanner.hpp"
#include "utils/arguments.hpp"
#include "utils/hash.hpp"
#include "origin_env.hpp"
#include "build_plan.hpp"
#include "compile_target.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <iterator>
#include <filesystem>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <variant>

class AbstractCompiler;

class CompileCommandsTable;

class ModuleDatabase {
public:

    static constexpr int file_magic = 0x0042444D;
    static constexpr int file_version_nr = 1;

    struct Reference {
        ModuleType type;
        std::string name;   //user header contains absolute path
        bool operator==(const Reference &other) const  = default;
        std::strong_ordering operator<=>(const Reference &other) const  = default;

        std::size_t hash() const {
            std::hash<std::string> hasher;
            return hash_combine(hasher(name), static_cast<std::size_t>(type));
        }

        template<typename Me, typename Arch>
        static void serialize(Me &me, Arch &arch) {
            arch(me.type, me.name);
        }


    };

    struct State {
        bool recompile = false;         ///<this file needs to be recompiled
        bool rescan = false;            ///<this file must be rescaned
    };



    struct Source {
        std::filesystem::path source_file = {};
        ModuleType type = {};
        std::string name = {};
        POriginEnv origin = {};
        std::vector<Reference> references = {};
        std::vector<Reference> exported = {};
        std::filesystem::path object_path = {};
        std::filesystem::path bmi_path = {};
        State state = {};

        template<typename Me, typename Arch>
        static void serialize(Me &me, Arch &arch) {
            arch(
                me.source_file,me.type,me.name,
                me.origin,me.references,me.exported,
                me.object_path,me.bmi_path
            );
        }
    };


    using PSource = std::shared_ptr<Source>;

    using FileIndex = std::unordered_map<std::filesystem::path, PSource>;
    using ModuleIndex = std::unordered_map<Reference, std::vector<PSource>, MethodHash >;
    using OriginMap = std::unordered_map<std::filesystem::path, POriginEnv>;


    void clear();

    ModuleDatabase() = default;




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
    ///Search multiple candidates
    /** some categories can have multiple candidates, for example implementation or partitions
        @param ref reference
        @return vector of found candidates
    */
    std::vector<PSource> find_multi(Reference ref) const;
    bool is_dirty() const;
    void clear_dirty() const;
    void set_dirty() const;


    ///checks whether files are modified and updates their states accordingly
    /** It also populates recompile state to ensure correct dependency */
    void update_files_state(AbstractCompiler &compiler);

    ///adds new file
    /**
     * @param origin source origin
     * @param source_file source file path
     * @param compiler reference to compiler
     * 
     * @retval true file added or updated
     * @retval false no change made
     * 
     * @note Adds file, if doesn't exist in database or is stored differently
     */
    bool add_file(const std::filesystem::path &source_file, AbstractCompiler &compiler);

    ///Create Source from scanner informations
    static Source from_scanner(const std::filesystem::path &source_file, const SourceScanner::Info &nfo);

    using Unsatisfied = std::vector<Reference>;

    std::pair<POriginEnv,bool> add_origin_no_discovery(const std::filesystem::path &origin_path, AbstractCompiler &compiler, Unsatisfied &missing);
    void run_discovery(Unsatisfied &missing_ordered, AbstractCompiler &compiler);
    POriginEnv add_origin(const std::filesystem::path &origin_path, AbstractCompiler &compiler);
    void rescan_origin(POriginEnv origin);

    ///Rescan simple file
    /**
     * @param origin file's origin. Can be null, if file is inline (command line specified)
     * @param source_file path
     * @param compiler compiler to use
     */
    Unsatisfied rescan_file(POriginEnv origin, const std::filesystem::path &source_file,
            AbstractCompiler &compiler);


    
    struct CompileAction {
        const ModuleDatabase &db;
        AbstractCompiler &compiler;
        const OriginEnv &env;

        using CompileStep = PSource;
        using LinkStep = std::pair<std::vector<PSource>, std::filesystem::path>; //objects and output
        std::variant<CompileStep, LinkStep> step;

        //compile action
        bool operator()() const noexcept;        
        void add_to_cctable(CompileCommandsTable &cctable) const;
        auto get_references(const PSource &f) const;
    };

    BuildPlan<CompileAction> create_build_plan(AbstractCompiler &compiler, 
                const OriginEnv &env,
                std::span<const CompileTarget> targets, 
                bool recompile, bool build_library) const;


    template<typename VectorOfSourceDef>
    static void extract_module_mapping(const BuildPlan<CompileAction> &a, VectorOfSourceDef &out);

    void check_for_modifications(AbstractCompiler &compiler);
    void check_for_recompile();
    void recompile_all();

    bool check_database_version(const std::filesystem::path &compiler, std::span<const ArgumentString> arguments);

    void export_database(std::ostream &s) const;
    void import_database(std::istream &s);

    template<typename Me, typename Arch>
    static void serialize(Me &me, Arch &arch) {
        int v = file_magic;
        arch(v);
        if (v != file_magic) return;
        v = file_version_nr;
        arch(v);
        if (v != file_version_nr) return;

        arch(me._hash_settings);
        if constexpr (std::is_const_v<std::remove_reference_t<Me> >) {
            arch(me._import_time);
            std::vector<Source *> src;
            std::transform(me._fileIndex.begin(), me._fileIndex.end(), std::back_inserter(src), [](const auto &itm) -> Source *{
                return itm.second.get();
            });
            arch(src);
        } else {
            arch(me._modify_time);
            std::vector<Source> src;
            arch(src);
            for (auto &x: src) me.put(std::move(x));
        }        
    }


protected:
    FileIndex _fileIndex;
    ModuleIndex _moduleIndex;
    OriginMap _originMap;
    std::size_t _hash_settings = 0;
    std::chrono::system_clock::time_point _modify_time; //time when database was modified
    std::chrono::system_clock::time_point _import_time = std::chrono::system_clock::now();   //time when database was imported
    mutable std::atomic<bool> _modified;     //database has been modified

    ///create transitive clousure from source file (source is excluded)
    /**
     * @param from starting source file, this file is  not included
     * @param ret result array to fill
     */
    template<typename FnRanged>
    void transitive_closure(PSource from, FnRanged &&ret) const;

    ///collects all bmis required to compile source "from"
    template<typename FnRanged>
    void collect_bmi_references(PSource from, FnRanged &&ret) const;
    
    static Unsatisfied merge_references(Unsatisfied a1, Unsatisfied a2);
  

};