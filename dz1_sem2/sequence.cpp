#include "sequence.h"

#include <cstdlib>
#include <vector>

namespace sequence {
namespace detail {

bool openNewOutput(const std::string& path, std::ofstream& output,
                   std::string& error)
{
    std::ifstream existing(path.c_str());
    if (existing.is_open()) {
        error = "Файл уже существует. Выберите новое имя: " + path;
        return false;
    }
    output.open(path.c_str());
    if (!output) {
        error = "Не удалось создать выходной файл: " + path;
        return false;
    }
    output.precision(17);
    return true;
}

bool finishOutput(std::ofstream& output, std::string& error)
{
    output.close();
    if (!output) {
        error = "Ошибка записи выходного файла.";
        return false;
    }
    return true;
}

} // namespace detail

bool createDataStreams(const std::string& path, std::string& error)
{
    error.clear();
    std::ofstream output;
    if (!detail::openNewOutput(path, output, error)) {
        return false;
    }
    for (int index = 0; index < 100; ++index) {
        output << std::rand() % 101 - 50 << '\n';
    }
    return detail::finishOutput(output, error);
}

bool createDataAlgorithms(const std::string& path, std::string& error)
{
    error.clear();
    std::ofstream output;
    if (!detail::openNewOutput(path, output, error)) {
        return false;
    }
    std::vector<int> data(100);
    std::generate(data.begin(), data.end(), []() {
        return std::rand() % 101 - 50;
    });
    std::copy(data.begin(), data.end(), std::ostream_iterator<int>(output, "\n"));
    return detail::finishOutput(output, error);
}

bool modifyFile(const std::string& inputPath, const std::string& outputPath,
                std::string& error)
{
    error.clear();
    if (inputPath == outputPath) {
        error = "Имена входного и выходного файлов должны различаться.";
        return false;
    }

    int multiplier = 0;
    bool hasNegative = false;
    // До создания результата проверяем ВСЕ токены и запоминаем множитель.
    if (!detail::inspectFile(inputPath, multiplier, hasNegative, error)) {
        return false;
    }
    if (!hasNegative) {
        error = "В исходных данных нет отрицательного числа.";
        return false;
    }
    std::ifstream input(inputPath.c_str());
    if (!input) {
        error = "Не удалось повторно открыть входной файл: " + inputPath;
        return false;
    }
    std::ofstream output;
    if (!detail::openNewOutput(outputPath, output, error)) {
        return false;
    }

    double sum = 0;
    std::size_t count = 0;
    std::transform(std::istream_iterator<int>(input), std::istream_iterator<int>(),
                   std::ostream_iterator<double>(output, "\n"),
                   [&](int value) {
        const double modified = value % 2 == 0 ? value * multiplier : value;
        sum += modified < 0 ? -modified : modified;
        ++count;
        return modified;
    });
    if (input.bad() || !input.eof() || count == 0) {
        error = "Ошибка повторного чтения входного файла: " + inputPath;
        return false;
    }
    output << sum << '\n' << sum / count << '\n';
    return detail::finishOutput(output, error);
}

} // namespace sequence
