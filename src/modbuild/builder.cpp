#include "builder.hpp"
#include "utils/log.hpp"

Builder::Builder(std::size_t threads, AbstractCompiler &compiler)
:_compiler(compiler) 
{
    _thrp.start(threads);
}

enum class TaskState {
    done,
    in_progress,
    waiting
};

struct BuildState {

    
    ThreadPool &tp;
    AbstractCompiler &cmp;
    std::vector<ModuleDatabase::CompilePlan> plan;
    OriginEnv def_origin;
    bool stop_on_error = false;
    bool error = false;
    bool stopped = false;

    BuildState(ThreadPool &tp, AbstractCompiler &cmp, 
        std::vector<ModuleDatabase::CompilePlan> plan,
        bool stop_on_error)
        :tp(tp),cmp(cmp),plan(std::move(plan))
        ,def_origin(OriginEnv::default_env())
        ,stop_on_error(stop_on_error)
         {
            cnt_to_compile =0;
            for (auto &p: plan) {
                bool recompile = p.sourceInfo->state.recompile 
                    || p.sourceInfo->bmi_path.empty()
                    || !std::filesystem::exists(p.sourceInfo->bmi_path);
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
    
    static void spawn_tasks(std::shared_ptr<BuildState> me) {
        std::lock_guard _(me->mx);
        std::size_t cnt = me->plan.size();
        bool all_done = true;
        for (std::size_t i = 0; i < cnt; ++i) {
            auto src = me->plan[i].sourceInfo;
            auto st = me->states[src];
            if (st == TaskState::done) continue;
                all_done = false;
            if (st == TaskState::in_progress) continue;            
            
            bool can_build = false;
            for(const auto &ref: me->plan[i].references) {
                TaskState &refst = me->states[ref];
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

    static void run_build(std::shared_ptr<BuildState> me, std::size_t index) {
        std::unique_lock lk(me->mx);
        if (me->stopped) return;
        const auto &item = me->plan[index];
        OriginEnv &env =item.sourceInfo->origin?*item.sourceInfo->origin :me->def_origin;
        std::vector<AbstractCompiler::ModuleMapping> modmap;
        AbstractCompiler::CompileResult compile_res;
        lk.unlock();
        int r = me->cmp.compile(env,item.sourceInfo->source_file, item.sourceInfo->type, modmap, compile_res);
        lk.lock();
        ++me->cnt_compiled;
        auto percent = (me->cnt_compiled+me->cnt_to_compile/2)*100/me->cnt_to_compile;
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
    auto state =std::make_shared<BuildState>(_thrp, _compiler, std::move(plan), stop_on_error);
    auto ret = state->prom.get_future();
    BuildState::spawn_tasks(std::move(state));
    return ret;
}

