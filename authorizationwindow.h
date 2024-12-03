#ifndef AUTHORIZATIONWINDOW_H
#define AUTHORIZATIONWINDOW_H

#include <QDialog>
#include "mainwindow.h"

class SelectionWindow; // Декларація класу SelectionWindow

namespace Ui {
class AuthorizationWindow;
}

class AuthorizationWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AuthorizationWindow(SelectionWindow* pSelectionWin, QWidget *parent = nullptr);
    ~AuthorizationWindow();

private slots:
    void on_btnSignIn_clicked();
    void on_btnSignUp_clicked();

private:
    Ui::AuthorizationWindow* ui;

    // Змінна, яка визначає, чи хоче користувач авторизуватися, чи зареєструватися
    bool m_bIsLogin;
    SelectionWindow* m_pMainWin;
    SelectionWindow* m_pSelectionWin; // Зв'язок із SelectionWindow

    SOCKET connectToServer(const std::string& strIp, const std::string& strPort);
};

#endif // AUTHORIZATIONWINDOW_H

