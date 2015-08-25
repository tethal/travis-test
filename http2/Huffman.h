#ifndef HTTP2_HUFFMAN_H_
#define HTTP2_HUFFMAN_H_

#include <cstdint>
#include <exception>
#include <string>

/**
 * \brief Encoder/decoder for the Huffman Code specified in RFC 7541.
 */
class Huffman {
public:

    /**
     * \brief Decodes a byte array into a string.
     * \param src the byte array to decode
     * \param length the length of src
     * \return decoded data
     * \throws InvalidEncoding if the code is invalid, see section 5.2 of the RFC
     */
    static std::string decode(const uint8_t *src, size_t length);

    //TODO encoder

    /**
     * \brief Throws by decode() if the source data do not contain valid Huffman code.
     */
    class InvalidEncoding : public std::exception {};
};

#endif /* HTTP2_HUFFMAN_H_ */
