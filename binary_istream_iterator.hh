// -*- mode: c++; -*-

#include <iostream>
#include <iterator>

namespace lzw {

struct binary_istream_iterator_t
{
    using stream_type = std::basic_istream< char, std::char_traits< char > >;

    using iterator_category = std::input_iterator_tag;

    using char_type = char;
    using value_type = char_type;

    // using difference_type = iterator_type::difference_type;

    using pointer = char *;
    using const_pointer = const char *;

    using reference = char &;
    using const_reference = const char &;

public:
    binary_istream_iterator_t() : stream(0) { }

    binary_istream_iterator_t(stream_type &s) : stream(&s) {
        ++*this;
    }

    const_reference operator*() const {
        return value;
    }

    const_pointer operator->() const {
        return &**this;
    }

    binary_istream_iterator_t &operator++() {
        if (stream && stream->get(value).fail())
            stream = 0;

        return *this;

    }

    binary_istream_iterator_t operator++(int) {
        auto tmp = *this;
        return ++*this, tmp;
    }

    friend bool
    operator==(const binary_istream_iterator_t&,
               const binary_istream_iterator_t&);

private:
    stream_type *stream;
    value_type value;    // last extracted value
};

inline bool
operator==(const binary_istream_iterator_t &lhs,
           const binary_istream_iterator_t &rhs)
{
    return lhs.stream == rhs.stream;
}

inline bool
operator!=(const binary_istream_iterator_t &lhs,
           const binary_istream_iterator_t &rhs)
{
    return !(lhs == rhs);
}

} // namespace lzw
