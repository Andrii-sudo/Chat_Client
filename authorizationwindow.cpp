#include "authorizationwindow.h"
#include "ui_authorizationwindow.h"
#include "selectionwindow.h"


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
    ui->setupUi(this);

    m_pMainWin = pMainWin;

    m_bIsLogin = true;

    // Заборона введення пробілів у поля авторизації
    QRegularExpression regexDisableSpaces("[^\\s]*");
    QValidator* validDisableSpaces = new QRegularExpressionValidator(regexDisableSpaces, this);

    ui->txtName->setValidator(validDisableSpaces);
    ui->txtPassword->setValidator(validDisableSpaces);


    WSADATA wsaData;

    // Ініціалізація Winsock
    int  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
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

void AuthorizationWindow::setSyncMethod(QString strSynchMethod)
{
    m_strSynchMethod = strSynchMethod;
}


SOCKET AuthorizationWindow::connectToServer(const std::string& strIp, const std::string& strPort)
{
    struct addrinfo* result = NULL,
        *ptr = NULL,
        hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;     // Використовувати IPv4
    hints.ai_socktype = SOCK_STREAM; // Тип сокета: потік (TCP)
    hints.ai_protocol = IPPROTO_TCP; // Протокол: TCP

    // Визначення локальної адреси і порту для з'єднанням з сервером
    int  iResult = getaddrinfo(strIp.c_str(), strPort.c_str(), &hints, &result);
    if (iResult != 0)
    {
        QMessageBox::critical(this, "Error", "getaddrinfo failed: " + QString::number(iResult));
        return INVALID_SOCKET;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    ptr = result;

    // Створення сокету
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET)
    {
        QMessageBox::critical(this, "Error", "Error at socket(): %ld\n" + QString::number(WSAGetLastError()));
        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    // Підключення сокету до серверу
    iResult = ::connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        QMessageBox::critical(this, "Error", "Unable to connect to server: " + QString::number(WSAGetLastError()));
        closesocket(ConnectSocket);
        freeaddrinfo(result);
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

    if (m_strSynchMethod == "Socket")
    {
        // Створення сокету та підключення його до серверу
        SOCKET socket = connectToServer("127.0.0.1", DEFAULT_PORT);
        if (socket == INVALID_SOCKET)
        {
            return;
        }

        std::string strName = ui->txtName->text().toStdString();
        std::string strPassword = ui->txtPassword->text().toStdString();


        std::string strInfo = std::string((m_bIsLogin) ? "log" : "reg") + " " + strName + " " + strPassword;

        int iResult;

        // Надсилання запиту до сервера
        iResult = send(socket, strInfo.c_str(), strInfo.length(), 0);
        if (iResult == SOCKET_ERROR)
        {
            QMessageBox::critical(this, "Error", "send failed: " + QString::number(WSAGetLastError()));
            closesocket(socket);
            return;
        }

        const int RECVBUF_SIZE = 1;
        std::vector<char> vecRecvBuf(RECVBUF_SIZE);
        // Приймання відповіді від серверу ("Y" або "N")
        iResult = recv(socket, vecRecvBuf.data(), vecRecvBuf.size(), 0);
        if (iResult > 0)
        {
            if(vecRecvBuf[0] == 'Y')
            {
                if(m_bIsLogin)
                {
                    QMessageBox::information(this, "Success", "Login successful!");
                }
                else
                {
                    QMessageBox::information(this, "Success", "Account created successfully!");
                }


                this->hide();

                m_pMainWin->setSyncMethod("Socket");

                m_pMainWin->setName(strName);
                m_pMainWin->show();
            }
            else if(vecRecvBuf[0] == 'N')
            {
                if(m_bIsLogin)
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
    else if (m_strSynchMethod == "Pipe")
    {
        // Функціонал через пайпи
        HANDLE hPipe;

        // Змінюємо std::string на std::wstring для роботи з wide character string
        //std::wstring pipeName = L"\\\\.\\pipe\\AuthPipe"; // Ім'я пайпа у wide string форматі
        std::wstring pipeName = L"\\\\.\\pipe\\ServerPipe";

        // Відкриття пайпа
        hPipe = CreateFileW(
            pipeName.c_str(),              // Ім'я пайпа
            GENERIC_READ | GENERIC_WRITE,  // Права доступу
            0,                             // Немає спільного доступу
            NULL,                          // Без атрибутів безпеки
            OPEN_EXISTING,                 // Відкриваємо існуючий пайп
            0,                             // Додаткові атрибути
            NULL);                         // Без шаблону

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            QMessageBox::critical(this, "Error", "Failed to connect to pipe.");
            return;
        }

        std::string strName = ui->txtName->text().toStdString();
        std::string strPassword = ui->txtPassword->text().toStdString();
        std::string strInfo = std::string((m_bIsLogin) ? "log" : "reg") + " " + strName + " " + strPassword;

        DWORD bytesWritten;
        BOOL result;

        // Надсилання інформації через пайп
        result = WriteFile(hPipe, strInfo.c_str(), strInfo.length(), &bytesWritten, NULL);
        if (!result || bytesWritten == 0)
        {
            QMessageBox::critical(this, "Error", "Failed to send data to the pipe.");
            CloseHandle(hPipe);
            return;
        }

        char buffer[2];  // Буфер для отримання відповіді
        DWORD bytesRead;

        // Отримання відповіді від сервера
        result = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);
        if (result && bytesRead > 0)
        {
            if (buffer[0] == 'Y')
            {
                if (m_bIsLogin)
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
            else if (buffer[0] == 'N')
            {
                if (m_bIsLogin)
                {
                    QMessageBox::information(this, "Error", "Incorrect username or password");
                }
                else
                {
                    QMessageBox::information(this, "Error", "User with this name already exists");
                }
            }
        }
        else
        {
            QMessageBox::critical(this, "Error", "Failed to receive data from the pipe.");
        }

        // Закриття пайпа
        CloseHandle(hPipe);
    }

}

void AuthorizationWindow::on_btnSignUp_clicked()
{
    if(m_bIsLogin) // Встановлення інтерфейсу для створення акаунту
    {
        m_bIsLogin = false;
        ui->labLogin->setText("Create account");
        ui->btnSignIn->setText("Sign up");
        ui->labHaveAcc->setText("Allready have an account?");
        ui->btnSignUp->setText("Sign in");
    }
    else // Встановлення інтерфейсу для авторизації
    {
        m_bIsLogin = true;
        ui->labLogin->setText("Login");
        ui->btnSignIn->setText("Sign in");
        ui->labHaveAcc->setText("Don't have account?");
        ui->btnSignUp->setText("Sign up");
    }
}

