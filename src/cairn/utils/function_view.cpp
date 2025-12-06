export module cairn.utils.function_view;

import <type_traits>;


template<typename Fn, bool nx, typename R, typename ... Args>
concept is_functionview_compatible = (nx?std::is_nothrow_invocable_r_v<R, Fn, Args...>:std::is_invocable_r_v<R, Fn, Args...>);


template<bool nx, typename R, typename ... Args>
class FunctionViewImpl {
public:

    constexpr FunctionViewImpl() = default;

    template<is_functionview_compatible<nx, R, Args...> Fn>
    constexpr FunctionViewImpl(Fn &&fn):_closure(&fn), _fn(invoke_fn<Fn>) {}    

    constexpr R operator()(Args ... args) const noexcept(nx) {        
        return _fn(_closure, std::forward<Args>(args)...);
    }

    constexpr operator bool() const  {
        return _fn != nullptr;
    }

protected:
    const void *_closure = nullptr;
    void (*_fn)(const void *, Args ... ) = nullptr;

    template<typename Fn>
    constexpr static R invoke_fn(const void *cctx, Args ... args) {
        void *ctx = const_cast<void *>(cctx);
        using FnPtr = std::add_pointer_t<std::remove_reference_t<Fn> >;
        auto closure = static_cast<FnPtr>(ctx);
        return (*closure)(std::forward<Args>(args)...);
    }
};

export template<typename Prototype> class FunctionView;

export template<typename R, typename ... Args>
class FunctionView<R(Args...)>: public FunctionViewImpl<false, R, Args...> {
public:
    using FunctionViewImpl<false, R, Args...>::FunctionViewImpl;
};

export template<typename R, typename ... Args>
class FunctionView<R(Args...) noexcept>: public FunctionViewImpl<true, R, Args...> {
public:
    using FunctionViewImpl<true, R, Args...>::FunctionViewImpl;
};