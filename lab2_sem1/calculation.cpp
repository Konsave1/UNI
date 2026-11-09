#include "calculation.h"

#include <cmath>

namespace {
const double pi = std::acos(-1.0);
}

Calculation::Calculation()
{
    clear();
}

void Calculation::clear()
{
    entry_ = "0";
    pending_.clear();
    repeated_.clear();
    error_.clear();
    accumulator_ = 0;
    repeatedOperand_ = 0;
    replaceEntry_ = true;
    waitingOperand_ = false;
}

QString Calculation::text() const
{
    return error_.isEmpty() ? entry_ : error_;
}

QString Calculation::hint() const
{
    if (hasError())
        return QString::fromUtf8("Введите новое число или нажмите C");
    if (!pending_.isEmpty())
        return QString::number(accumulator_, 'g', 15) + " " + pending_;
    return QString::fromUtf8("Последовательное вычисление; процент = x / 100");
}

bool Calculation::hasError() const
{
    return !error_.isEmpty();
}

void Calculation::fail(const QString& message)
{
    error_ = message;
    pending_.clear();
    repeated_.clear();
    replaceEntry_ = true;
    waitingOperand_ = false;
}

void Calculation::startInput()
{
    if (hasError())
        clear();
    if (replaceEntry_) {
        entry_ = "0";
        replaceEntry_ = false;
        if (pending_.isEmpty())
            repeated_.clear();
    }
    waitingOperand_ = false;
}

void Calculation::digit(const QString& digit)
{
    if (digit.size() != 1 || digit < "0" || digit > "9")
        return;
    startInput();
    QString digits = entry_;
    digits.remove('-');
    digits.remove('.');
    if (digits.size() >= 16)
        return;
    if (entry_ == "0")
        entry_ = digit;
    else if (entry_ == "-0")
        entry_ = "-" + digit;
    else
        entry_ += digit;
}

void Calculation::decimalPoint()
{
    startInput();
    if (!entry_.contains('.'))
        entry_ += '.';
}

void Calculation::clearEntry()
{
    if (hasError()) {
        clear();
        return;
    }
    entry_ = "0";
    repeated_.clear();
    replaceEntry_ = true;
    waitingOperand_ = false;
}

void Calculation::backspace()
{
    if (hasError() || replaceEntry_) {
        clearEntry();
        return;
    }
    entry_.chop(1);
    if (entry_.isEmpty() || entry_ == "-")
        entry_ = "0";
}

void Calculation::changeSign()
{
    if (hasError())
        return;
    if (entry_.startsWith('-'))
        entry_.remove(0, 1);
    else
        entry_.prepend('-');
    waitingOperand_ = false;
    repeated_.clear();
    // Оставляем научную запись результата целой; её нельзя редактировать цифрами.
    if (entry_ == "-0" || entry_ == "0")
        replaceEntry_ = false;
}

bool Calculation::setResult(double value)
{
    if (!std::isfinite(value)) {
        fail(QString::fromUtf8("Переполнение или недопустимый результат"));
        return false;
    }
    if (value == 0)
        value = 0; // Не выводить -0.
    entry_ = QString::number(value, 'g', 15);
    replaceEntry_ = true;
    waitingOperand_ = false;
    return true;
}

bool Calculation::calculate(double left, const QString& operation,
                            double right, double& result)
{
    if (operation == "+")
        result = left + right;
    else if (operation == "-")
        result = left - right;
    else if (operation == "*")
        result = left * right;
    else if (operation == "/") {
        if (right == 0) {
            fail(QString::fromUtf8("Деление на ноль"));
            return false;
        }
        result = left / right;
    } else if (operation == "^") {
        if ((left == 0 && right <= 0) ||
            (left < 0 && std::floor(right) != right)) {
            fail(QString::fromUtf8("Недопустимая степень"));
            return false;
        }
        result = std::pow(left, right);
    } else {
        fail(QString::fromUtf8("Неизвестная операция"));
        return false;
    }
    if (!std::isfinite(result)) {
        fail(QString::fromUtf8("Переполнение или недопустимый результат"));
        return false;
    }
    return true;
}

