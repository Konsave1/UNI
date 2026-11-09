#include "calculation.h"
#include "calculator.h"

#include <QApplication>
#include <QDir>
#include <QFontInfo>
#include <QEventLoop>
#include <QKeyEvent>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTimer>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
int checks = 0;

void require(bool ok, const char* message)
{
    ++checks;
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void enter(Calculation& calculation, QString text)
{
    const bool negative = text.startsWith('-');
    if (negative)
        text.remove(0, 1);
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) == '.')
            calculation.decimalPoint();
        else
            calculation.digit(QString(text.at(i)));
    }
    if (negative)
        calculation.changeSign();
}

void number(const Calculation& calculation, double expected, const char* message)
{
    bool ok = false;
    const double actual = calculation.text().toDouble(&ok);
    const double scale = std::fabs(expected) > 1 ? std::fabs(expected) : 1;
    require(!calculation.hasError() && ok && std::isfinite(actual) &&
            std::fabs(actual - expected) < scale * 1e-11, message);
}

void testArithmetic()
{
    Calculation c;
    number(c, 0, "initial zero");
    const char* operations[] = {"+", "-", "*", "/", "^"};
    const double expected[] = {11, 5, 24, 8.0 / 3, 512};
    for (int i = 0; i < 5; ++i) {
        c.clear(); enter(c, "8"); c.operation(operations[i]); enter(c, "3"); c.equals();
        number(c, expected[i], "binary arithmetic");
    }
    c.clear(); enter(c, "2"); c.operation("+"); enter(c, "3");
    c.operation("*"); enter(c, "4"); c.equals();
    number(c, 20, "sequential calculation");
    c.equals(); number(c, 80, "repeated equals");
    enter(c, "7"); c.equals(); number(c, 7, "new input resets repeated equals");
    c.clear(); enter(c, "9"); c.operation("+"); c.operation("*"); enter(c, "2"); c.equals();
    number(c, 18, "replace pending operation");
    c.clear(); enter(c, "5"); c.operation("+"); c.equals();
    number(c, 10, "equals with missing second operand");
    c.clear(); enter(c, "5"); c.operation("+"); enter(c, "9"); c.function("sqrt", true);
    c.operation("*"); enter(c, "2"); c.equals();
    number(c, 16, "unary result is ready operand for pending operation");
    c.clear(); enter(c, "5"); c.operation("+"); c.constant("pi"); c.equals();
    number(c, 5 + std::acos(-1.0), "constant as second operand");
    c.clear(); enter(c, "9"); c.operation("+"); enter(c, "123"); c.clearEntry();
    enter(c, "2"); c.equals(); number(c, 11, "CE preserves pending operation");
    c.clear(); enter(c, "9"); c.operation("+"); c.clear(); enter(c, "2"); c.equals();
    number(c, 2, "C clears pending operation");
    std::cout << "PASS arithmetic and calculator state\n";
}

void testInput()
{
    Calculation c;
    enter(c, "0.125"); c.decimalPoint(); enter(c, "0"); number(c, 0.125, "one decimal point");
    c.clear(); c.changeSign(); enter(c, "7"); number(c, -7, "negative entry from zero");
    c.changeSign(); number(c, 7, "sign toggle");
    c.clear(); enter(c, "123"); c.backspace(); number(c, 12, "backspace");
    c.backspace(); c.backspace(); number(c, 0, "backspace to zero");
    c.clear(); enter(c, "99999999999999999");
    require(c.text().size() == 16, "input precision limit");
    c.clear(); enter(c, "1"); c.operation("/"); enter(c, "3"); c.equals();
    c.digit("2"); number(c, 2, "typing after result replaces it");
    c.clear(); enter(c, "200"); c.function("percent", true); number(c, 2, "percent is x/100");
    std::cout << "PASS numeric entry, editing and percent\n";
}

void testFunctions()
{
    Calculation c;
    const char* functions[] = {"sqrt", "reciprocal", "square", "abs", "factorial",
                                "log", "ln", "exp", "power10", "asin", "acos", "atan"};
    const char* inputs[] = {"9", "4", "-3", "-7", "5", "100", "1", "0", "3", "0.5", "0.5", "1"};
    const double expected[] = {3, 0.25, 9, 7, 120, 2, 0, 1, 1000, 30, 60, 45};
    for (int i = 0; i < 12; ++i) {
        c.clear(); enter(c, inputs[i]); c.function(functions[i], true);
        number(c, expected[i], "unary scientific function");
    }
    c.clear(); enter(c, "0"); c.function("factorial", true); number(c, 1, "zero factorial");
    c.clear(); enter(c, "170"); c.function("factorial", true);
    require(!c.hasError() && std::isfinite(c.text().toDouble()), "170 factorial finite");
    c.clear(); enter(c, "30"); c.function("sin", true); number(c, 0.5, "sin degrees");
    c.clear(); enter(c, "60"); c.function("cos", true); number(c, 0.5, "cos degrees");
    c.clear(); enter(c, "45"); c.function("tan", true); number(c, 1, "tan degrees");
    c.clear(); c.constant("pi"); c.operation("/"); enter(c, "2"); c.equals();
    c.function("sin", false); number(c, 1, "sin radians");
    c.clear(); enter(c, "0.5"); c.function("asin", false);
    number(c, std::acos(-1.0) / 6, "inverse trig radians");
    c.clear(); c.constant("e"); c.function("ln", true); number(c, 1, "e constant");
    std::cout << "PASS scientific functions, constants and angles\n";
}

