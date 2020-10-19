// -*- mode: c++; -*-

#include <unistd.h>

#include <cstring>

#include <iostream>
#include <fstream>
#include <limits>
#include <map>
#include <memory>

#include <filesystem>
namespace fs = std::filesystem;

#include <ext/stdio_filebuf.h>

#include <fmt/format.h>
using fmt::print;

#include <lzw_streambase.hh>
#include <lzw-d.hh>

#if !defined(BYTE_ORDER)
#  error undefined endianness
#endif // !BYTE_ORDER

#define LZW_LSB_PACKING LITTLE_ENDIAN
#define LZW_MSB_PACKING    BIG_ENDIAN

#define LZW_PACKING LZW_LSB_PACKING

#define LZW_MIN_BITS  9
#define LZW_MAX_BITS 16

#define LZW_CLEAR_TABLE_CODE 256
#define LZW_EOD_CODE         257
#define LZW_FIRST_CODE       257

namespace lzw {
namespace detail {

template< typename OutputIterator >
struct buffer_t
{
    explicit buffer_t(OutputIterator iter)
        : iter(iter), buf(), pending()
    { }

    void write(size_t value, size_t bits) {
#if LZW_PACKING == LZW_MSB_PACKING
        // MSB bit-packing order: TIFF, PDF, etc.
        buf |= (value << ((sizeof buf << 3) - pending));
#elif LZW_PACKING == LZW_LSB_PACKING
        // LSB bit-packing order: GIF, etc.
        buf |= (value << pending);
#endif // LZW_PACKING == ...

        pending += bits;
        do_write(7);
    }

    ~buffer_t() {
        do_write(0);
    }

private:
    void do_write(size_t threshold) {
        for (; pending > threshold; pending -= (std::min)(8UL, pending)) {
#if LZW_PACKING == LZW_MSB_PACKING
            *iter++ = buf >> ((sizeof buf << 3) - 8);
            buf <<= 8;
#elif LZW_PACKING == LZW_LSB_PACKING
            *iter++ = buf & 0xFF;
            buf >>= 8;
#endif // LZW_PACKING == LZW_LSB_PACKING
        }
    }

private:
    OutputIterator iter;
    size_t buf, pending;
};

} // namespace detail

template< typename InputIterator, typename OutputIterator >
void compress(InputIterator iter, InputIterator last, OutputIterator out)
{
    ASSERT(0 == (LZW_MAX_BITS & (LZW_MAX_BITS - 1)));

    size_t bits = LZW_MIN_BITS, next = LZW_FIRST_CODE;
    ASSERT(LZW_MAX_BITS >= LZW_MIN_BITS);

    std::map< std::string, size_t > table;

    for (size_t i = 0; i < 256; ++i)
        table[std::string(1, i)] = i;

    //
    // Output header
    //
    *out++ = 0x1F;
    *out++ = 0x9D;
    *out++ = 0x80 | LZW_MAX_BITS;

    std::string s;
    detail::buffer_t< OutputIterator > buf(out);

    for (; iter != last; ++iter) {
        s.append(1UL, *iter);

        if (table.end() == table.find(s)) {
            //
            // Add the new string to the table:
            //
            if ((1UL << LZW_MAX_BITS) > next)
                table[s] = next++;

            s.pop_back();
            buf.write(table.at(s), bits);

            if (bits < LZW_MAX_BITS && (1UL << bits) < next)
                ++bits;

            s = *iter;
        }
    }

    if (!s.empty())
        buf.write(table.at(s), bits);
}

template< typename InputStream, typename OutputStream >
void uncompress(InputStream &input, OutputStream &output, size_t max_code = 32767)
{
    input_code_stream< InputStream > in(input, max_code);
    output_symbol_stream< OutputStream > out(output);

    std::unordered_map< unsigned, std::string > table((max_code * 11) / 10);

    for (size_t i = 0; i < 256; ++i)
        table[i] = std::string(1, i);

    std::string previous_string;
    unsigned code, next_code = 257;

    while (in >> code) {
        if (table.find(code) == table.end())
            table[code] = previous_string + previous_string[0];

        out << table[code];

        if (previous_string.size() && next_code <= max_code)
            table[next_code++] = previous_string + table[code][0];

        previous_string = table[code];
    }
}

template< typename InputIterator, typename OutputIterator >
void uncompress(InputIterator, InputIterator, OutputIterator)
{
}

} // namespace lzw

static void
press(std::istream &in, std::ostream &out, size_t max_code,
      bool uncompress_mode = false)
{
    using        iterator = std::istream_iterator< char >;
    using output_iterator = std::ostream_iterator< char >;

    if (uncompress_mode) {
#if 1
        lzw::uncompress(in, out, max_code);
#else
        lzw::uncompress(iterator(in), iterator(), output_iterator(out));
#endif // 0
    } else {
        lzw::compress(iterator(in), iterator(), output_iterator(out));
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
