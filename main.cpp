#include "duplicates.h"
#include "hasher.h"
#include "scanner.h"

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    try {
        ScanOptions opts;
        std::string hashName = "crc32";

        // ---- Описание параметров командной строки ----
        po::options_description desc("bayan — поиск дублей файлов\n\nПараметры");
        desc.add_options()
            ("help,h",
             "Показать справку")
            ("dirs,d",
             po::value<std::vector<std::string>>(&opts.dirs)->multitoken(),
             "Директории для сканирования (можно несколько)")
            ("exclude,e",
             po::value<std::vector<std::string>>(&opts.excludeDirs)->multitoken(),
             "Директории для исключения (можно несколько)")
            ("level,l",
             po::value<int>(&opts.level)->default_value(-1),
             "Глубина сканирования (-1 = рекурсивно, 0 = только указанная папка)")
            ("min-size,m",
             po::value<std::uintmax_t>(&opts.minSize)->default_value(1),
             "Минимальный размер файла в байтах (по умолчанию 1)")
            ("mask,n",
             po::value<std::vector<std::string>>(&opts.masks)->multitoken(),
             "Маски имён файлов (например: *.txt *.cpp)")
            ("block-size,b",
             po::value<std::size_t>(&opts.blockSize)->default_value(4096),
             "Размер блока для хеширования (байт)")
            ("hash,a",
             po::value<std::string>(&hashName)->default_value("crc32"),
             "Алгоритм хеширования: crc32 или md5");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help") || opts.dirs.empty()) {
            std::cout << desc << "\n";
            return 0;
        }

        // Валидация параметров
        if (opts.blockSize == 0) {
            std::cerr << "Ошибка: размер блока должен быть > 0\n";
            return 1;
        }

        opts.algo = parseHashAlgo(hashName);

        // ---- Сканирование файлов ----
        const auto files = scanFiles(opts);

        if (files.empty()) {
            std::cout << "Файлы не найдены\n";
            return 0;
        }

        // ---- Поиск дублей ----
        const auto dupGroups = findDuplicates(files);

        if (dupGroups.empty()) {
            std::cout << "Дублей не найдено\n";
            return 0;
        }

        // ---- Вывод результата ----
        // Каждая группа дублей выводится блоком; группы разделяются пустой строкой
        for (const auto& group : dupGroups) {
            for (const auto* fe : group)
                std::cout << fe->path.string() << "\n";
            std::cout << "\n";
        }

    } catch (const po::error& e) {
        std::cerr << "Ошибка параметров: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
