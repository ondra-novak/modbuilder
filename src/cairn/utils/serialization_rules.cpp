export module cairn.utils.serializer.rules;

import <filesystem>;
import <string>;
import <type_traits>;

template<typename T>
concept is_linear_container = requires(T obj, std::size_t sz) {
    {obj.size()}->std::convertible_to<std::size_t>;
    {obj.resize(sz)};
    {obj.begin()}->std::input_or_output_iterator;    
    {obj.end()}->std::input_or_output_iterator;            
};

template<typename T>
concept is_assoc_container = requires(T obj, std::size_t sz, typename T::value_type val) {
    {obj.size()}->std::convertible_to<std::size_t>;    
    {obj.begin()}->std::input_or_output_iterator;        
    {obj.end()}->std::input_or_output_iterator;            
    {obj.insert(val)};
};

export template<is_linear_container T, typename Arch>
inline void serialize_rule(const T &val, Arch &arch) {
    std::uint32_t sz  = static_cast<std::uint32_t>(val.size());
    arch(sz);
    for (const auto &x: val) arch(x);
}

export template<is_linear_container T, typename Arch>
inline void deserialize_rule(T &val, Arch &arch) {
    std::uint32_t sz;
    arch(sz);
    val.resize(sz);
    for (auto &x: val) arch(x);
}

export template<is_assoc_container Obj, typename Arch>
inline void serialize_rule(const Obj &v, Arch &arch) {
    std::uint32_t sz = static_cast<std::uint32_t>(v.size());
    arch(sz);
    for (const auto &x: v) arch(v);
}
export template<is_assoc_container Obj, typename Arch>
inline void deserialize_rule(Obj &v, Arch &arch) {
    std::uint32_t sz;
    arch(sz);
    for (std::uint32_t i=0;i<sz;++i) {
        typename Obj::value_type x;
        arch(x);
        v.insert(std::move(x));
    }
}

export template<typename Arch>
inline void serialize_rule(const std::filesystem::path &path, Arch &arch) {
    serialize_rule(path.u8string(), arch);
}
export template<typename Arch>
inline void deserialize_rule(std::filesystem::path &path, Arch &arch) {
    std::u8string s;
    deserialize_rule(s,arch);
    path  = s;
}


