// -*- mode: c++; -*-
// Copyright (c) 2011 Mark Nelson
// Copyright (c) 2020- Thinkoid, LLC

#include <defs.hh>
#include <lzw.hh>

#include <unistd.h>

#include <fmt/format.h>
using fmt::print;

#include <fstream>
#include <iterator>

struct binary_istream_iterator_t
{
private:
    using stream_type = std::basic_istream< char, std::char_traits< char > >;

public:
    using iterator_category = std::input_iterator_tag;

    using value_type = char;

    using pointer = char *;
    using const_pointer = const char *;

    using difference_type = std::ptrdiff_t;

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

////////////////////////////////////////////////////////////////////////

static void
press(std::istream &in, std::ostream &out, bool uncompress_mode = false)
{
    using iterator = binary_istream_iterator_t;
    using output_iterator = std::ostream_iterator< char >;

    if (uncompress_mode) {
        lzw::uncompress(iterator(in), iterator(), output_iterator(out));
    } else {
        lzw::compress(iterator(in), iterator(), output_iterator(out));
    }
}

int main(int argc, char **argv)
{
    bool uncompress_mode = false;

    for (int opt; -1 != (opt = getopt(argc, argv, "d")); ) {
        switch (opt) {
        case 'd':
            uncompress_mode = true;
            break;

        default:
            print(stderr, "invalid option {}\n", char(opt));
            print(stderr, "Usage: prog [-m maxcode] [-d] [input [output]]\n");
            exit(2);
        }
    }

    switch (argc - optind) {
    case 2: {
        using namespace std;

        ifstream in(argv[optind], ios::in  | ios::binary);
        in.unsetf(ios::skipws);

        ofstream out(argv[optind + 1], ios::out | ios::binary);

        press(in, out, uncompress_mode);
    }
        break;

    case 1: {
        using namespace std;

        ifstream in(argv[optind], ios_base::in  | ios_base::binary);
        in.unsetf(ios::skipws);

        press(in, cout, uncompress_mode);
    }
        break;

    case 0: {
        using namespace std;
        press(cin, cout, uncompress_mode);
    }
        break;

    default:
        break;
    }

    return 0;
}
