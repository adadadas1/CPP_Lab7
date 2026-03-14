# Теория к лабораторной работе №7: библиотека Boost

## Цель

Получить практические навыки использования библиотеки Boost при разработке программ на C++.

---

## Что такое Boost

**Boost** — коллекция рецензируемых, портируемых C++ библиотек, расширяющих стандартную библиотеку. Многие компоненты Boost впоследствии вошли в стандарт C++11/14/17 (умные указатели, regex, filesystem, random и др.).

Установка:
```bash
sudo apt install libboost-all-dev   # Debian/Ubuntu
sudo pacman -S boost                # Arch Linux
```

В CMakeLists.txt:
```cmake
find_package(Boost REQUIRED COMPONENTS filesystem regex program_options)
target_link_libraries(bayan PRIVATE
    Boost::filesystem Boost::regex Boost::program_options)
```

---

## Boost.Filesystem

Позволяет работать с файловой системой переносимо:

```cpp
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

fs::path p = "/home/user/docs";
for (const auto& entry : fs::directory_iterator(p)) {
    if (fs::is_regular_file(entry.path())) {
        std::cout << entry.path().filename().string()
                  << " (" << fs::file_size(entry.path()) << " bytes)\n";
    }
}
```

Ключевые функции:
- `fs::is_directory(p)` / `fs::is_regular_file(p)` — тип записи
- `fs::file_size(p)` — размер файла в байтах
- `fs::canonical(p, ec)` — абсолютный путь без `.` и `..`
- `fs::equivalent(p1, p2)` — одинаковый ли физический файл

---

## Boost.Regex

Расширенные регулярные выражения (C++11 std::regex — урезанный аналог):

```cpp
#include <boost/regex.hpp>

boost::regex re(".*\\.txt", boost::regex::icase);  // *.txt без учёта регистра
std::string filename = "README.TXT";
if (boost::regex_match(filename, re)) {
    std::cout << "Совпало!\n";
}
```

В проекте маски файлов (`*.txt`, `file?.cpp`) преобразуются в regex:
```cpp
static boost::regex maskToRegex(const std::string& mask)
{
    std::string pattern;
    for (char ch : mask) {
        if (ch == '*')      pattern += ".*";
        else if (ch == '?') pattern += ".";
        else if (std::string(".^$+{}[]|()/\\").find(ch) != std::string::npos)
            pattern += std::string("\\") + ch;  // экранируем спецсимволы
        else               pattern += ch;
    }
    return boost::regex(pattern, boost::regex::icase);
}
```

---

## Boost.ProgramOptions

Разбор аргументов командной строки:

```cpp
#include <boost/program_options.hpp>
namespace po = boost::program_options;

po::options_description desc("Параметры");
desc.add_options()
    ("help,h",   "Справка")
    ("dirs,d",   po::value<std::vector<std::string>>(&dirs)->multitoken(),
                 "Директории для сканирования")
    ("level,l",  po::value<int>(&level)->default_value(-1),
                 "Глубина сканирования");

po::variables_map vm;
po::store(po::parse_command_line(argc, argv, desc), vm);
po::notify(vm);

if (vm.count("help")) { std::cout << desc; return 0; }
```

Пример вызова программы:
```bash
./bayan --dirs /home/user/docs --level 2 --mask "*.txt" --hash md5
```

---

## Boost.CRC — хеширование блоков

CRC32 — быстрая контрольная сумма для обнаружения изменений:

```cpp
#include <boost/crc.hpp>

boost::crc_32_type crc;
crc.process_bytes(data, size);
std::uint32_t checksum = crc.checksum();
```

---

## Алгоритм поиска дублей (поблочное хеширование)

Ключевая идея — **ленивое чтение**: блоки файлов считываются только тогда, когда есть с кем сравнивать.

**Шаг 1.** Группируем файлы по размеру — файлы разного размера не могут быть дублями.

**Шаг 2.** Внутри группы итеративно делим на подгруппы по хешу блока 0, затем блока 1 и т.д.:

