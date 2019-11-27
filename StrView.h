#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <string>

#ifndef DPS_STRVIEW_H_
#define DPS_STRVIEW_H_

class StrView {
    const char *ptr;
    size_t len;

    // Avoid undefined memcmp behaviour when len == 0
    static bool memEqual(const char *a, const char *b, size_t n) {
        return n == 0 || ::memcmp(a, b, n) == 0;
    }

public:
    static const size_t npos = ~size_t(0);

    StrView() : ptr(nullptr), len(0) { }
    StrView(const char *data, size_t size) : ptr(data), len(size) { }
    StrView(const std::string &str) : ptr(str.data()), len(str.size()) { }
    StrView(const char *str) : ptr(str), len(::strlen(str)) { }

    const char *data() const { return ptr; }
    bool empty() const { return len == 0; }
    size_t size() const { return len; }
    const char *begin() const { return ptr; }
    const char *end() const { return ptr + len; }
    char operator[](size_t i) const { return ptr[i]; }

    StrView substr(size_t start, size_t n = npos) const {
        start = std::min(start, len);
        return StrView(ptr + start, std::min(n, len - start));
    }

    bool startswith(StrView prefix) const {
        return len >= prefix.len && memEqual(ptr, prefix.ptr, prefix.len);
    }

    bool operator==(StrView that) const {
        return len == that.len && memEqual(ptr, that.ptr, len);
    }
    bool operator!=(StrView that) const { return !(*this == that); }

    size_t find(char ch) const {
        for (size_t i = 0; i < len; ++i) {
            if (ptr[i] == ch)
                return i;
        }
        return npos;
    }

    std::string str() const { return len ? std::string(ptr, len) : std::string(); }
    operator std::string() const { return str(); }
};

inline std::ostream &operator<<(std::ostream &out, StrView str) {
    out.write(str.data(), str.size());
    return out;
}

#endif
