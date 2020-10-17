// -*- mode: c++; -*-

#include <unistd.h>

#include <cstring>

#include <iostream>
#include <fstream>
#include <limits>
#include <memory>

#include <filesystem>
namespace fs = std::filesystem;

#include <ext/stdio_filebuf.h>

#include <fmt/format.h>
using fmt::print;

#include <lzw_streambase.hh>
#include <lzw-d.hh>
#include <lzw.hh>

template< typename InputIterator, typename OutputIterator >
void compress(InputIterator iter, InputIterator last, OutputIterator out,
              size_t max_bits = 16)
{
    ASSERT(0 == (max_bits & (max_bits - 1)));

    size_t bits = 9;
    ASSERT(max_bits >= bits);

    size_t next = 257, pending = 0;
    unsigned buf = 0;

    std::unordered_map< std::string, unsigned > table(1UL << 16);

    for (size_t i = 0; i < 256; ++i)
        table[std::string(1, i)] = i;

    //
    // Output header
    //
    *out++ = 0x1F;
    *out++ = 0x9D;
    *out++ = 0x80 | max_bits;

    std::string s;

    for (; iter != last; ++iter) {
        s.append(1UL, *iter);

        if (table.end() == table.find(s)) {
            //
            // Add the new string to the table:
            //
            if ((1UL << max_bits) > next)
                table[s] = next++;

            s.pop_back();

            buf |= table.at(s) << pending;
            pending += bits;

            for (; 8 <= pending; pending -= 8) {
                *out++ = buf & 0xFF;
                buf >>= 8;
            }

            if (bits < max_bits && (1UL << bits) < next)
                ++bits;

            s = *iter;
        }
    }

    if (!s.empty()) {
        buf |= table.at(s) << pending;
        pending += bits;

        for (; pending; pending -= (std::min)(8UL, pending)) {
            *out++ = buf & 0xFF;
            buf >>= 8;
        }
    }
}

template< typename InputIterator, typename OutputIterator >
void uncompress(InputIterator, InputIterator, OutputIterator)
{
}

static void
press(std::istream &in, std::ostream &out, size_t max_code,
      bool uncompress_mode = false)
{
    using        iterator = std::istream_iterator< char >;
    using output_iterator = std::ostream_iterator< char >;

    if (uncompress_mode) {
        lzw::decompress(in, out, max_code);
        // uncompress(iterator(in), iterator(), output_iterator(out));
    } else {
        compress(iterator(in), iterator(), output_iterator(out));
    }
}

int main(int argc, char **argv)
{
    long maxcode = 32767;
    bool uncompress_mode = false;

    for (int opt; -1 != (opt = getopt(argc, argv, "m:d")); ) {
        switch (opt) {
        case 'm':
            maxcode = atol(optarg);
            break;

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

        press(in, out, maxcode, uncompress_mode);
    }
        break;

    case 1: {
        using namespace std;

        ifstream in(argv[optind], ios_base::in  | ios_base::binary);
        in.unsetf(ios::skipws);

        __gnu_cxx::stdio_filebuf< char > outbuf(1, ios::out | ios::binary);

        ostream out(&outbuf);
        out.unsetf(ios::skipws);

        press(in, out, maxcode, uncompress_mode);
    }
        break;

    case 0: {
        using namespace std;

        __gnu_cxx::stdio_filebuf< char > inbuf(0, ios::in | ios::binary);

        istream in(&inbuf);
        in.unsetf(ios::skipws);

        __gnu_cxx::stdio_filebuf< char > outbuf(1, ios::out | ios::binary);

        ostream out(&outbuf);
        out.unsetf(ios::skipws);

        press(in, out, maxcode, uncompress_mode);

        inbuf.close();
        outbuf.close();
    }
        break;

    default:
        break;
    }

    return 0;
}
