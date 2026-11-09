#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "calculation.h"
#include <QWidget>

class QPlainTextEdit;
class QLabel;
class QRadioButton;
class QGridLayout;
class QKeyEvent;

class Calculator : public QWidget {
    Q_OBJECT

public:
    explicit Calculator(QWidget* parent = 0);

protected:
    void keyPressEvent(QKeyEvent* event);

private slots:
    void slotButton();
    void slotMode(bool scientific);

private:
    Calculation calculation_;
    QPlainTextEdit* display_;
    QLabel* hint_;
    QWidget* scientificPanel_;
    QRadioButton* degrees_;
    void addButton(QGridLayout* grid, const QString& title, const QString& name,
                   int row, int column, const QString& shortcut = QString());
    void refreshDisplay();
};

#endif
