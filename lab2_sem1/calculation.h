#ifndef CALCULATION_H
#define CALCULATION_H

#include <QString>

// Вычисления отделены от виджетов, чтобы их можно было проверять независимо.
class Calculation {
public:
    Calculation();
    QString text() const;
    QString hint() const;
    bool hasError() const;
    void clear();
    void clearEntry();
    void digit(const QString& digit);
    void decimalPoint();
    void backspace();
    void changeSign();
    void operation(const QString& operation);
    void equals();
    void function(const QString& name, bool degrees);
    void constant(const QString& name);

private:
    QString entry_;
    QString pending_;
    QString repeated_;
    QString error_;
    double accumulator_;
    double repeatedOperand_;
    bool replaceEntry_;
    bool waitingOperand_;

    void startInput();
    bool setResult(double value);
    bool calculate(double left, const QString& operation, double right, double& result);
    void fail(const QString& message);
};

#endif
