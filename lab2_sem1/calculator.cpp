#include "calculator.h"

#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QKeyEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

Calculator::Calculator(QWidget* parent) : QWidget(parent)
{
    setObjectName("calculator");
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    QWidget* modes = new QWidget(this);
    QHBoxLayout* modeLayout = new QHBoxLayout(modes);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    QRadioButton* normal = new QRadioButton(QString::fromUtf8("Обычный"), modes);
    QRadioButton* scientific = new QRadioButton(QString::fromUtf8("Инженерный"), modes);
    normal->setObjectName("normal_mode");
    scientific->setObjectName("scientific_mode");
    normal->setChecked(true);
    modeLayout->addWidget(normal);
    modeLayout->addWidget(scientific);
    modeLayout->addStretch();
    mainLayout->addWidget(modes);

    display_ = new QPlainTextEdit(this);
    display_->setObjectName("display");
    display_->setReadOnly(true);
    display_->setFocusPolicy(Qt::NoFocus);
    display_->setFixedHeight(94);
    QFont font("Monospace", 21);
    font.setStyleHint(QFont::TypeWriter);
    font.setFixedPitch(true);
    display_->setFont(font);
    mainLayout->addWidget(display_);
    hint_ = new QLabel(this);
    hint_->setObjectName("hint");
    hint_->setWordWrap(true);
    hint_->setFixedHeight(42);
    mainLayout->addWidget(hint_);

    QHBoxLayout* panels = new QHBoxLayout;
    mainLayout->addLayout(panels);
    scientificPanel_ = new QWidget(this);
    scientificPanel_->setObjectName("scientific_panel");
    QVBoxLayout* scientificLayout = new QVBoxLayout(scientificPanel_);
    scientificLayout->setContentsMargins(0, 0, 0, 0);
    QWidget* angles = new QWidget(scientificPanel_);
    QHBoxLayout* angleLayout = new QHBoxLayout(angles);
    angleLayout->setContentsMargins(0, 0, 0, 0);
    degrees_ = new QRadioButton(QString::fromUtf8("Градусы"), angles);
    degrees_->setObjectName("degrees");
    QRadioButton* radians = new QRadioButton(QString::fromUtf8("Радианы"), angles);
    radians->setObjectName("radians");
    degrees_->setChecked(true);
    angleLayout->addWidget(degrees_);
    angleLayout->addWidget(radians);
    scientificLayout->addWidget(angles);
    QGridLayout* functions = new QGridLayout;
    scientificLayout->addLayout(functions);
    addButton(functions, "sin", "sin", 0, 0);
    addButton(functions, "cos", "cos", 0, 1);
    addButton(functions, "tan", "tan", 0, 2);
    addButton(functions, "asin", "asin", 1, 0);
    addButton(functions, "acos", "acos", 1, 1);
    addButton(functions, "atan", "atan", 1, 2);
    addButton(functions, "ln", "ln", 2, 0);
    addButton(functions, "log10", "log", 2, 1);
    addButton(functions, "exp", "exp", 2, 2);
    addButton(functions, "x^y", "power", 3, 0);
    addButton(functions, "10^x", "power10", 3, 1);
    addButton(functions, "n!", "factorial", 3, 2);
    addButton(functions, QString::fromUtf8("π"), "pi", 4, 0);
    addButton(functions, "e", "e", 4, 1);
    addButton(functions, "|x|", "abs", 4, 2);
    panels->addWidget(scientificPanel_);

    QWidget* standardPanel = new QWidget(this);
    QGridLayout* grid = new QGridLayout(standardPanel);
    grid->setContentsMargins(0, 0, 0, 0);
    addButton(grid, "CE", "clear_entry", 0, 0, "Delete");
    addButton(grid, "C", "clear_all", 0, 1, "Escape");
    addButton(grid, QString::fromUtf8("⌫"), "backspace", 0, 2, "Backspace");
    addButton(grid, QString::fromUtf8("±"), "sign", 0, 3);
    addButton(grid, "%", "percent", 1, 0);
    addButton(grid, "1/x", "reciprocal", 1, 1);
    addButton(grid, QString::fromUtf8("x²"), "square", 1, 2);
    addButton(grid, QString::fromUtf8("÷"), "divide", 1, 3, "/");
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            QString digit = QString::number(7 - row * 3 + column);
            addButton(grid, digit, "digit_" + digit, row + 2, column, digit);
        }
    }
    addButton(grid, QString::fromUtf8("×"), "multiply", 2, 3, "*");
    addButton(grid, QString::fromUtf8("−"), "subtract", 3, 3, "-");
    addButton(grid, "+", "add", 4, 3, "+");
    addButton(grid, "0", "digit_0", 5, 0, "0");
    addButton(grid, ".", "decimal", 5, 1, ".");
    addButton(grid, QString::fromUtf8("√"), "sqrt", 5, 2);
    addButton(grid, "=", "equals", 5, 3, "Return");
    panels->addWidget(standardPanel);

    connect(scientific, SIGNAL(toggled(bool)), this, SLOT(slotMode(bool)));
    slotMode(false);
    refreshDisplay();
}

