#pragma once

#include "abstract_compiler.hpp"
#include "compile_commands_supp.hpp"
#include "module_database.hpp"
#include "utils/thread_pool.hpp"
#include <future>
#include <mutex>
#include "build_plan.hpp"

class Builder {
public:



    template<typename Action>
    struct BuildState {
        ThreadPool &_thrp;
        const BuildPlan<Action> &_plan;
        typename BuildPlan<Action>::State _state;
        std::promise<bool> _result;
        bool _is_stopped = false;
        bool _keep_going = false;
        std::mutex _mx;

        BuildState(ThreadPool &pool, const BuildPlan<Action> &plan, bool keep_going)
            :_thrp(pool), _plan(plan), _state(plan.initialize_state()),_keep_going(keep_going) {}

        static void start(std::shared_ptr<BuildState> st) {
            std::lock_guard _(st->_mx);
            next_step(std::move(st));
        }

        static void next_step(std::shared_ptr<BuildState> st) {
            bool done = st->_plan.prepare_actions(st->_state, [&](auto id, const Action &action){
                st->_thrp.push([st, &action, id]() noexcept {
                    std::unique_lock lk(st->_mx);
                    if (st->_is_stopped) return;
                    lk.unlock();
                    bool ok = action();
                    lk.lock();
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
                _result.set_value(result);
            }
        }
    };



    template<typename Action>
    requires(std::is_nothrow_invocable_r_v<bool, Action>)
    static std::future<bool> build(ThreadPool &tp, const BuildPlan<Action> &plan, bool keep_going) {
        auto st = std::make_shared<BuildState<Action> >(tp, plan, keep_going);
        auto ret = st->_result.get_future();
        BuildState<Action>::start(std::move(st));
        return ret;


    }



};