void Calculation::operation(const QString& operation)
{
    if (hasError())
        return;
    if (operation != "+" && operation != "-" && operation != "*" &&
        operation != "/" && operation != "^")
        return;
    const double value = entry_.toDouble();
    if (!pending_.isEmpty() && !waitingOperand_) {
        double result = 0;
        if (!calculate(accumulator_, pending_, value, result) || !setResult(result))
            return;
        accumulator_ = result;
    } else if (pending_.isEmpty())
        accumulator_ = value;
    pending_ = operation;
    repeated_.clear();
    replaceEntry_ = true;
    waitingOperand_ = true;
}

void Calculation::equals()
{
    if (hasError())
        return;
    double result = 0;
    if (!pending_.isEmpty()) {
        const double right = entry_.toDouble();
        if (!calculate(accumulator_, pending_, right, result))
            return;
        repeated_ = pending_;
        repeatedOperand_ = right;
        pending_.clear();
        setResult(result);
    } else if (!repeated_.isEmpty()) {
        if (!calculate(entry_.toDouble(), repeated_, repeatedOperand_, result))
            return;
        setResult(result);
    } else
        replaceEntry_ = true;
}

void Calculation::constant(const QString& name)
{
    if (hasError())
        clear();
    if (name == "pi")
        setResult(pi);
    else if (name == "e")
        setResult(std::exp(1.0));
    repeated_.clear();
}

void Calculation::function(const QString& name, bool degrees)
{
    if (hasError())
        return;
    const double value = entry_.toDouble();
    double result = 0;
    const double angle = degrees ? value * pi / 180.0 : value;
    if (name == "sqrt") {
        if (value < 0) {
            fail(QString::fromUtf8("Корень из отрицательного числа"));
            return;
        }
        result = std::sqrt(value);
    } else if (name == "reciprocal") {
        if (value == 0) {
            fail(QString::fromUtf8("Деление на ноль"));
            return;
        }
        result = 1.0 / value;
    } else if (name == "square")
        result = value * value;
    else if (name == "percent")
        result = value / 100.0;
    else if (name == "abs")
        result = std::fabs(value);
    else if (name == "sin")
        result = std::sin(angle);
    else if (name == "cos")
        result = std::cos(angle);
    else if (name == "tan") {
        if (std::fabs(std::cos(angle)) < 1e-12) {
            fail(QString::fromUtf8("Тангенс не определён"));
            return;
        }
        result = std::tan(angle);
    } else if (name == "asin" || name == "acos") {
        if (value < -1 || value > 1) {
            fail(QString::fromUtf8("Аргумент должен быть от -1 до 1"));
            return;
        }
        result = name == "asin" ? std::asin(value) : std::acos(value);
        if (degrees)
            result *= 180.0 / pi;
    } else if (name == "atan") {
        result = std::atan(value);
        if (degrees)
            result *= 180.0 / pi;
    } else if (name == "ln" || name == "log") {
        if (value <= 0) {
            fail(QString::fromUtf8("Логарифм требует x > 0"));
            return;
        }
        result = name == "ln" ? std::log(value) : std::log10(value);
    } else if (name == "exp")
        result = std::exp(value);
    else if (name == "power10")
        result = std::pow(10.0, value);
    else if (name == "factorial") {
        if (value < 0 || value > 170 || std::floor(value) != value) {
            fail(QString::fromUtf8("Факториал: целое число от 0 до 170"));
            return;
        }
        result = 1;
        for (int i = 2; i <= value; ++i)
            result *= i;
    } else {
        fail(QString::fromUtf8("Неизвестная функция"));
        return;
    }
    repeated_.clear();
    setResult(result);
}
