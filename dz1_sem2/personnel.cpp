#include "personnel.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace personnel {
namespace {

bool validDate(const Date& date) {
    if (date.year < 1 || date.year > 9999 ||
        date.month < 1 || date.month > 12) {
        return false;
    }
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = date.year % 400 == 0 ||
                (date.year % 4 == 0 && date.year % 100 != 0);
    if (leap) {
        days[1] = 29;
    }
    return date.day >= 1 && date.day <= days[date.month - 1];
}

bool sameDate(const Date& first, const Date& second) {
    return first.day == second.day && first.month == second.month &&
           first.year == second.year;
}

bool nonnegative(double value) {
    return std::isfinite(value) && value >= 0;
}

bool fileExists(const std::string& filename) {
    std::ifstream input(filename.c_str(), std::ios::binary);
    return input.is_open();
}

bool newFilename(const std::string& filename, const std::string& source,
                 std::string& error) {
    if (filename.empty()) {
        error = "Имя нового файла не должно быть пустым.";
        return false;
    }
    if (!source.empty() && filename == source) {
        error = "Новый файл не должен совпадать с файлом базы.";
        return false;
    }
    if (fileExists(filename)) {
        error = "Файл уже существует. Укажите новое имя: " + filename;
        return false;
    }
    return true;
}

void removeIncomplete(const std::string& filename, std::string& error) {
    if (std::remove(filename.c_str()) != 0) {
        error += " Не удалось удалить незавершённый файл: " + filename;
    }
}

// read/write всегда работают с целым объектом Person.
bool readFile(const std::string& filename, std::vector<Person>& people,
              std::map<int, std::streampos>& index, std::string& error) {
    std::ifstream input(filename.c_str(), std::ios::binary);
    if (!input) {
        error = "Не удалось открыть бинарный файл: " + filename;
        return false;
    }
    std::vector<Person> loaded;
    std::map<int, std::streampos> loadedIndex;
    while (true) {
        std::streampos position = input.tellg();
        Person person = {};
        input.read(reinterpret_cast<char*>(&person), sizeof(Person));
        if (input.gcount() == 0 && input.eof() && !input.bad()) {
            break;
        }
        if (input.gcount() != static_cast<std::streamsize>(sizeof(Person))) {
            error = "Файл повреждён: неполная запись Person или ошибка чтения.";
            return false;
        }
        if (!input || position == std::streampos(-1)) {
            error = "Ошибка чтения бинарного файла: " + filename;
            return false;
        }
        if (!validatePerson(person, error)) {
            error = "Некорректная запись в файле: " + error;
            return false;
        }
        if (loadedIndex.find(person.id) != loadedIndex.end()) {
            error = "Файл содержит повторяющийся табельный номер.";
            return false;
        }
        loadedIndex[person.id] = position;
        loaded.push_back(person);
    }
    people.swap(loaded);
    index.swap(loadedIndex);
    return true;
}

bool writeNewFile(const std::string& filename,
                  const std::vector<Person>& people, std::string& error) {
    if (!newFilename(filename, "", error)) {
        return false;
    }
    std::ofstream output(filename.c_str(), std::ios::binary);
    if (!output) {
        error = "Не удалось создать бинарный файл: " + filename;
        return false;
    }
    for (std::vector<Person>::const_iterator it = people.begin();
         it != people.end(); ++it) {
        output.write(reinterpret_cast<const char*>(&*it), sizeof(Person));
        if (!output) {
            break;
        }
    }
    output.close();
    if (!output) {
        error = "Не удалось полностью записать бинарный файл: " + filename;
        removeIncomplete(filename, error);
        return false;
    }
    return true;
}

std::string dateText(const Date& date) {
    std::ostringstream text;
    text << date.day << '.' << date.month << '.' << date.year;
    return text.str();
}

} // namespace

