#pragma once
#include "hasher.h"

#include <boost/filesystem.hpp>
#include <string>
#include <vector>

// Описание одного файла-кандидата на дубль.
// Хеши блоков вычисляются лениво и кешируются.
struct FileEntry
{
    boost::filesystem::path path;
    std::uintmax_t          fileSize  = 0;
    std::size_t             blockSize = 0;
    HashAlgo                algo      = HashAlgo::CRC32;

    // Кеш хешей блоков: hashes[i] — хеш i-го блока (пустая строка = ещё не считан)
    mutable std::vector<std::string> hashes;

    // Вернуть хеш блока с индексом idx.
    // Если блок ещё не читался — читаем с диска и кешируем.
    const std::string& blockHash(std::size_t idx) const;

    // Количество блоков в файле
    std::size_t numBlocks() const;
};

// Параметры сканирования
struct ScanOptions
{
    std::vector<std::string> dirs;          // директории для сканирования
    std::vector<std::string> excludeDirs;   // директории для исключения
    int                      level    = -1; // -1 = рекурсивно, 0 = только указанная папка
    std::uintmax_t           minSize  = 1;  // минимальный размер файла (байт)
    std::vector<std::string> masks;         // маски имён файлов (пустой = все)
    std::size_t              blockSize = 1; // размер блока для хеширования
    HashAlgo                 algo = HashAlgo::CRC32;
};

// Собрать список файлов, удовлетворяющих критериям сканирования
std::vector<FileEntry> scanFiles(const ScanOptions& opts);
