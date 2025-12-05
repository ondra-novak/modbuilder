export module cairn.utils.serializer;

import <concepts>;
import <cstdint>;
import <iostream>;
import <map>;
import <memory>;
import <set>;
import <stdexcept>;
import <type_traits>;
import <utility>;

export template<typename Obj, typename Arch>
concept has_serialize_method = requires(Obj obj, Arch arch) {
    {std::remove_cvref_t<Obj>::template serialize<Obj, Arch>(obj, arch)};
};

export template<typename Obj, typename Arch>
concept has_serialize_rule = requires(Obj obj, Arch arch) {
    {serialize_rule(obj, arch)}->std::same_as<void>;
};
export template<typename Obj, typename Arch>
concept has_deserialize_rule = requires(Obj obj, Arch arch) {
    {deserialize_rule(obj, arch)}->std::same_as<void>;
};




template<typename T> struct is_shared_ptr_t {static constexpr bool value = false;};
template<typename T> struct is_shared_ptr_t<std::shared_ptr<T> > {static constexpr bool value = true;};
template<typename T> constexpr bool is_shared_ptr = is_shared_ptr_t<T >::value;


export template<std::invocable<const void *, std::size_t> Fn>
class SerializerBinaryWriter {
public:

    template<typename T> static constexpr bool fail = false;

    SerializerBinaryWriter(Fn &&fn):_fn(std::forward<Fn>(fn)) {}

    template<typename T>
    requires(std::is_trivially_copy_assignable_v<T>)
    void write_binary(const T &val) {
        _fn(&val,sizeof(val));
    }

    template<typename T, typename ... Args>
    void operator()(const T &val, const Args & ... args) {
        if constexpr (is_shared_ptr<T>) {
            auto ref = std::shared_ptr<const char>(val, reinterpret_cast<const char *>(val.get()));
            int wr = 0;
            if (val) {
                auto ins = shared_refs.emplace(ref, next_idx);                
                if (ins.second) {
                    wr = next_idx | 1; next_idx+=2;               
                } else {
                    wr = ins.first->second;                
                }
            }
            write_binary(wr);
            if (wr & 1) {
                (*this)(*val);
            }
        } else if constexpr(std::is_pointer_v<T>) {
            this->operator()(*val);
        } else if constexpr(has_serialize_method<const T &, SerializerBinaryWriter &>) {
            T::template serialize<const T &, SerializerBinaryWriter &>(val, *this);
        } else if constexpr(has_serialize_rule<const T &, SerializerBinaryWriter &>) {
            serialize_rule(val, *this);
        } else {
            static_assert(std::is_trivially_copy_assignable_v<T>, "Missing serialization rules, type is not trivial");
            write_binary(val);
        }
        if constexpr(sizeof...(Args) > 0) {
            this->operator()(args...);
        }

    } 

protected:
    Fn _fn;
    std::map<std::shared_ptr<const char>, int> shared_refs;
    int next_idx = 2;
};

export template<std::invocable<void *, std::size_t> Fn>
class SerializerBinaryReader {
public:

    static_assert(std::is_invocable_r_v<bool, Fn, void *, std::size_t>);

    template<typename T> static constexpr bool fail = false;

    SerializerBinaryReader(Fn &&fn):_fn(std::forward<Fn>(fn)) {}

    template<typename T>
    requires(std::is_trivially_copy_assignable_v<T>)
    void read_binary(T &val) {
        bool b = _fn(&val,sizeof(val));
        if (!b) throw std::runtime_error("Deserialization: Unexpected EOF");
    }

    template<typename T, typename ... Args>
    void operator()(T &val, Args &... args) {
        if constexpr(is_shared_ptr<T>) {
            int ref;
            read_binary(ref);
            if (!ref) {
                val = {};
            } else {
                if (ref & 1) {
                    val = std::make_shared<typename T::element_type>();                    
                    (*this)(*val);
                    _ptr_map[ref] = val;
                } else {
                    ref |= 1;
                    auto x = _ptr_map[ref];
                    val = T(x, reinterpret_cast<typename T::element_type *>(x.get()));
                    
                }
            }
        } else if constexpr(std::is_pointer_v<T>) {
            this->operator()(*val);
        } else if constexpr(has_serialize_method<T &, SerializerBinaryReader &>) {
            T::template serialize<T &, SerializerBinaryReader &>(val, *this);
        } else if constexpr(has_deserialize_rule<T &, SerializerBinaryReader &>) {
            deserialize_rule(val, *this);
        } else {
            static_assert(std::is_trivially_copy_assignable_v<T>, "Missing serialization rules, type is not trivial");
            read_binary(val);
        }
        if constexpr(sizeof...(Args) > 0) {
            this->operator()(args...);
        }
    } 


protected:
    Fn _fn;
    std::map<int, std::shared_ptr<void> > _ptr_map;
};



export template<typename T>
void serialize_to_stream(std::ostream &out, const T &what) {
    SerializerBinaryWriter wr([&](const void *data, std::size_t sz){
        out.write(reinterpret_cast<const char *>(data),sz);
    });
    wr(what);
}
export template<typename T>
void deserialize_from_stream(std::istream &in, T &what) {    
    SerializerBinaryReader rd([&](void *data, std::size_t sz){
        in.read(reinterpret_cast<char *>(data), sz);
        if (!in || static_cast<std::size_t>(in.gcount()) != sz) return false;
        return true;
    });
    rd(what);
}