bool validatePerson(const Person& person, std::string& error) {
    error.clear();
    if (person.id <= 0 || person.department <= 0) {
        error = "Табельный номер и номер отдела должны быть положительными.";
    } else if (person.sex != 'M' && person.sex != 'F') {
        error = "Пол должен быть M или F.";
    } else if (!validDate(person.hired)) {
        error = "Некорректная дата поступления на работу.";
    } else if (!nonnegative(person.salary) ||
               !nonnegative(person.bonusPercent) ||
               !nonnegative(person.incomeTax) ||
               !nonnegative(person.accrued) || !nonnegative(person.withheld)) {
        error = "Оклад, надбавка, налог, начислено и удержано должны быть "
                "конечными неотрицательными числами.";
    } else if (person.workingDays < 1 || person.workingDays > 31 ||
               person.workedDays < 0 || person.workedDays > person.workingDays) {
        error = "Рабочих дней должно быть от 1 до 31; отработанных — "
                "от 0 до количества рабочих дней.";
    } else if (person.withheld > person.accrued) {
        error = "Удержанная сумма не может превышать начисленную.";
    }
    if (!error.empty()) {
        return false;
    }
    bool terminated = false;
    bool hasName = false;
    for (unsigned int i = 0; i < sizeof(person.fullName); ++i) {
        if (person.fullName[i] == '\0') {
            terminated = true;
            break;
        }
        if (person.fullName[i] != ' ' && person.fullName[i] != '\t' &&
            person.fullName[i] != '\r' && person.fullName[i] != '\n') {
            hasName = true;
        }
        if (person.fullName[i] == '\r' || person.fullName[i] == '\n' ||
            person.fullName[i] == '\t') {
            error = "ФИО должно занимать одну строку без табуляции.";
            return false;
        }
    }
    if (!terminated || !hasName) {
        error = "ФИО должно быть непустым и содержать не более 127 байт.";
        return false;
    }
    return true;
}

bool Database::open(const std::string& filename, std::string& error) {
    error.clear();
    std::vector<Person> people;
    std::map<int, std::streampos> newIndex;
    if (!readFile(filename, people, newIndex, error)) {
        return false;
    }
    path_ = filename;
    index_.swap(newIndex);
    return true;
}

bool Database::create(const std::string& filename,
                      const std::vector<Person>& people, std::string& error) {
    error.clear();
    if (!newFilename(filename, path_, error)) {
        return false;
    }
    std::map<int, bool> ids;
    for (std::vector<Person>::const_iterator it = people.begin();
         it != people.end(); ++it) {
        if (!validatePerson(*it, error)) {
            return false;
        }
        if (ids.find(it->id) != ids.end()) {
            error = "Табельные номера сотрудников должны быть уникальны.";
            return false;
        }
        ids[it->id] = true;
    }
    if (!writeNewFile(filename, people, error)) {
        return false;
    }
    if (!open(filename, error)) {
        removeIncomplete(filename, error);
        return false;
    }
    return true;
}

bool Database::readAll(std::vector<Person>& people, std::string& error) const {
    error.clear();
    if (!isOpen()) {
        error = "Сначала откройте или создайте базу сотрудников.";
        return false;
    }
    std::map<int, std::streampos> index;
    return readFile(path_, people, index, error);
}

bool Database::get(int id, Person& person, std::string& error) const {
    error.clear();
    if (!isOpen()) {
        error = "Сначала откройте или создайте базу сотрудников.";
        return false;
    }
    std::map<int, std::streampos>::const_iterator it = index_.find(id);
    if (it == index_.end()) {
        error = "Сотрудник с таким табельным номером не найден.";
        return false;
    }
    std::ifstream input(path_.c_str(), std::ios::binary);
    if (!input) {
        error = "Не удалось открыть базу сотрудников.";
        return false;
    }
    input.seekg(it->second);
    Person loaded = {};
    input.read(reinterpret_cast<char*>(&loaded), sizeof(Person));
    if (!input || input.gcount() != static_cast<std::streamsize>(sizeof(Person))) {
        error = "Не удалось прочитать запись по индексу.";
        return false;
    }
    if (!validatePerson(loaded, error)) {
        return false;
    }
    if (loaded.id != id) {
        error = "Файл изменён вне программы. Откройте базу заново.";
        return false;
    }
    person = loaded;
    return true;
}

