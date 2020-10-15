// -*- mode: c++; -*-

#ifndef LZW_HH
#define LZW_HH

#include <string>
#include <unordered_map>

namespace lzw {

template< typename InputStream, typename OutputStream >
void compress(InputStream &input, OutputStream &output, size_t max_code = 32767)
{
    input_symbol_stream< InputStream > in(input);
    output_code_stream< OutputStream > out(output, max_code);

    std::unordered_map< std::string, unsigned > table((max_code * 11) / 10);

    for (size_t i = 0; i < 256; ++i)
        table[std::string(1, i)] = i;

    unsigned next_code = 257;
    std::string  current_string;

    for (char c; in >> c; ) {
        current_string = current_string + c;

        if (table.find(current_string) == table.end()) {
            if (next_code <= max_code)
                table[current_string] = next_code++;

            current_string.pop_back();
            out << table[current_string];

            current_string = c;
        }
    }

    if (!current_string.empty())
        out << table[current_string];
}

template< typename InputStream, typename OutputStream >
void decompress(InputStream &input, OutputStream &output, size_t max_code = 32767)
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

}; //namespace lzw

#endif // LZW_HH
