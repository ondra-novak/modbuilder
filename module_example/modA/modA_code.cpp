module;
#include <just_example.hpp>

export module modA;
import <just_example.hpp>

export namespace mymod {

    int fact(int c) {
        if (c < 2) return 1;
        return c * fact(c-1);
    }

}

