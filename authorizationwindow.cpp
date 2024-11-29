#include "authorizationwindow.h"
#include "ui_authorizationwindow.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

#include <QMessageBox>
#include <QRegularExpressionValidator>

#define DEFAULT_PORT "12345"

AuthorizationWindow::AuthorizationWindow(MainWindow* pMainWin, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AuthorizationWindow)
{
    m_pMainWin = pMainWin;

    ui->setupUi(this);

    // Створюємо регулярний вираз, який забороняє пробіли
    QRegularExpression regex("[^\\s]*");  // Будь-які символи, окрім пробілів
    QValidator* validator = new QRegularExpressionValidator(regex, this);

    // Призначаємо валідатор для QLineEdit
    ui->txtName->setValidator(validator);
    ui->txtPassword->setValidator(validator);


    bIsLogin = true;

    WSADATA wsaData;

    // Initialize Winsock
    int	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        QMessageBox::critical(this, "Error", "WSAStartup failed: " + QString::number(iResult));
        exit(1);
    }
}

AuthorizationWindow::~AuthorizationWindow()
{
    delete ui;
    WSACleanup();
}

SOCKET AuthorizationWindow::connectToServer(const std::string& strIp, const std::string& strPort)
{
    struct addrinfo *result = NULL,
        *ptr = NULL,
        hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int	iResult = getaddrinfo(strIp.c_str(), strPort.c_str(), &hints, &result);
    if (iResult != 0)
    {
        QMessageBox::critical(this, "Error", "getaddrinfo failed: " + QString::number(iResult));
        return INVALID_SOCKET;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    ptr = result;
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

    if (ConnectSocket == INVALID_SOCKET)
    {
        QMessageBox::critical(this, "Error", "Error at socket(): %ld\n" + QString::number(WSAGetLastError()));
        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    iResult = ::connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        QMessageBox::critical(this, "Error", "Unable to connect to server: " + QString::number(WSAGetLastError()));
        closesocket(ConnectSocket); // Закриття сокета
        freeaddrinfo(result);       // Очищення результату
        return INVALID_SOCKET;
    }

    freeaddrinfo(result);

    return ConnectSocket;
}

void AuthorizationWindow::on_btnSignIn_clicked()
{
    if(ui->txtName->text().isEmpty() || ui->txtPassword->text().isEmpty())
    {
        QMessageBox::information(this, "Input Error", "Both username and password fields must be filled.");
        return;
    }

    SOCKET socket = connectToServer("127.0.0.1", DEFAULT_PORT);
    if (socket == INVALID_SOCKET)
    {
        return;
    }

    std::string strName = ui->txtName->text().toStdString();
    std::string strPassword = ui->txtPassword->text().toStdString();


    std::string strInfo = std::string((bIsLogin) ? "log" : "reg") + " " + strName + " " + strPassword;

    int iResult;

    iResult = send(socket, strInfo.c_str(), strInfo.length(), 0);
    if (iResult == SOCKET_ERROR)
    {
        QMessageBox::critical(this, "Error", "send failed: " + QString::number(WSAGetLastError()));
        closesocket(socket);
        return;
    }

    const int RECVBUF_SIZE = 20;
    std::vector<char> vecRecvBuf(RECVBUF_SIZE);

    // Receive data until the server closes the connection

    iResult = recv(socket, vecRecvBuf.data(), vecRecvBuf.size(), 0);
    if (iResult > 0)
    {
        if(vecRecvBuf[0] == 'Y')
        {
            if(bIsLogin)
            {
                QMessageBox::information(this, "Success", "Login successful!");
            }
            else
            {
                QMessageBox::information(this, "Success", "Account created successfully!");
            }

            this->hide();

            m_pMainWin->setName(strName);
            m_pMainWin->show();
        }
        else if(vecRecvBuf[0] == 'N')
        {
            if(bIsLogin)
            {
                QMessageBox::information(this, "Error", "Incorrect username or password");
            }
            else
            {
                QMessageBox::information(this, "Error", "User with this name already exists");
            }
        }
    }
    else if (iResult == 0)
    {
        QMessageBox::information(this, "Error", "Connection closed");
    }
    else
    {
        QMessageBox::information(this, "Error", "recv failed: " + QString::number(WSAGetLastError()));
    }

    closesocket(socket);
}

void AuthorizationWindow::on_btnSignUp_clicked()
{
    if(bIsLogin)
    {
        bIsLogin = false;
        ui->labLogin->setText("Create account");
        ui->btnSignIn->setText("Sign up");
        ui->labHaveAcc->setText("Allready have an account?");
        ui->btnSignUp->setText("Sign in");
    }
    else
    {
        bIsLogin = true;
        ui->labLogin->setText("Login");
        ui->btnSignIn->setText("Sign in");
        ui->labHaveAcc->setText("Don't have account?");
        ui->btnSignUp->setText("Sign up");
    }
}

