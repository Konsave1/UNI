#ifndef DZ1_PERSONNEL_H
#define DZ1_PERSONNEL_H

#include "person.h"

#include <ios>
#include <map>
#include <string>
#include <vector>

namespace personnel {

bool validatePerson(const Person& person, std::string& error);

class Database {
public:
    bool open(const std::string& filename, std::string& error);
    bool create(const std::string& filename,
                const std::vector<Person>& people, std::string& error);
    bool append(const Person& person, std::string& error);
    bool replace(int id, const Person& person, std::string& error);
    bool erase(int id, std::string& error);
    bool readAll(std::vector<Person>& people, std::string& error) const;
    bool searchDepartment(int department, const std::string& output,
                          std::string& error) const;
    bool searchSex(char sex, const std::string& output,
                   std::string& error) const;
    bool searchDate(const Date& date, const std::string& output,
                    std::string& error) const;
    bool report(const std::string& output, std::string& error) const;
    bool get(int id, Person& person, std::string& error) const;
    const std::string& path() const;
    bool isOpen() const;

private:
    std::string path_;
    std::map<int, std::streampos> index_;

    bool rewrite(const std::vector<Person>& people, std::string& error);
};

} // namespace personnel

#endif
