#include "builder.hpp"
#include "abstract_compiler.hpp"
#include "module_database.hpp"
#include "origin_env.hpp"
#include "utils/arguments.hpp"
#include "utils/log.hpp"

#include <unordered_set>

/*

namespace {

using SourceDef = AbstractCompiler::SourceDef;

SourceDef referenceToBMISourceDef(const ModuleDatabase::CompilePlanReference &ref) {
    return SourceDef {
        ref.source->type,
        ref.name,
        ref.source->bmi_path
    };
}

SourceDef referenceToSourceDef(const ModuleDatabase::CompilePlanReference &ref) {
    return SourceDef {
        ref.source->type,
        ref.name,
        ref.source->source_file
    };
}

std::vector<SourceDef> createBMIReferences(const ModuleDatabase::CompilePlan &plan) {
    std::vector<SourceDef> out;
    out.reserve(plan.references.size());
    for (const auto &r: plan.references)  {
        out.push_back(referenceToBMISourceDef(r));
    }  
    return out; 
}


}

Builder::Builder(std::size_t threads, AbstractCompiler &compiler, OriginEnv default_env)
:_compiler(compiler),_default_env(std::move(default_env)) 
{
    _thrp.start(threads);
}

enum class TaskState {
    done,
    in_progress,
    waiting
};

struct OldBuildState {

    
    ThreadPool &tp;
    AbstractCompiler &cmp;
    std::vector<ModuleDatabase::CompilePlan> plan;
    OriginEnv def_origin;
    bool stop_on_error = false;
    bool error = false;
    bool stopped = false;

    OldBuildState(ThreadPool &tp, AbstractCompiler &cmp, 
        std::vector<ModuleDatabase::CompilePlan> plan,
        bool stop_on_error)
        :tp(tp),cmp(cmp),plan(std::move(plan))
        ,def_origin(OriginEnv::default_env())
        ,stop_on_error(stop_on_error)
         {
            cnt_to_compile =0;
            for (auto &p: this->plan) {
                bool recompile = p.sourceInfo->state.recompile;
                if (!p.sourceInfo->bmi_path.empty() && !std::filesystem::exists(p.sourceInfo->bmi_path)) recompile = true;
                if (!p.sourceInfo->object_path.empty() && !std::filesystem::exists(p.sourceInfo->object_path)) recompile = true;
                states[p.sourceInfo] = recompile?TaskState::waiting:TaskState::done;
                cnt_to_compile += recompile?1:0;
            }
            cnt_compiled = 0;


        }



    std::mutex mx;
    std::unordered_map<ModuleDatabase::PSource, TaskState> states;
    std::promise<bool> prom;
    std::size_t cnt_to_compile;
    std::size_t cnt_compiled;

    static void start(std::shared_ptr<OldBuildState> me) {
        std::lock_guard _(me->mx);
        spawn_tasks(std::move(me));
    }

    static void spawn_tasks(std::shared_ptr<OldBuildState> me) {
        std::size_t cnt = me->plan.size();
        bool all_done = true;
        for (std::size_t i = 0; i < cnt; ++i) {
            auto src = me->plan[i].sourceInfo;
            auto &st = me->states[src];
            if (st == TaskState::done) continue;
                all_done = false;
            if (st == TaskState::in_progress) continue;            
            
            bool can_build = true;
            for(const auto &ref: me->plan[i].references) {
                TaskState &refst = me->states[ref.source];
                if (refst != TaskState::done) {
                    can_build = false;
                    break;
                }
            }
            if (can_build) {
                st = TaskState::in_progress;
                me->tp.push([me,i]() noexcept{
                    run_build(me, i);
                });
                
            }
        }
        if (all_done) {
            if (!me->stopped) {
                me->prom.set_value(!me->error);
                me->stopped = true;
            }            
        }
    }

    static void run_build(std::shared_ptr<OldBuildState> me, std::size_t index) {
        std::unique_lock lk(me->mx);
        if (me->stopped) return;

        auto &item = me->plan[index];
        AbstractCompiler::CompileResult compile_res;
        const auto &env = item.sourceInfo->origin?*item.sourceInfo->origin:me->def_origin;
        auto modmap = createBMIReferences(item);
        lk.unlock();
       int r = me->cmp.compile(env, {
                item.sourceInfo->type,
                item.sourceInfo->name,
                item.sourceInfo->source_file}, modmap, compile_res);
        lk.lock();
        ++me->cnt_compiled;
        auto percent = (me->cnt_compiled*100+me->cnt_to_compile/2)/me->cnt_to_compile;
        if (!r) Log::verbose("{}% Compiled: {}", percent, item.sourceInfo->source_file.string());
        else Log::verbose("{}% Compile failed: {} code {}",percent,  item.sourceInfo->source_file.string(), r);

        if (r) {
            me->error = true;
            if (me->stop_on_error && !me->stopped) {
                me->stopped = true;
                me->prom.set_value(false);
                return;
            }
        }
        item.sourceInfo->bmi_path =std::move(compile_res.interface);
        item.sourceInfo->object_path = std::move(compile_res.object);
        item.sourceInfo->state.recompile = false;
        me->states[item.sourceInfo] = TaskState::done;        
        spawn_tasks(std::move(me));
    }

};


std::future<bool> Builder::build(std::vector<ModuleDatabase::CompilePlan> plan, bool stop_on_error)
{
    auto state =std::make_shared<OldBuildState>(_thrp, _compiler, std::move(plan), stop_on_error);
    auto ret = state->prom.get_future();
    OldBuildState::start(std::move(state));
    return ret;
}

void Builder::generate_compile_commands(CompileCommandsTable &cctable, std::vector<ModuleDatabase::CompilePlan> plan) {
    std::vector<ArgumentString> args;
    for (auto &item: plan) {
        const auto &env = item.sourceInfo->origin?*item.sourceInfo->origin:_default_env;
        auto modmap = createBMIReferences(item);
        bool b;
        args.clear();
        b =  _compiler.generate_compile_command(env, {
            item.sourceInfo->type, item.sourceInfo->name, item.sourceInfo->source_file
        }, modmap, args);
        if (b) {
            cctable.update(cctable.record(env.working_dir, item.sourceInfo->source_file,args));        
        }
    }
}



std::vector<AbstractCompiler::SourceDef> Builder::create_module_mapping(const std::span<const ModuleDatabase::CompilePlan> &plan) {
    std::unordered_set<AbstractCompiler::SourceDef, MethodHash> s;
    for (const auto &p: plan) {
        for (const auto &r: p.references) {
            s.insert(referenceToSourceDef(r));
        }
    }
    return {s.begin(), s.end()};
}


struct BuildPlanState {
    ThreadPool &pool;
    std::promise<bool> result;
    bool resolved = false;
    bool error = false;
};
*/
