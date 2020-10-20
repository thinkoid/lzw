LZW
===

This is a refinement of the code found at
http://marknelson.us/2011/11/08/lzw-revisited/ which re-implements the
compress/decompress pair of functions with templates taking input and output
iterators. The API changed from:


    template< typename InputStream, typename OutputStream >
    void compress(InputStream in, OutputStream out, unsigned max_code = 32767);

    template< typename InputStream, typename OutputStream >
    void decompress(InputStream in, OutputStream out, unsigned max_code = 32767);

to:

    template< typename InputIterator, typename OutputIterator >
    void compress(InputIterator iter, InputIterator last, OutputIterator out);

    template< typename InputIterator, typename OutputIterator >
    void uncompress(InputIterator iter, InputIterator last, OutputIterator out);

The original copyright notice and license is preserved in the lzw.cc source file
and acknowledges the source.

A `binary_iterator_t` type is provided for dealing with default `cin`/`cout`
streams which cannot be reopened in binary mode and, thus, can ruin your
afternoon. See the program lzw.cc.

The parameters of the algorithm(min and max bit-width, clear/EOF/EOD codes,
etc.) are fixed via macros. No sophisticated policy-driven (or otherwise)
mechanism for customizing the behavior, yet.