void Calculator::addButton(QGridLayout* grid, const QString& title,
                           const QString& name, int row, int column,
                           const QString& shortcut)
{
    QPushButton* button = new QPushButton(title, grid->parentWidget());
    button->setObjectName(name);
    button->setMinimumSize(64, 43);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (!shortcut.isEmpty())
        button->setShortcut(QKeySequence(shortcut));
    grid->addWidget(button, row, column);
    connect(button, SIGNAL(clicked()), this, SLOT(slotButton()));
}

void Calculator::slotMode(bool scientific)
{
    scientificPanel_->setVisible(scientific);
    setWindowTitle(scientific
        ? QString::fromUtf8("Калькулятор (Инженерный)")
        : QString::fromUtf8("Калькулятор (Обычный)"));
    layout()->activate();
    adjustSize();
}

void Calculator::refreshDisplay()
{
    display_->setPlainText(calculation_.text());
    hint_->setText(calculation_.hint());
}

void Calculator::slotButton()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button)
        return;
    const QString name = button->objectName();
    if (name.startsWith("digit_"))
        calculation_.digit(name.right(1));
    else if (name == "decimal")
        calculation_.decimalPoint();
    else if (name == "clear_all")
        calculation_.clear();
    else if (name == "clear_entry")
        calculation_.clearEntry();
    else if (name == "backspace")
        calculation_.backspace();
    else if (name == "sign")
        calculation_.changeSign();
    else if (name == "equals")
        calculation_.equals();
    else if (name == "pi" || name == "e")
        calculation_.constant(name);
    else if (name == "add")
        calculation_.operation("+");
    else if (name == "subtract")
        calculation_.operation("-");
    else if (name == "multiply")
        calculation_.operation("*");
    else if (name == "divide")
        calculation_.operation("/");
    else if (name == "power")
        calculation_.operation("^");
    else
        calculation_.function(name, degrees_->isChecked());
    refreshDisplay();
}

// Специализированный обработчик события клавиатуры (лекция Qt 5).
void Calculator::keyPressEvent(QKeyEvent* event)
{
    QString name;
    const int key = event->key();
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        name = "digit_" + QString::number(key - Qt::Key_0);
    else if (key == Qt::Key_Period || key == Qt::Key_Comma)
        name = "decimal";
    else if (key == Qt::Key_Plus)
        name = "add";
    else if (key == Qt::Key_Minus)
        name = "subtract";
    else if (key == Qt::Key_Asterisk)
        name = "multiply";
    else if (key == Qt::Key_Slash)
        name = "divide";
    else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Equal)
        name = "equals";
    else if (key == Qt::Key_Backspace)
        name = "backspace";
    else if (key == Qt::Key_Escape)
        name = "clear_all";
    else if (key == Qt::Key_Delete)
        name = "clear_entry";
    QPushButton* button = name.isEmpty() ? 0 : findChild<QPushButton*>(name);
    if (button) {
        button->click();
        event->accept();
    } else
        QWidget::keyPressEvent(event);
}
