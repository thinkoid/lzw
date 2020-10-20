// -*- mode: c++; -*-

#include <unistd.h>

#include <cstring>

#include <iostream>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <type_traits>

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
#define LZW_EOF_CODE         256

#define LZW_EOD_CODE         257
#define LZW_FIRST_CODE       257

namespace lzw {
namespace detail {

template< typename InputIterator >
struct input_buffer_t
{
    explicit input_buffer_t(InputIterator iter, InputIterator last)
        : iter(iter), last(last), buf(), pending()
    { }

    size_t get(size_t bits) {
        do_get(bits);

        if (pending < bits)
            return LZW_EOF_CODE;

        size_t code;

#if LZW_PACKING == LZW_MSB_PACKING
        // MSB bit-packing order: TIFF, PDF, etc.
        code = buf >> ((sizeof buf << 3) - bits);
        buf <<= bits;
#elif LZW_PACKING == LZW_LSB_PACKING
        // LSB bit-packing order: GIF, etc.
        code = buf & ((1UL << bits) - 1);
        buf >>= bits;
#endif // LZW_PACKING == ...

        pending -= bits;

        return code;
    }

private:
    void do_get(size_t bits) {
        for (; iter != last && pending < bits; ++iter, pending += 8) {
            const size_t c = static_cast< unsigned char >(*iter);

#if LZW_PACKING == LZW_MSB_PACKING
            // MSB bit-packing order: TIFF, PDF, etc.
            buf |= c << ((sizeof buf << 3) - pending - 8);
#elif LZW_PACKING == LZW_LSB_PACKING
            // LSB bit-packing order: GIF, etc.
            buf |= c << pending;
#endif // LZW_PACKING == ...
        }
    }

private:
    InputIterator iter, last;
    size_t buf, pending;
};

template< typename OutputIterator >
struct output_buffer_t
{
    explicit output_buffer_t(OutputIterator iter)
        : out(iter), buf(), pending()
    { }

    void put(size_t value, size_t bits) {
#if LZW_PACKING == LZW_MSB_PACKING
        // MSB bit-packing order: TIFF, PDF, etc.
        buf |= (value << ((sizeof buf << 3) - pending));
#elif LZW_PACKING == LZW_LSB_PACKING
        // LSB bit-packing order: GIF, etc.
        buf |= (value << pending);
#endif // LZW_PACKING == ...

        pending += bits;
        do_put(7);
    }

    ~output_buffer_t() {
        do_put(0);
    }

private:
    void do_put(size_t threshold) {
        for (; pending > threshold; pending -= (std::min)(8UL, pending)) {
#if LZW_PACKING == LZW_MSB_PACKING
            *out++ = buf >> ((sizeof buf << 3) - 8);
            buf <<= 8;
#elif LZW_PACKING == LZW_LSB_PACKING
            *out++ = buf & 0xFF;
            buf >>= 8;
#endif // LZW_PACKING == LZW_LSB_PACKING
        }
    }

private:
    OutputIterator out;
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
    *out++ = 0x1F; // magic
    *out++ = 0x9D; // more magic
    *out++ = 0x80 | LZW_MAX_BITS;

    std::string s;
    detail::output_buffer_t< OutputIterator > buf(out);

    for (; iter != last; ++iter) {
        s.append(1UL, *iter);

        if (table.end() == table.find(s)) {
            if ((1UL << LZW_MAX_BITS) > next)
                table[s] = next++;

            s.pop_back();
            buf.put(table.at(s), bits);

            if (bits < LZW_MAX_BITS && (1UL << bits) < next)
                ++bits;

            s = *iter;
        }
    }

    if (!s.empty())
        buf.put(table.at(s), bits);
}

template< typename InputIterator, typename OutputIterator >
void uncompress(InputIterator iter, InputIterator last, OutputIterator out)
{
    size_t next = LZW_FIRST_CODE, bits = LZW_MIN_BITS, max_bits = 0;

    std::map< size_t, std::string > table;

    for (size_t i = 0; i < 256; ++i)
        table[i] = std::string(1UL, i);

    {
        (iter != last && *iter++ == char(0x1F) &&
         iter != last && *iter++ == char(0x9D) &&
         iter != last) ||
            (throw std::runtime_error("invalid header"), false);

        max_bits = static_cast< unsigned char >(*iter++) & ~0x80;

        if ((max_bits & (max_bits - 1)) || bits > max_bits)
            throw std::runtime_error("invalid max bits indicator");
    }

    std::string prev;
    detail::input_buffer_t< InputIterator > buf(iter, last);

    for (size_t code; LZW_EOF_CODE != (code = buf.get(bits)); ) {
        if (table.find(code) == table.end()) {
            ASSERT(!prev.empty());
            table[code] = prev + prev[0];
        }

        const auto &s = table[code];
        std::copy(s.begin(), s.end(), out);

        if (bits < max_bits && (1UL << bits) - 1 <= next)
            ++bits;

        if (!prev.empty() && (1UL << max_bits) > next)
            table[next++] = prev + table[code][0];

        prev = table[code];
    }
}

} // namespace lzw

static void
press(std::istream &in, std::ostream &out, bool uncompress_mode = false)
{
    using        iterator = std::istream_iterator< char >;
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

        __gnu_cxx::stdio_filebuf< char > outbuf(1, ios::out | ios::binary);

        ostream out(&outbuf);
        out.unsetf(ios::skipws);

        press(in, out, uncompress_mode);
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

        press(in, out, uncompress_mode);

        inbuf.close();
        outbuf.close();
    }
        break;

    default:
        break;
    }

    return 0;
}
