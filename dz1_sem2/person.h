#ifndef DZ1_PERSON_H
#define DZ1_PERSON_H

namespace personnel {

struct Date {
    int day;
    int month;
    int year;
};

// Только поля фиксированного размера: в файл записывается весь объект Person.
struct Person {
    int id;
    int department;
    char fullName[128];
    char sex;
    double salary;
    Date hired;
    double bonusPercent;
    double incomeTax;       // Сумма подоходного налога, а не процент.
    int workedDays;
    int workingDays;
    double accrued;
    double withheld;
};

} // namespace personnel

#endif
