#include "scanner.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <stdexcept>

namespace fs = boost::filesystem;

// Преобразуем glob-маску в regex-паттерн (только * и ?)
static boost::regex maskToRegex(const std::string& mask)
{
    std::string pattern;
    for (char ch : mask) {
        if (ch == '*')      pattern += ".*";
        else if (ch == '?') pattern += ".";
        else if (std::string(".^$+{}[]|()/\\").find(ch) != std::string::npos)
            pattern += std::string("\\") + ch;
        else               pattern += ch;
    }
    // Маска применяется к имени без учёта регистра
    return boost::regex(pattern, boost::regex::icase);
}

// Проверяем, соответствует ли имя файла хотя бы одной маске
static bool matchesMasks(const std::string& filename,
                          const std::vector<boost::regex>& regexes)
{
    if (regexes.empty()) return true;  // маски не заданы — всё разрешено
    for (const auto& re : regexes) {
        if (boost::regex_match(filename, re)) return true;
    }
    return false;
}

const std::string& FileEntry::blockHash(std::size_t idx) const
{
    if (hashes[idx].empty()) {
        // Блок ещё не вычислен — читаем с диска
        std::ifstream f(path.string(), std::ios::binary);
        if (!f)
            throw std::runtime_error("Не удалось открыть файл: " + path.string());

        f.seekg(static_cast<std::streamoff>(idx * blockSize));
        std::vector<char> buf(blockSize, '\0');

        // Последний блок может быть короче blockSize
        f.read(buf.data(), static_cast<std::streamsize>(blockSize));
        const std::size_t bytesRead = static_cast<std::size_t>(f.gcount());

        hashes[idx] = hashBlock(buf.data(), bytesRead, algo);
    }
    return hashes[idx];
}

// Количество блоков в файле (с округлением вверх)
std::size_t FileEntry::numBlocks() const
{
    if (blockSize == 0) return 0;
    return (fileSize + blockSize - 1) / blockSize;
}

// Рекурсивный (или нет) обход директории с фильтрацией
static void collectFiles(const fs::path& dir,
                          int currentDepth,
                          int maxDepth,
                          const std::vector<fs::path>& excludePaths,
                          std::uintmax_t minSize,
                          const std::vector<boost::regex>& regexes,
                          std::size_t blockSize,
                          HashAlgo algo,
                          std::vector<FileEntry>& result)
{
    if (!fs::is_directory(dir)) return;

    for (const auto& entry : fs::directory_iterator(dir)) {
        const fs::path& p = entry.path();

        // Проверяем, не исключена ли эта папка
        bool excluded = false;
        for (const auto& ex : excludePaths) {
            if (fs::equivalent(p, ex)) { excluded = true; break; }
        }
        if (excluded) continue;

        if (fs::is_directory(p)) {
            // Уходим в подпапку только если глубина позволяет
            if (maxDepth < 0 || currentDepth < maxDepth)
                collectFiles(p, currentDepth + 1, maxDepth,
                             excludePaths, minSize, regexes,
                             blockSize, algo, result);
        } else if (fs::is_regular_file(p)) {
            const std::uintmax_t sz = fs::file_size(p);
            if (sz < minSize) continue;
            if (!matchesMasks(p.filename().string(), regexes)) continue;

            FileEntry fe;
            fe.path      = p;
            fe.fileSize  = sz;
            fe.blockSize = blockSize;
            fe.algo      = algo;
            // Инициализируем кеш пустыми строками
            fe.hashes.resize(fe.numBlocks());
            result.push_back(std::move(fe));
        }
    }
}

std::vector<FileEntry> scanFiles(const ScanOptions& opts)
{
    // Компилируем маски в regex-паттерны
    std::vector<boost::regex> regexes;
    for (const auto& m : opts.masks)
        regexes.push_back(maskToRegex(m));

    // Приводим исключённые пути к каноническому виду
    std::vector<fs::path> excludePaths;
    for (const auto& e : opts.excludeDirs) {
        boost::system::error_code ec;
        fs::path cp = fs::canonical(e, ec);
        if (!ec) excludePaths.push_back(cp);
    }

    std::vector<FileEntry> result;
    for (const auto& d : opts.dirs) {
        boost::system::error_code ec;
        fs::path dir = fs::canonical(d, ec);
        if (ec || !fs::is_directory(dir)) continue;

        collectFiles(dir, 0, opts.level,
                     excludePaths, opts.minSize,
                     regexes, opts.blockSize, opts.algo, result);
    }
    return result;
}
