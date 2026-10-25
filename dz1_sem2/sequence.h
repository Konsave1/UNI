#ifndef DZ1_SEM2_SEQUENCE_H
#define DZ1_SEM2_SEQUENCE_H

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

namespace sequence {

bool createDataStreams(const std::string& path, std::string& error);
bool createDataAlgorithms(const std::string& path, std::string& error);
bool modifyFile(const std::string& inputPath, const std::string& outputPath,
                std::string& error);

namespace detail {

// Выходные файлы создаются только под новыми именами.
bool openNewOutput(const std::string& path, std::ofstream& output,
                   std::string& error);
bool finishOutput(std::ofstream& output, std::string& error);

template<class Value>
bool parseValue(const std::string& token, Value& value, std::string& error)
{
    std::string::size_type pos = 0;
    if (!token.empty() && (token[0] == '+' || token[0] == '-')) {
        ++pos;
    }
    if (pos == token.size()) {
        error = "Некорректное целое число: " + token;
        return false;
    }
    for (; pos < token.size(); ++pos) {
        if (token[pos] < '0' || token[pos] > '9') {
            error = "Некорректное целое число: " + token;
            return false;
        }
    }

    std::istringstream parser(token);
    if (!(parser >> value)) {
        error = "Переполнение при чтении целого числа: " + token;
        return false;
    }
    if (parser.peek() != std::char_traits<char>::eof()) {
        error = "Некорректное целое число: " + token;
        return false;
    }
    if (value < -50 || value > 50) {
        error = "Число вне диапазона [-50; 50]: " + token;
        return false;
    }
    return true;
}

// Полностью проверяет файл, не сохраняя его содержимое в контейнере.
template<class Value>
bool inspectFile(const std::string& path, Value& multiplier,
                 bool& hasNegative, std::string& error)
{
    std::ifstream input(path.c_str());
    if (!input) {
        error = "Не удалось открыть входной файл: " + path;
        return false;
    }

    bool hasValues = false;
    hasNegative = false;
    std::string token;
    while (input >> token) {
        Value value = Value();
        if (!parseValue(token, value, error)) {
            return false;
        }
        hasValues = true;
        if (!hasNegative && value < Value()) {
            multiplier = value;
            hasNegative = true;
        }
    }
    if (input.bad() || !input.eof()) {
        error = "Ошибка чтения входного файла: " + path;
        return false;
    }
    if (!hasValues) {
        error = "Входной файл пуст.";
        return false;
    }
    return true;
}

template<class InputIterator>
bool getMultiplier(InputIterator first, InputIterator last,
                   typename std::iterator_traits<InputIterator>::value_type& multiplier,
                   std::string& error)
{
    typedef typename std::iterator_traits<InputIterator>::value_type Value;
    if (first == last) {
        error = "Исходная последовательность пуста.";
        return false;
    }
    for (InputIterator pos = first; pos != last; ++pos) {
        if (*pos < -50 || *pos > 50) {
            error = "Исходное число вне диапазона [-50; 50].";
            return false;
        }
    }
    InputIterator negative = std::find_if(first, last, [](const Value& value) {
        return value < Value();
    });
    if (negative == last) {
        error = "В исходных данных нет отрицательного числа.";
        return false;
    }
    // Копия необходима, даже если первый отрицательный элемент сам чётный.
    multiplier = *negative;
    return true;
}

} // namespace detail

template<class Container>
bool loadDataStreams(const std::string& path, Container& data,
                     std::string& error)
{
    error.clear();
    typedef typename Container::value_type Value;
    std::ifstream input(path.c_str());
    if (!input) {
        error = "Не удалось открыть входной файл: " + path;
        return false;
    }

    Container loaded;
    std::string token;
    while (input >> token) {
        Value value = Value();
        if (!detail::parseValue(token, value, error)) {
            return false;
        }
        loaded.push_back(value);
    }
    if (input.bad() || !input.eof()) {
        error = "Ошибка чтения входного файла: " + path;
        return false;
    }
    if (loaded.empty()) {
        error = "Входной файл пуст.";
        return false;
    }
    data.swap(loaded);
    return true;
}

template<class Container>
bool loadDataAlgorithms(const std::string& path, Container& data,
                        std::string& error)
{
    error.clear();
    typedef typename Container::value_type Value;
    Value multiplier = Value();
    bool hasNegative = false;
    // Потоковый итератор не отличает чистый EOF от overflow в последнем токене.
    if (!detail::inspectFile(path, multiplier, hasNegative, error)) {
        return false;
    }

    std::ifstream input(path.c_str());
    if (!input) {
        error = "Не удалось повторно открыть входной файл: " + path;
        return false;
    }
    Container loaded;
    std::copy(std::istream_iterator<Value>(input), std::istream_iterator<Value>(),
              std::back_inserter(loaded));
    if (input.bad() || !input.eof()) {
        error = "Ошибка чтения входного файла: " + path;
        return false;
    }
    data.swap(loaded);
    return true;
}

// Container должен хранить вещественные значения, чтобы среднее не усекалось.
// Итоги относятся только к значениям, находившимся в контейнере до вызова.
template<class Container>
bool appendStatistics(Container& data, std::string& error)
{
    error.clear();
    typedef typename Container::value_type Value;
    if (data.empty()) {
        error = "Нельзя вычислить среднее для пустой последовательности.";
        return false;
    }
    Value sum = Value();
    const typename Container::size_type count = data.size();
    for (typename Container::const_iterator pos = data.begin(); pos != data.end(); ++pos) {
        sum += *pos < Value() ? -*pos : *pos;
    }
    const Value average = sum / static_cast<Value>(count);
    data.push_back(sum);
    data.push_back(average);
    return true;
}

template<class InputContainer, class ResultContainer>
bool modifyContainer(const InputContainer& input, ResultContainer& result,
                     std::string& error)
{
    error.clear();
    typedef typename InputContainer::value_type InputValue;
    typedef typename ResultContainer::value_type ResultValue;
    InputValue multiplier = InputValue();
    if (!detail::getMultiplier(input.begin(), input.end(), multiplier, error)) {
        return false;
    }

    ResultContainer modified;
    for (typename InputContainer::const_iterator pos = input.begin(); pos != input.end(); ++pos) {
        const InputValue value = *pos;
        modified.push_back(static_cast<ResultValue>(value % 2 == 0 ? value * multiplier : value));
    }
    if (!appendStatistics(modified, error)) {
        return false;
    }
    result.swap(modified);
    return true;
}

// first и last задают многопроходный диапазон целых чисел.
// Здесь записываются только преобразованные значения; итоги добавляет вызывающий код.
template<class InputIterator, class OutputIterator>
bool modifyIterators(InputIterator first, InputIterator last,
                     OutputIterator output, std::string& error)
{
    error.clear();
    typedef typename std::iterator_traits<InputIterator>::value_type Value;
    Value multiplier = Value();
    if (!detail::getMultiplier(first, last, multiplier, error)) {
        return false;
    }
    for (; first != last; ++first, ++output) {
        const Value value = *first;
        *output = value % 2 == 0 ? value * multiplier : value;
    }
    return true;
}

template<class InputContainer, class ResultContainer>
bool modifyForEach(const InputContainer& input, ResultContainer& result,
                   std::string& error)
{
    error.clear();
    typedef typename InputContainer::value_type InputValue;
    typedef typename ResultContainer::value_type ResultValue;
    InputValue multiplier = InputValue();
    if (!detail::getMultiplier(input.begin(), input.end(), multiplier, error)) {
        return false;
    }

    ResultContainer modified;
    std::for_each(input.begin(), input.end(), [&](const InputValue& value) {
        modified.push_back(static_cast<ResultValue>(value % 2 == 0 ? value * multiplier : value));
    });
    if (!appendStatistics(modified, error)) {
        return false;
    }
    result.swap(modified);
    return true;
}

template<class Container>
bool outputResultStreams(const std::string& path, const Container& data,
                         std::string& error)
{
    error.clear();
    std::ofstream output;
    if (!detail::openNewOutput(path, output, error)) {
        return false;
    }
    for (typename Container::const_iterator pos = data.begin(); pos != data.end(); ++pos) {
        output << *pos << '\n';
    }
    return detail::finishOutput(output, error);
}

template<class Container>
bool outputResultAlgorithms(const std::string& path, const Container& data,
                            std::string& error)
{
    error.clear();
    typedef typename Container::value_type Value;
    std::ofstream output;
    if (!detail::openNewOutput(path, output, error)) {
        return false;
    }
    std::copy(data.begin(), data.end(), std::ostream_iterator<Value>(output, "\n"));
    return detail::finishOutput(output, error);
}

} // namespace sequence

#endif
