export module cairn.build_plan;
import <span>;
import <string>;
import <vector>;
import <queue>;

export template<typename Action>
class BuildPlan {

public:    

    using TargetID = std::size_t;
    
    TargetID create_target(Action action, std::string name) {
        std::size_t out = _items.size();
        _items.push_back({std::move(action),{},name});
        return out;
    }
    void add_dependency(TargetID target, TargetID target_depends_on ) {
        _items[target].dependencies.push_back(target_depends_on);
    }

    enum class TargetState {
        waiting,
        pending,
        done
    };

    struct State {
        std::vector<TargetState> _states;
    };

    

    State initialize_state() const {
        return {{_items.size(), TargetState::waiting}};
    }

    ///Enumerate actions and mark them pending
    /**
     * @param state state object
     * @param fn function called for each available target, the function receives Target and Action, 
     * and must return true to mark target pending, or false to left target waiting
     * @retval true all done (there are no dependencies left)
     * @retval false still in progress
     */
    template<typename Fn>
    requires(std::is_invocable_r_v<bool, Fn, TargetID,const Action &>)
    bool prepare_actions(State &state, Fn &&fn) const {

        if (state._states.size() != _items.size()) return true;

        bool retval = true;
        auto titer = _items.begin();
        for (auto &st: state._states) {
            if (st != TargetState::done)  {
                retval = false;
                if (st == TargetState::waiting) {
                    bool can_run = true;
                    for (auto d: titer->dependencies) {
                        if (state._states[d] != TargetState::done) {
                            can_run = false;
                            break;
                        }
                    }
                    if (can_run) {
                        st = TargetState::pending;
                        bool r = fn(std::distance(_items.begin(), titer), titer->action);
                        if (!r) st = TargetState::waiting;
                    }
                }
            }
            ++titer;
        }
        return retval;
    }

    static void mark_done(State &state, TargetID target) {
        state._states[target] = TargetState::done;
    }

    struct Item {
        Action action;       //action to execute
        std::vector<std::size_t> dependencies;   //dependencies required to be done (index)
        std::string name;
    };

    auto begin() const {return _items.begin();}
    auto end() const {return _items.end();}
    auto get_plan() const {return std::span<const Item>(_items);}

    static unsigned int computeMaxParallelism(const BuildPlan& plan)
    {
        auto &items = plan._items;
        unsigned int n = static_cast<unsigned int>(items.size());
        std::vector<unsigned int> indegree(n, 0);
        std::vector<std::vector<unsigned int>> graph(n);

        // Build graph + indegree count
        for (unsigned int i = 0; i < n; ++i) {
            for (auto dep : items[i].dependencies) {
                graph[dep].push_back(i);    // dep → i
                indegree[i]++;             // i depends on dep
            }
        }

        // Queue of tasks ready to run
        std::queue<unsigned int> q;
        for (unsigned int i = 0; i < n; ++i)
            if (indegree[i] == 0)
                q.push(i);

        unsigned int maxParallel = 0;

        while (!q.empty()) {
            unsigned int levelSize = static_cast<unsigned int>(q.size());     // number of tasks runnable now
            maxParallel = std::max(maxParallel, levelSize);

            // Process whole "level"
            for (unsigned int k = 0; k < levelSize; ++k) {
                unsigned int u = q.front();
                q.pop();

                for (auto v : graph[u]) {
                    if (--indegree[v] == 0)
                        q.push(v);
                }
            }
        }

        return maxParallel;
    }

protected:

    std::vector<Item> _items;
};


