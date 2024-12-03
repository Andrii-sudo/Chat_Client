#ifndef SELECTIONWINDOW_H
#define SELECTIONWINDOW_H

#include <QWidget>
#include "authorizationwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class SelectionWindow; // Декларація класу Ui::SelectionWindow
}
QT_END_NAMESPACE

class SelectionWindow : public QWidget {
    Q_OBJECT

public:
    explicit SelectionWindow(QWidget *parent = nullptr);
    ~SelectionWindow();

private slots:
    void onPushBtnSocketClicked();
    void onPushBtnPipeClicked();

private:
    Ui::SelectionWindow *ui;
    AuthorizationWindow *authWindow;  // Вказівник на вікно авторизації
    QString commMethod;               // Змінна для методу зв'язку
};

#endif // SELECTIONWINDOW_H
