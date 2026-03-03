#include "hasher.h"

#include <boost/crc.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <stdexcept>

// CRC32 через boost::crc_32_type
static std::string crc32Hash(const char* data, std::size_t size)
{
    boost::crc_32_type crc;
    crc.process_bytes(data, size);
    std::uint32_t val = crc.checksum();

    // Храним как 4 байта (little-endian не важен — главное стабильность)
    std::string result(4, '\0');
    result[0] = static_cast<char>((val >> 24) & 0xFF);
    result[1] = static_cast<char>((val >> 16) & 0xFF);
    result[2] = static_cast<char>((val >>  8) & 0xFF);
    result[3] = static_cast<char>((val      ) & 0xFF);
    return result;
}

// MD5 через boost::uuids::detail::md5
static std::string md5Hash(const char* data, std::size_t size)
{
    using boost::uuids::detail::md5;
    md5 hash;
    hash.process_bytes(data, size);

    md5::digest_type digest;
    hash.get_digest(digest);

    // digest — массив из 4 uint32_t, итого 16 байт
    std::string result(16, '\0');
    for (int i = 0; i < 4; ++i) {
        result[i * 4 + 0] = static_cast<char>((digest[i] >> 24) & 0xFF);
        result[i * 4 + 1] = static_cast<char>((digest[i] >> 16) & 0xFF);
        result[i * 4 + 2] = static_cast<char>((digest[i] >>  8) & 0xFF);
        result[i * 4 + 3] = static_cast<char>((digest[i]      ) & 0xFF);
    }
    return result;
}

std::string hashBlock(const char* data, std::size_t size, HashAlgo algo)
{
    switch (algo) {
    case HashAlgo::CRC32: return crc32Hash(data, size);
    case HashAlgo::MD5:   return md5Hash(data, size);
    }
    throw std::logic_error("Неизвестный алгоритм хеширования");
}

HashAlgo parseHashAlgo(const std::string& name)
{
    if (name == "crc32") return HashAlgo::CRC32;
    if (name == "md5")   return HashAlgo::MD5;
    throw std::invalid_argument("Неизвестный алгоритм: " + name + " (допустимо: crc32, md5)");
}
