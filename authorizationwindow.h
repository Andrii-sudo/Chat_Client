#ifndef AUTHORIZATIONWINDOW_H
#define AUTHORIZATIONWINDOW_H

#include <QDialog>
#include "mainwindow.h"

#include <windows.h>

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
    void on_btnLogin_clicked();

    void on_btnSignUp_clicked();

private:
    Ui::AuthorizationWindow *ui;
    bool bIsLogin;

    SOCKET connectToServer(const std::string& strIp, const std::string& strPort);
};

#endif // AUTHORIZATIONWINDOW_H
