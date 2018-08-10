#include <memory>

/**
 * Create a unique_ptr<T> value.
 *
 * C++ cannot deduce the type here:
 *   std::unique_ptr<Deduced type> ptr(p);
 * But it can deduce the type if it is made into a function:
 *   auto ptr = mkuniq(p);
 * This often makes the code look a bit cleaner.
 */
template <class T>
inline std::unique_ptr<T> mkuniq(T *p) {
    return std::unique_ptr<T>(p);
}
