qore_add_static_library(http2
    Huffman.cpp
)

qore_add_test(http2
    test/TestHuffman.cpp
)
