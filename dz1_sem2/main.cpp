#include "sequence.h"
#include "personnel.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool readLine(const std::string& prompt, std::string& value)
{
    std::cout << prompt;
    return static_cast<bool>(std::getline(std::cin, value));
}

template<class T>
bool readValue(const std::string& prompt, T& value)
{
    std::string line;
    while (readLine(prompt, line)) {
        std::istringstream input(line);
        T candidate;
        if (input >> candidate) {
            input >> std::ws;
            if (input.eof()) {
                value = candidate;
                return true;
            }
        }
        std::cout << "Некорректное значение. Повторите ввод.\n";
    }
    return false;
}

bool readChoice(const std::string& prompt, int low, int high, int& choice)
{
    while (readValue(prompt, choice)) {
        if (choice >= low && choice <= high)
            return true;
        std::cout << "Выберите число от " << low << " до " << high << ".\n";
    }
    return false;
}

bool readPath(const std::string& prompt, std::string& path)
{
    while (readLine(prompt, path)) {
        if (!path.empty())
            return true;
        std::cout << "Имя файла не должно быть пустым.\n";
    }
    return false;
}

void showStatus(bool ok, const std::string& error)
{
    if (ok)
        std::cout << "Готово.\n";
    else
        std::cout << "Ошибка: " << error << '\n';
}

template<class Container>
void printContainer(const std::string& title, const Container& data)
{
    std::cout << title << '\n';
    for (typename Container::const_iterator it = data.begin(); it != data.end(); ++it)
        std::cout << *it << ' ';
    std::cout << '\n';
}

template<class InputContainer, class ResultContainer>
void processContainer(const std::string& inputPath, int modifyMode)
{
    int loadMode;
    if (!readChoice("Загрузка: 1 — файловый поток, 2 — copy и итераторы: ", 1, 2, loadMode))
        return;
    InputContainer source;
    std::string error;
    bool ok = loadMode == 1
        ? sequence::loadDataStreams(inputPath, source, error)
        : sequence::loadDataAlgorithms(inputPath, source, error);
    if (!ok) {
        showStatus(false, error);
        return;
    }
    printContainer("Исходные числа:", source);
    ResultContainer result;
    if (modifyMode == 1)
        ok = sequence::modifyContainer(source, result, error);
    else if (modifyMode == 2) {
        ok = sequence::modifyIterators(source.begin(), source.end(),
                                       std::back_inserter(result), error);
        if (ok)
            ok = sequence::appendStatistics(result, error);
    } else
        ok = sequence::modifyForEach(source, result, error);
    if (!ok) {
        showStatus(false, error);
        return;
    }
    printContainer("Результат (последние два числа — сумма и среднее модулей):", result);
    int outputMode;
    std::string outputPath;
    if (!readChoice("Сохранение: 1 — файловый поток, 2 — copy и итераторы: ", 1, 2, outputMode)
        || !readPath("Новый файл результата: ", outputPath))
        return;
    ok = outputMode == 1
        ? sequence::outputResultStreams(outputPath, result, error)
        : sequence::outputResultAlgorithms(outputPath, result, error);
    showStatus(ok, error);
}

void processNumbers()
{
    std::string inputPath;
    int method;
    if (!readPath("Файл исходных чисел: ", inputPath)
        || !readChoice("Обработка: 1 — контейнер, 2 — итераторы, 3 — for_each, 4 — потоковый transform: ",
                       1, 4, method))
        return;
    if (method == 4) {
        std::string outputPath;
        std::string error;
        if (readPath("Новый файл результата: ", outputPath))
            showStatus(sequence::modifyFile(inputPath, outputPath, error), error);
        return;
    }
    int container;
    if (!readChoice("Контейнер: 1 — vector, 2 — deque, 3 — list: ", 1, 3, container))
        return;
    if (container == 1)
        processContainer<std::vector<int>, std::vector<double> >(inputPath, method);
    else if (container == 2)
        processContainer<std::deque<int>, std::deque<double> >(inputPath, method);
    else
        processContainer<std::list<int>, std::list<double> >(inputPath, method);
}

