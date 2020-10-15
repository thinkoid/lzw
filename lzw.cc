// -*- mode: c++; -*-

#define _ITERATOR_DEBUG_LEVEL 0

#include <unistd.h>

#include <cstring>

#include <iostream>
#include <fstream>
#include <memory>

#include <filesystem>
namespace fs = std::filesystem;

#include <ext/stdio_filebuf.h>

#include <fmt/format.h>
using fmt::print;

#include <lzw_streambase.hh>
#include <lzw-d.hh>
#include <lzw.hh>

static void
press(std::istream &in, std::ostream &out, size_t max_code,
      bool uncompress_mode = false)
{
    if (uncompress_mode)
        lzw::decompress(in, out, max_code);
    else
        lzw::compress(in, out, max_code);
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