bool Database::append(const Person& person, std::string& error) {
    error.clear();
    if (!validatePerson(person, error)) {
        return false;
    }
    std::vector<Person> people;
    std::map<int, std::streampos> currentIndex;
    if (!isOpen()) {
        error = "Сначала откройте или создайте базу сотрудников.";
        return false;
    }
    if (!readFile(path_, people, currentIndex, error)) {
        return false;
    }
    if (currentIndex.find(person.id) != currentIndex.end()) {
        error = "Сотрудник с таким табельным номером уже существует.";
        return false;
    }
    std::ofstream output(path_.c_str(), std::ios::binary | std::ios::app);
    if (!output) {
        error = "Не удалось открыть базу для дозаписи.";
        return false;
    }
    output.seekp(0, std::ios::end);
    std::streampos position = output.tellp();
    if (position == std::streampos(-1)) {
        error = "Не удалось определить позицию новой записи.";
        return false;
    }
    output.write(reinterpret_cast<const char*>(&person), sizeof(Person));
    output.close();
    if (!output) {
        error = "Ошибка дозаписи в базу. Проверьте целостность файла.";
        return false;
    }
    currentIndex[person.id] = position;
    index_.swap(currentIndex);
    return true;
}

bool Database::rewrite(const std::vector<Person>& people, std::string& error) {
    const std::string temporary = path_ + ".tmp";
    const std::string backup = path_ + ".bak";
    if (!newFilename(backup, path_, error)) {
        return false;
    }
    if (!writeNewFile(temporary, people, error)) {
        return false;
    }
    std::vector<Person> checked;
    std::map<int, std::streampos> newIndex;
    if (!readFile(temporary, checked, newIndex, error)) {
        removeIncomplete(temporary, error);
        return false;
    }
    // Переносимая замена: исходные данные остаются в резервном файле.
    if (std::rename(path_.c_str(), backup.c_str()) != 0) {
        error = "Не удалось создать резервный файл. Исходная база сохранена.";
        removeIncomplete(temporary, error);
        return false;
    }
    if (std::rename(temporary.c_str(), path_.c_str()) != 0) {
        if (std::rename(backup.c_str(), path_.c_str()) == 0) {
            error = "Не удалось заменить базу. Исходный файл восстановлен.";
            removeIncomplete(temporary, error);
        } else {
            // Индекс исходных записей подходит и к резервной копии.
            path_ = backup;
            error = "Не удалось заменить базу и восстановить имя исходного файла. "
                    "Исходные записи доступны в резервной базе: " + backup +
                    ". Новый файл сохранён отдельно: " + temporary;
        }
        return false;
    }
    index_.swap(newIndex);
    if (std::remove(backup.c_str()) != 0) {
        error = "Новая база сохранена, но не удалось удалить резервный файл: " +
                backup;
        return false;
    }
    return true;
}

bool Database::replace(int id, const Person& person, std::string& error) {
    error.clear();
    if (!validatePerson(person, error)) {
        return false;
    }
    std::vector<Person> people;
    if (!readAll(people, error)) {
        return false;
    }
    std::vector<Person>::iterator found = people.end();
    for (std::vector<Person>::iterator it = people.begin(); it != people.end(); ++it) {
        if (it->id == id) {
            found = it;
        } else if (it->id == person.id) {
            error = "Новый табельный номер уже занят другим сотрудником.";
            return false;
        }
    }
    if (found == people.end()) {
        error = "Сотрудник для замены не найден.";
        return false;
    }
    *found = person;
    return rewrite(people, error);
}

bool Database::erase(int id, std::string& error) {
    error.clear();
    std::vector<Person> people;
    if (!readAll(people, error)) {
        return false;
    }
    for (std::vector<Person>::iterator it = people.begin(); it != people.end(); ++it) {
        if (it->id == id) {
            people.erase(it);
            return rewrite(people, error);
        }
    }
    error = "Сотрудник для удаления не найден.";
    return false;
}

