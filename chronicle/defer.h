#pragma once
// defer //////////////////////////////////////////////////////
template <typename F>
struct DeferStruct {
    F f;
    DeferStruct(F f) : f(f) {}
    ~DeferStruct() { f(); }
};

template <typename F>
DeferStruct<F> CreateDefer(F f) {
    return DeferStruct<F>(f);
}

#define CAT(x, y)         x##y
#define CALL_CAT(x, y)    CAT(x, y)
#define ADD_COUNTER(x)    CALL_CAT(x, __COUNTER__)
#define DEFER(f)       auto ADD_COUNTER(_defer_) = DeferStruct(f)
// DEFER(f) -> audo _defer_1 = DefStruct(f)
// end defer