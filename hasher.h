#pragma once
#include <cstddef>
#include <string>
#include <vector>

// Поддерживаемые алгоритмы хеширования
enum class HashAlgo { CRC32, MD5 };

// Вычислить хеш блока данных.
// Возвращает бинарную строку (raw bytes) — для сравнения достаточно ==.
std::string hashBlock(const char* data, std::size_t size, HashAlgo algo);

// Распарсить строку "crc32" / "md5" -> HashAlgo
HashAlgo parseHashAlgo(const std::string& name);