bool Database::searchDepartment(int department, const std::string& output,
                                std::string& error) const {
    error.clear();
    if (department <= 0) {
        error = "Номер отдела должен быть положительным.";
        return false;
    }
    if (!newFilename(output, path_, error)) {
        return false;
    }
    std::vector<Person> people;
    if (!readAll(people, error)) {
        return false;
    }
    std::vector<Person> selected;
    for (std::vector<Person>::const_iterator it = people.begin();
         it != people.end(); ++it) {
        if (it->department == department) {
            selected.push_back(*it);
        }
    }
    return writeNewFile(output, selected, error);
}

bool Database::searchSex(char sex, const std::string& output,
                         std::string& error) const {
    error.clear();
    if (sex != 'M' && sex != 'F') {
        error = "Пол должен быть M или F.";
        return false;
    }
    if (!newFilename(output, path_, error)) {
        return false;
    }
    std::vector<Person> people;
    if (!readAll(people, error)) {
        return false;
    }
    std::vector<Person> selected;
    for (std::vector<Person>::const_iterator it = people.begin();
         it != people.end(); ++it) {
        if (it->sex == sex) {
            selected.push_back(*it);
        }
    }
    return writeNewFile(output, selected, error);
}

bool Database::searchDate(const Date& date, const std::string& output,
                          std::string& error) const {
    error.clear();
    if (!validDate(date)) {
        error = "Некорректная дата для поиска.";
        return false;
    }
    if (!newFilename(output, path_, error)) {
        return false;
    }
    std::vector<Person> people;
    if (!readAll(people, error)) {
        return false;
    }
    std::vector<Person> selected;
    for (std::vector<Person>::const_iterator it = people.begin();
         it != people.end(); ++it) {
        if (sameDate(it->hired, date)) {
            selected.push_back(*it);
        }
    }
    return writeNewFile(output, selected, error);
}

bool Database::report(const std::string& output, std::string& error) const {
    error.clear();
    if (!newFilename(output, path_, error)) {
        return false;
    }
    std::vector<Person> people;
    if (!readAll(people, error)) {
        return false;
    }
    std::ofstream text(output.c_str());
    if (!text) {
        error = "Не удалось создать текстовую ведомость: " + output;
        return false;
    }
    text << "Ведомость сотрудников\n"
         << "Таб. номер\tОтдел\tФИО\tПол\tОклад\tДата приёма\tНадбавка, %\t"
         << "Подоходный налог (сумма)\tОтработано дней\tРабочих дней\t"
         << "Начислено\tУдержано\tК выплате\n";
    text.precision(12);
    long double totalAccrued = 0;
    long double totalWithheld = 0;
    for (std::vector<Person>::const_iterator it = people.begin();
         it != people.end(); ++it) {
        text << it->id << '\t' << it->department << '\t' << it->fullName << '\t'
             << it->sex << '\t' << it->salary << '\t' << dateText(it->hired) << '\t'
             << it->bonusPercent << '\t' << it->incomeTax << '\t'
             << it->workedDays << '\t' << it->workingDays << '\t'
             << it->accrued << '\t' << it->withheld << '\t'
             << it->accrued - it->withheld << '\n';
        totalAccrued += it->accrued;
        totalWithheld += it->withheld;
    }
    text << "\nКоличество сотрудников: " << people.size()
         << "\nИтого начислено: " << totalAccrued
         << "\nИтого удержано: " << totalWithheld
         << "\nИтого к выплате: " << totalAccrued - totalWithheld << '\n';
    text.close();
    if (!text) {
        error = "Не удалось полностью записать ведомость: " + output;
        removeIncomplete(output, error);
        return false;
    }
    return true;
}

const std::string& Database::path() const {
    return path_;
}

bool Database::isOpen() const {
    return !path_.empty();
}

} // namespace personnel
