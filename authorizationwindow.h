#ifndef AUTHORIZATIONWINDOW_H
#define AUTHORIZATIONWINDOW_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class AuthorizationWindow;
}

class AuthorizationWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AuthorizationWindow(MainWindow* pMainWin, QWidget *parent = nullptr);
    ~AuthorizationWindow();

private slots:
    void on_btnSignIn_clicked();
    void on_btnSignUp_clicked();

private:
    Ui::AuthorizationWindow* ui;

    // Змінна, яка визначає чи хоче користувач авторизуватися, чи зареєструватися
    bool m_bIsLogin;

    MainWindow* m_pMainWin;

    SOCKET connectToServer(const std::string& strIp, const std::string& strPort);
};

#endif // AUTHORIZATIONWINDOW_H