```
Группа по размеру: [A, B, C, D]
  → хеш блока 0: {hash1: [A, B], hash2: [C, D]}
  → хеш блока 1: {hash3: [A, B]} (C и D разошлись)
  → хеш блока 2: [A, B] совпали во всех блоках → дубли
```

```cpp
for (std::size_t blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
    std::map<std::string, std::vector<const FileEntry*>> byHash;
    for (const FileEntry* fe : sub)
        byHash[fe->blockHash(blockIdx)].push_back(fe);  // ленивое чтение
    // оставляем только подгруппы с >=2 файлами
}
```

Хеши кешируются в `FileEntry::hashes` — каждый блок читается максимум один раз.

---

## Структура проекта

| Файл | Ответственность |
|------|-----------------|
| `hasher.h/cpp` | Вычисление CRC32 / MD5 для блока данных |
| `scanner.h/cpp` | Обход директорий, фильтрация по маскам, сборка `FileEntry` |
| `duplicates.h/cpp` | Поблочный алгоритм поиска дублей |
| `main.cpp` | Разбор аргументов (Boost.ProgramOptions), вывод результата |

---

## Краткая сводка для защиты

### Ключевые определения

- **Boost** — набор портируемых C++ библиотек. Многие вошли в C++11/17.
- **Boost.Filesystem** — работа с файлами/директориями переносимо.
- **Boost.Regex** — регулярные выражения с расширенным синтаксисом.
- **Boost.ProgramOptions** — типизированный разбор аргументов командной строки.
- **CRC32** — быстрая 32-битная контрольная сумма (не криптографическая).
- **MD5** — 128-битный хеш (быстрее SHA, достаточно для сравнения файлов).
- **Ленивое вычисление (lazy evaluation)** — блок файла читается только при необходимости сравнения.
- **Поблочное хеширование** — сравниваем файлы не целиком, а по блокам фиксированного размера.

### Как работает программа

1. `main` разбирает аргументы через Boost.ProgramOptions, заполняет `ScanOptions`.
2. `scanFiles` обходит директории через Boost.Filesystem, фильтрует по маскам (Boost.Regex) и минимальному размеру.
3. Для каждого файла создаётся `FileEntry` с пустым кешем хешей.
4. `findDuplicates` группирует `FileEntry` по размеру, затем поблочно сужает группы.
5. Хеш блока (`blockHash`) вычисляется лениво — при первом обращении блок читается с диска и кешируется.
6. Группы из ≥2 файлов, прошедшие все блоки, выводятся как дубли.

### Типичные вопросы на защите

**Q: Зачем группировать по размеру перед хешированием?**
A: Файлы разного размера заведомо не дубли. Это быстрая первичная фильтрация без чтения содержимого.

**Q: Зачем ленивое чтение блоков?**
A: Если два файла уже различаются на блоке 0, блок 1 читать не нужно. Это экономит дисковый I/O.

**Q: Чем CRC32 отличается от MD5?**
A: CRC32 — 4 байта, быстрее, выше вероятность коллизии. MD5 — 16 байт, медленнее, практически без коллизий. Для сравнения файлов обычно достаточно CRC32; MD5 надёжнее.

**Q: Что такое `mutable` в `FileEntry::hashes`?**
A: Позволяет изменять поле в `const`-методе. `blockHash` объявлен `const` (не меняет логическое состояние объекта), но физически обновляет кеш при первом чтении.

**Q: Зачем `boost::regex::icase` при проверке масок?**
A: Маски файлов сравниваются без учёта регистра — `*.TXT` и `*.txt` должны давать одинаковый результат.

**Q: Как добавить новый алгоритм хеширования (SHA256)?**
A: Добавить значение в `enum class HashAlgo`, реализовать функцию в `hasher.cpp`, добавить ветку в `hashBlock` и строку в `parseHashAlgo`. `Scanner` и `findDuplicates` не меняются.
