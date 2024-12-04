#ifndef SELECTIONWINDOW_H
#define SELECTIONWINDOW_H

#include <QWidget>
#include "authorizationwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class SelectionWindow; // Декларація класу Ui::SelectionWindow
}
QT_END_NAMESPACE

class SelectionWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SelectionWindow(AuthorizationWindow* pAuthWin, QWidget *parent = nullptr);
    ~SelectionWindow();

private slots:
    void on_pushBtnSocket_clicked();
    void on_pushBtnPipe_clicked();

private:
    Ui::SelectionWindow *ui;
    AuthorizationWindow *m_pAuthWin;   // Вказівник на вікно авторизації

    QString strSyncMethod;             // Змінна для методу зв'язку
};

#endif // SELECTIONWINDOW_H
