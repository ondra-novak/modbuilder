#include "arguments.hpp"
#include "abstract_compiler.hpp"
#include <vector>

template<typename T>
int tmain(int argc, T *argv[]) {

    std::vector<ArgumentStringView> args;
    args.resize(argc);
    for (int i = 0; i < argc; ++i) {
        args[i] = argv[i];
    }

    //todo entry point
    return 0;

}


#ifdef _WIN32
int wmain(int argc, wchar_t *argv[]) {
    return tmain(argc, argv);
}
#else
int main(int argc, char *argv[]) {
    return tmain(argc, argv);
}
#endif