void showTextFile()
{
    std::string path;
    if (!readPath("Текстовый файл: ", path))
        return;
    std::ifstream file(path.c_str());
    if (!file) {
        std::cout << "Не удалось открыть файл.\n";
        return;
    }
    std::string line;
    while (std::getline(file, line))
        std::cout << line << '\n';
    if (file.bad())
        std::cout << "Ошибка чтения файла.\n";
}

void sequenceMenu()
{
    int choice;
    while (true) {
        std::cout << "\nЧасть 1. Вариант 9\n"
                  << "1. Создать файл обычными потоками\n"
                  << "2. Создать файл через generate и copy\n"
                  << "3. Загрузить и преобразовать данные\n"
                  << "4. Просмотреть текстовый файл\n"
                  << "0. Назад\n";
        if (!readChoice("> ", 0, 4, choice) || choice == 0)
            return;
        if (choice == 1 || choice == 2) {
            std::string path;
            std::string error;
            if (!readPath("Новый файл для 100 чисел [-50, 50]: ", path))
                return;
            const bool ok = choice == 1
                ? sequence::createDataStreams(path, error)
                : sequence::createDataAlgorithms(path, error);
            showStatus(ok, error);
        } else if (choice == 3)
            processNumbers();
        else
            showTextFile();
    }
}

bool readDate(personnel::Date& date)
{
    std::string line;
    while (readLine("Дата приёма (день месяц год): ", line)) {
        std::istringstream input(line);
        if (input >> date.day >> date.month >> date.year) {
            input >> std::ws;
            if (input.eof())
                return true;
        }
        std::cout << "Введите три целых числа через пробел.\n";
    }
    return false;
}

bool readPerson(personnel::Person& person)
{
    person = personnel::Person();
    std::string name;
    if (!readValue("Табельный номер: ", person.id)
        || !readValue("Номер отдела: ", person.department)
        || !readLine("Фамилия, имя, отчество: ", name))
        return false;
    if (name.empty() || name.size() >= sizeof(person.fullName)) {
        std::cout << "ФИО должно занимать от 1 до 127 байт UTF-8. Запись не добавлена.\n";
        return false;
    }
    std::strcpy(person.fullName, name.c_str());
    if (!readValue("Пол (M/F): ", person.sex)
        || !readValue("Оклад: ", person.salary)
        || !readDate(person.hired)
        || !readValue("Надбавка, %: ", person.bonusPercent)
        || !readValue("Подоходный налог, сумма: ", person.incomeTax)
        || !readValue("Отработано дней: ", person.workedDays)
        || !readValue("Рабочих дней в месяце: ", person.workingDays)
        || !readValue("Начислено: ", person.accrued)
        || !readValue("Удержано: ", person.withheld))
        return false;
    std::string error;
    if (!personnel::validatePerson(person, error)) {
        showStatus(false, error);
        return false;
    }
    return true;
}

void printPerson(const personnel::Person& person)
{
    std::cout << person.id << " | отдел " << person.department << " | "
              << person.fullName << " | " << person.sex << " | приём "
              << person.hired.day << '.' << person.hired.month << '.' << person.hired.year
              << " | оклад " << person.salary << " | надбавка " << person.bonusPercent
              << "% | налог " << person.incomeTax << " | дни " << person.workedDays
              << '/' << person.workingDays << " | начислено " << person.accrued
              << " | удержано " << person.withheld
              << " | к выплате " << person.accrued - person.withheld << '\n';
}