void testErrors()
{
    Calculation c;
    const char* functions[] = {"sqrt", "reciprocal", "ln", "log", "asin", "acos",
                                "factorial", "factorial", "factorial", "tan", "exp", "power10"};
    const char* inputs[] = {"-1", "0", "0", "-1", "2", "-2", "-1", "1.5", "171", "90", "1000", "309"};
    for (int i = 0; i < 12; ++i) {
        c.clear(); enter(c, inputs[i]); c.function(functions[i], true);
        require(c.hasError(), "invalid domain or overflow rejected");
        c.digit("4"); number(c, 4, "digit recovers from error");
    }
    c.clear(); enter(c, "1"); c.operation("/"); enter(c, "0"); c.equals();
    require(c.hasError(), "division by zero");
    c.equals(); require(c.hasError(), "equals preserves error");
    c.clearEntry(); number(c, 0, "CE recovers from error");
    c.clear(); enter(c, "-2"); c.operation("^"); enter(c, "0.5"); c.equals();
    require(c.hasError(), "fractional power of negative");
    c.clear(); enter(c, "0"); c.operation("^"); enter(c, "0"); c.equals();
    require(c.hasError(), "zero power zero policy");
    c.clear(); enter(c, "10"); c.operation("^"); enter(c, "309"); c.equals();
    require(c.hasError(), "binary overflow");
    std::cout << "PASS domains, overflow and error recovery\n";
}

QPushButton* button(Calculator& window, const char* name)
{
    QPushButton* found = window.findChild<QPushButton*>(name);
    require(found != 0, "button exists");
    return found;
}

void click(Calculator& window, const char* name)
{
    button(window, name)->click();
    QApplication::processEvents();
}

void testWindow(const QString& screenshots)
{
    Calculator window;
    window.show();
    QApplication::processEvents();
    QPlainTextEdit* display = window.findChild<QPlainTextEdit*>("display");
    QRadioButton* normal = window.findChild<QRadioButton*>("normal_mode");
    QRadioButton* scientific = window.findChild<QRadioButton*>("scientific_mode");
    QRadioButton* radians = window.findChild<QRadioButton*>("radians");
    QWidget* panel = window.findChild<QWidget*>("scientific_panel");
    require(display && normal && scientific && radians && panel, "required widgets exist");
    require(normal->isChecked() && !panel->isVisible(), "normal mode initially");
    require(window.windowTitle().contains(QString::fromUtf8("(Обычный)")), "normal title");
    require(display->isReadOnly() && display->font().fixedPitch(), "read-only monospace display");
    const int normalWidth = window.width();
    click(window, "digit_2"); click(window, "add"); click(window, "digit_3"); click(window, "equals");
    require(display->toPlainText() == "5", "buttons connected to arithmetic");
    if (!screenshots.isEmpty())
        require(window.grab().save(screenshots + "/normal.png"), "save normal screenshot");
    scientific->click(); QApplication::processEvents();
    require(panel->isVisible() && scientific->isChecked(), "scientific panel visible");
    require(window.width() > normalWidth, "window expands in scientific mode");
    require(window.windowTitle().contains(QString::fromUtf8("(Инженерный)")), "scientific title");
    require(display->toPlainText() == "5", "switch preserves result");
    click(window, "clear_all"); click(window, "digit_3"); click(window, "digit_0"); click(window, "sin");
    require(display->toPlainText().toDouble() == 0.5, "scientific button degrees");
    radians->click(); QApplication::processEvents();
    require(scientific->isChecked() && panel->isVisible(), "angle buttons independent from mode");
    click(window, "clear_all"); click(window, "pi"); click(window, "divide");
    click(window, "digit_2"); click(window, "equals"); click(window, "sin");
    require(std::fabs(display->toPlainText().toDouble() - 1) < 1e-11, "UI radians");
    if (!screenshots.isEmpty())
        require(window.grab().save(screenshots + "/scientific.png"), "save scientific screenshot");
    normal->click(); QApplication::processEvents();
    require(!panel->isVisible() && window.width() == normalWidth, "window shrinks back");
    click(window, "clear_all");
    window.activateWindow();
    window.setFocus();
    QApplication::processEvents();
    QKeyEvent press(QEvent::KeyPress, Qt::Key_7, Qt::NoModifier, "7");
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_7, Qt::NoModifier, "7");
    QApplication::sendEvent(&window, &press);
    QApplication::sendEvent(&window, &release);
    // Клавиатурное сочетание QPushButton вызывает короткую анимацию нажатия.
    QEventLoop events;
    QTimer::singleShot(180, &events, SLOT(quit()));
    events.exec();
    require(display->toPlainText() == "7", "digit keyboard shortcut");
    click(window, "divide"); click(window, "digit_0"); click(window, "equals");
    require(display->toPlainText().contains(QString::fromUtf8("ноль")), "error shown in display");
    click(window, "digit_8"); require(display->toPlainText() == "8", "UI error recovery");
    std::cout << "PASS real widgets, signals, modes, geometry and keyboard\n";
}
}

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QString screenshots;
    if (argc > 1) {
        screenshots = QString::fromLocal8Bit(argv[1]);
        require(QDir().mkpath(screenshots), "create screenshot directory");
    }
    testArithmetic();
    testInput();
    testFunctions();
    testErrors();
    testWindow(screenshots);
    std::cout << "PASS all " << checks << " checks\n";
    return 0;
}
