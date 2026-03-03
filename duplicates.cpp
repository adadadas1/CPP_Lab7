#include "duplicates.h"

#include <map>
#include <unordered_map>

// Найти дубли внутри группы файлов одинакового размера.
// Итеративно разбиваем группу по хешу каждого следующего блока.
// Блок читается только если в группе >=2 файла (ленивое чтение).
static std::vector<std::vector<const FileEntry*>>
findDuplicatesInGroup(std::vector<const FileEntry*> group)
{
    // Текущие подгруппы (начинаем с одной — все файлы вместе)
    std::vector<std::vector<const FileEntry*>> current = { group };
    std::vector<std::vector<const FileEntry*>> result;

    // Количество блоков одинаково для всех файлов в группе (одинаковый размер)
    const std::size_t numBlocks = group.empty() ? 0 : group[0]->numBlocks();

    for (std::size_t blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
        std::vector<std::vector<const FileEntry*>> nextRound;

        for (auto& sub : current) {
            if (sub.size() < 2) continue;  // одиночный файл — дублей нет

            // Делим подгруппу по хешу текущего блока
            // Здесь и происходит ленивое чтение — блок читается первый раз
            std::map<std::string, std::vector<const FileEntry*>> byHash;
            for (const FileEntry* fe : sub)
                byHash[fe->blockHash(blockIdx)].push_back(fe);

            for (auto& kv : byHash) {
                if (kv.second.size() >= 2)
                    nextRound.push_back(std::move(kv.second));
            }
        }

        current = std::move(nextRound);
        if (current.empty()) break;
    }

    // Всё что осталось в current после всех блоков — настоящие дубли
    for (auto& sub : current) {
        if (sub.size() >= 2)
            result.push_back(std::move(sub));
    }
    return result;
}

std::vector<std::vector<const FileEntry*>>
findDuplicates(const std::vector<FileEntry>& files)
{
    // Шаг 1: группируем по размеру файла
    std::map<std::uintmax_t, std::vector<const FileEntry*>> bySize;
    for (const auto& fe : files)
        bySize[fe.fileSize].push_back(&fe);

    // Шаг 2: для каждой группы с >=2 файлами ищем дубли поблочно
    std::vector<std::vector<const FileEntry*>> result;
    for (auto& kv : bySize) {
        if (kv.second.size() < 2) continue;

        auto dupes = findDuplicatesInGroup(kv.second);
        for (auto& grp : dupes)
            result.push_back(std::move(grp));
    }
    return result;
}