void searchPersonnel(const personnel::Database& database)
{
    int criterion;
    std::string output;
    std::string error;
    if (!readChoice("Поиск: 1 — отдел, 2 — пол, 3 — дата приёма: ", 1, 3, criterion)
        || !readPath("Новый бинарный файл выборки: ", output))
        return;
    bool ok = false;
    if (criterion == 1) {
        int department;
        if (!readValue("Номер отдела: ", department))
            return;
        ok = database.searchDepartment(department, output, error);
    } else if (criterion == 2) {
        char sex;
        if (!readValue("Пол (M/F): ", sex))
            return;
        ok = database.searchSex(sex, output, error);
    } else {
        personnel::Date date;
        if (!readDate(date))
            return;
        ok = database.searchDate(date, output, error);
    }
    showStatus(ok, error);
    if (ok)
        std::cout << "Выборку можно открыть через пункт 2; исходная база сохранена.\n";
}

void personnelMenu()
{
    personnel::Database database;
    int choice;
    while (true) {
        std::cout << "\nЧасть 2. Задача 29 — сотрудники\n"
                  << "Текущая база: " << (database.isOpen() ? database.path() : "не открыта") << '\n'
                  << "1. Создать новую базу\n2. Открыть базу\n3. Показать сотрудников\n"
                  << "4. Добавить сотрудника\n5. Заменить запись\n6. Удалить запись\n"
                  << "7. Поиск с сохранением выборки\n8. Текстовая ведомость\n0. Назад\n";
        if (!readChoice("> ", 0, 8, choice) || choice == 0)
            return;
        std::string error;
        if (choice == 1 || choice == 2) {
            std::string path;
            if (!readPath(choice == 1 ? "Новый бинарный файл: " : "Бинарный файл: ", path))
                return;
            if (choice == 2)
                showStatus(database.open(path, error), error);
            else {
                int count;
                if (!readChoice("Количество сотрудников (0..10000): ", 0, 10000, count))
                    return;
                std::vector<personnel::Person> records;
                bool valid = true;
                for (int i = 0; i < count; ++i) {
                    std::cout << "Сотрудник " << i + 1 << '\n';
                    personnel::Person record;
                    if (!readPerson(record)) {
                        valid = false;
                        break;
                    }
                    records.push_back(record);
                }
                if (valid)
                    showStatus(database.create(path, records, error), error);
            }
            continue;
        }
        if (!database.isOpen()) {
            std::cout << "Сначала создайте или откройте базу.\n";
            continue;
        }
        if (choice == 3) {
            std::vector<personnel::Person> records;
            if (!database.readAll(records, error))
                showStatus(false, error);
            else {
                for (std::vector<personnel::Person>::const_iterator it = records.begin();
                     it != records.end(); ++it)
                    printPerson(*it);
                std::cout << "Всего сотрудников: " << records.size() << '\n';
            }
        } else if (choice == 4) {
            personnel::Person record;
            if (readPerson(record))
                showStatus(database.append(record, error), error);
        } else if (choice == 5 || choice == 6) {
            int id;
            if (!readValue("Табельный номер существующей записи: ", id))
                return;
            personnel::Person old;
            if (!database.get(id, old, error)) {
                showStatus(false, error);
                continue;
            }
            printPerson(old);
            if (choice == 6)
                showStatus(database.erase(id, error), error);
            else {
                personnel::Person record;
                if (readPerson(record))
                    showStatus(database.replace(id, record, error), error);
            }
        } else if (choice == 7)
            searchPersonnel(database);
        else {
            std::string output;
            if (readPath("Новый текстовый файл ведомости: ", output))
                showStatus(database.report(output, error), error);
        }
    }
}

} // namespace

int main()
{
    std::srand(static_cast<unsigned int>(std::time(0)));
    std::cout.precision(12);
    int choice;
    while (true) {
        std::cout << "\nДЗ №1, семестр 2, вариант 9\n"
                  << "1. Последовательные контейнеры\n"
                  << "2. Ассоциативные контейнеры и сотрудники\n0. Выход\n";
        if (!readChoice("> ", 0, 2, choice) || choice == 0)
            return 0;
        if (choice == 1)
            sequenceMenu();
        else
            personnelMenu();
    }
}
