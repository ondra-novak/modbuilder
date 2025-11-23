module;


export module cairn.builder;

import cairn.abstract_compiler;
import cairn.build_plan;
import cairn.utils.threadpool;
import cairn.utils.log;
import <mutex>;
import <atomic>;
import <memory>;


export class Builder {
public:



    template<typename Action>
    struct BuildState {
        ThreadPool &_thrp;
        const BuildPlan<Action> &_plan;
        typename BuildPlan<Action>::State _state;
        std::atomic<bool> _done = {false};
        bool _result = false;
        bool _is_stopped = false;
        bool _keep_going = false;
        std::mutex _mx;
        std::size_t _to_compile;
        std::size_t _compiled = 0;

        BuildState(ThreadPool &pool, const BuildPlan<Action> &plan, bool keep_going)
            :_thrp(pool), _plan(plan), _state(plan.initialize_state())
            ,_keep_going(keep_going), _to_compile(plan.get_plan().size()) {}

        static void start(std::shared_ptr<BuildState> st) {
            std::lock_guard<std::mutex> _(st->_mx);
            next_step(std::move(st));
        }

        static void next_step(std::shared_ptr<BuildState> st) {
            bool done = st->_plan.prepare_actions(st->_state, [&](auto id, const Action &action){
                st->_thrp.push([st, &action, id]() noexcept {
                    std::unique_lock<std::mutex> lk(st->_mx);
                    if (st->_is_stopped) return;
                    lk.unlock();                    
                    bool ok = action();
                    lk.lock();
                    ++st->_compiled;
                    Log::verbose("[{:3}%] {}", (st->_compiled * 100 + st->_to_compile/2)/st->_to_compile, st->_plan.get_plan()[id].name);
                    st->_plan.mark_done(st->_state, id);
                    if (ok || st->_keep_going) next_step(st);
                    else st->finish(false);
                });
                return true;
            });
            if (done) st->finish(true);
        }

        void finish(bool result) {
            if (!_is_stopped) {
                _is_stopped = true;
                _result = result;
                _done = true;
                _done.notify_all();
            }
        }
    };



    template<typename Action>
    requires(std::is_nothrow_invocable_r_v<bool, Action>)
    static bool build(ThreadPool &tp, const BuildPlan<Action> &plan, bool keep_going) {
        auto st = std::make_shared<BuildState<Action> >(tp, plan, keep_going);
        BuildState<Action>::start(st);
        st->_done.wait(false);        
        return st->_result;


    }



};