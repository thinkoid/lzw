// -*- mode: c++; -*-

#include <defs.hh>
#include <binary_istream_iterator.hh>

#include <iostream>
#include <fstream>

int main(int argc, char **argv)
{
    using iterator = lzw::binary_istream_iterator_t;
    std::copy(iterator(std::cin), iterator(), std::ostream_iterator< char >(std::cout));
    return 0;
}
