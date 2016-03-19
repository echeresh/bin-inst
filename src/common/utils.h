#pragma once
#include <cstdint>
#include <chrono>
#include <fstream>
#include <string>

namespace utils
{
    inline uint64_t rdtsc() __attribute__((always_inline));
    inline uint64_t rdtsc()
    {
        uint64_t x;
        __asm__ volatile ("rdtsc\n\tshl $32, %%rdx\n\tor %%rdx, %%rax" : "=a" (x) : : "rdx");
        return x;
    }

    inline double dsecnd()
    {
        return std::chrono::duration<double, std::ratio<1, 1>>
            (std::chrono::steady_clock::now() - std::chrono::steady_clock::time_point()).count();
    }

    template <class T>
    void save(const T& o, std::ostream& out)
    {
        out.write((char*)&o, sizeof(o));
    }

    template <class T>
    T load(std::istream& in)
    {
        T t;
        in.read((char*)&t, sizeof(t));
        return t;
    }

    template <>
    void save<std::string>(const std::string& s, std::ostream& out);

    template<>
    std::string load<std::string>(std::istream& in);

    /*
    template <class T>
    void saveVector(const std::vector<T>& v, ostream& out)
    {
        auto sz = v.size();
        out.write((char*)&sz, sizeof(sz));
        if (sz != 0)
            out.write((char*)&v[0], v.size() * sizeof(T));
    }

    template <class T, class BinInputStream>
    vector<T> loadVector(BinInputStream& in)
    {
        vector<T>::size_type sz;
        in.read((char*)&sz, sizeof(sz));
        vector<T> v(sz);
        if (sz != 0)
            in.read((char*)&v[0], sz*sizeof(T));
        return v;
    }
    */
} //namespace utils
