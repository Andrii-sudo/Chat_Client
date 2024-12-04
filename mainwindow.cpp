#include "mainwindow.h"
#include "./ui_mainwindow.h"


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
#include <QTimer>
#include <QStringListModel>

#define DEFAULT_PORT "12345"
#define PIPE_NAME L"\\\\.\\pipe\\ServerPipe"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Заборона введення пробілів у поле пошуку
    QRegularExpression regexDisableSpaces("[^\\s]*");
    QValidator* validDisableSpaces = new QRegularExpressionValidator(regexDisableSpaces, this);

    ui->txtSearch->setValidator(validDisableSpaces);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setSyncMethod(QString strSynchMethod)
{
    m_strSynchMethod = strSynchMethod;
}

void MainWindow::setName(std::string strName)
{
    m_strUserName = QString::fromStdString(strName);
    ui->labName->setText(m_strUserName);

    // Надсилання запиту оновлення інформації раз у 1.5 секунди
    QTimer *ptUpdateChatsTimer = new QTimer(this);
    connect(ptUpdateChatsTimer, &QTimer::timeout, this, &MainWindow::updateChats);
    ptUpdateChatsTimer->start(1500);
}

SOCKET MainWindow::connectToServer(const std::string& strIp, const std::string& strPort)
{
    struct addrinfo *result = NULL,
        *ptr = NULL,
        hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;     // Використовувати IPv4
    hints.ai_socktype = SOCK_STREAM; // Тип сокета: потік (TCP)
    hints.ai_protocol = IPPROTO_TCP; // Протокол: TCP

    // Визначення локальної адреси і порту для з'єднанням з сервером
    int	iResult = getaddrinfo(strIp.c_str(), strPort.c_str(), &hints, &result);
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

// Підключення до іменованого пайпа
HANDLE MainWindow::connectToPipe()
{
    HANDLE hPipe = CreateFile(
        PIPE_NAME,            // Ім'я пайпа
        GENERIC_READ | GENERIC_WRITE, // Режим доступу
        0,                    // Немає спільного доступу
        NULL,                 // Атрибути безпеки
        OPEN_EXISTING,        // Відкрити існуючий пайп
        0,                    // Атрибути файлу
        NULL                  // Шаблон файлу
        );

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        QMessageBox::critical(nullptr, "Error", "Failed to connect to pipe: " + QString::number(GetLastError()));
    }

    return hPipe;
}

void MainWindow::on_btnSearch_clicked()
{
    if (ui->txtSearch->text().isEmpty())
    {
        return;
    }

    std::string strSearch = std::string("ser ") + ui->txtSearch->text().toStdString();

    if (m_strSynchMethod == "Socket")
    {
        // Створення сокету та підключення його до серверу
        SOCKET socket = connectToServer("127.0.0.1", DEFAULT_PORT);
        if (socket == INVALID_SOCKET)
        {
            return;
        }
        int iResult;

        // Надсилання запиту до сервера
        iResult = send(socket, strSearch.c_str(), strSearch.length(), 0);
        if (iResult == SOCKET_ERROR)
        {
            QMessageBox::critical(this, "Error", "send failed: " + QString::number(WSAGetLastError()));
            closesocket(socket);
            return;
        }

        const int RECVBUF_SIZE = 2048;
        std::vector<char> vecRecvBuf(RECVBUF_SIZE);

        // Приймання відповіді від серверу (послідовність імен користувачів записані через ' ')
        iResult = recv(socket, vecRecvBuf.data(), vecRecvBuf.size(), 0);
        if (iResult > 0)
        {
            QString strReceivedData = QString::fromStdString(std::string(vecRecvBuf.data(), iResult));
            QStringList strLsFoundUsers = strReceivedData.split(' ', Qt::SkipEmptyParts);

            // Видалення зі списку імені користувача, який ініціював пошук
            strLsFoundUsers.removeAll(m_strUserName);

            if(!strLsFoundUsers.empty())
            {
                // Створюємо модель на основі списку знайдених користувачів
                QStringListModel* model = new QStringListModel(strLsFoundUsers, this);

                // Прив'язуємо модель
                ui->lsChats->setModel(model);
            }
            else
            {
                QMessageBox::information(this, "Search", "No users found :(");
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
        HANDLE hPipe = connectToPipe();
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        DWORD bytesWritten;
        BOOL bResult = WriteFile(
            hPipe,                        // Дескриптор каналу
            strSearch.c_str(),            // Дані для запису
            strSearch.length(),           // Кількість байтів для запису
            &bytesWritten,                // Кількість фактично записаних байтів
            NULL);                        // Немає асинхронного IO

        if (!bResult)
        {
            QMessageBox::critical(this, "Error", "Write to pipe failed: " + QString::number(GetLastError()));
            CloseHandle(hPipe);
            return;
        }

        const int RECVBUF_SIZE = 2048;
        std::vector<char> vecRecvBuf(RECVBUF_SIZE);
        DWORD bytesRead;

        // Читання відповіді з каналу
        bResult = ReadFile(
            hPipe,                        // Дескриптор каналу
            vecRecvBuf.data(),            // Буфер для отриманих даних
            RECVBUF_SIZE,                 // Максимальний розмір буфера
            &bytesRead,                   // Кількість фактично прочитаних байтів
            NULL);                        // Немає асинхронного IO

        if (!bResult || bytesRead == 0)
        {
            QMessageBox::critical(this, "Error", "Read from pipe failed: " + QString::number(GetLastError()));
            CloseHandle(hPipe);
            return;
        }

        QString strReceivedData = QString::fromStdString(std::string(vecRecvBuf.data(), bytesRead));
        QStringList strLsFoundUsers = strReceivedData.split(' ', Qt::SkipEmptyParts);

        // Видалення зі списку імені користувача, який ініціював пошук
        strLsFoundUsers.removeAll(m_strUserName);

        if (!strLsFoundUsers.empty())
        {
            // Створюємо модель на основі списку знайдених користувачів
            QStringListModel* model = new QStringListModel(strLsFoundUsers, this);

            // Прив'язуємо модель
            ui->lsChats->setModel(model);
        }
        else
        {
            QMessageBox::information(this, "Search", "No users found :(");
        }

        CloseHandle(hPipe);
    }
}

void MainWindow::on_btnSend_clicked()
{
    if(ui->txtMessage->text().isEmpty() || m_strSelectedName.isEmpty())
    {
        return;
    }

    std::string strMessage = std::string("mes ") + m_strUserName.toStdString() + " " +  m_strSelectedName.toStdString()
                             + " " + ui->txtMessage->text().toStdString();

    if (m_strSynchMethod == "Socket")
    {
        // Створення сокету та підключення його до серверу
        SOCKET socket = connectToServer("127.0.0.1", DEFAULT_PORT);
        if (socket == INVALID_SOCKET)
        {
            return;
        }

        ui->txtMessage->clear();

        int iResult;

        // Надсилання запиту до сервера
        iResult = send(socket, strMessage.c_str(), strMessage.length(), 0);
        if (iResult == SOCKET_ERROR)
        {
            QMessageBox::critical(this, "Error", "send failed: " + QString::number(WSAGetLastError()));
            closesocket(socket);
            return;
        }

        const int RECVBUF_SIZE = 1;
        std::vector<char> vecRecvBuf(RECVBUF_SIZE);

        // Приймання відповіді від серверу (Завжди "Y")
        iResult = recv(socket, vecRecvBuf.data(), vecRecvBuf.size(), 0);
        if (iResult > 0)
        {
            // Завжди приймаємо Y
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
        HANDLE hPipe = connectToPipe();
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        ui->txtMessage->clear();

        DWORD bytesWritten;
        BOOL bResult = WriteFile(
            hPipe,                        // Дескриптор каналу
            strMessage.c_str(),           // Дані для запису
            strMessage.length(),          // Кількість байтів для запису
            &bytesWritten,                // Кількість фактично записаних байтів
            NULL);                        // Немає асинхронного IO

        if (!bResult)
        {
            QMessageBox::critical(this, "Error", "Write to pipe failed: " + QString::number(GetLastError()));
            CloseHandle(hPipe);
            return;
        }

        const int RECVBUF_SIZE = 1; // Очікуємо одну літеру ("Y")
        std::vector<char> vecRecvBuf(RECVBUF_SIZE);
        DWORD bytesRead;

        // Читання відповіді з каналу
        bResult = ReadFile(
            hPipe,                        // Дескриптор каналу
            vecRecvBuf.data(),            // Буфер для отриманих даних
            RECVBUF_SIZE,                 // Максимальний розмір буфера
            &bytesRead,                   // Кількість фактично прочитаних байтів
            NULL);                        // Немає асинхронного IO

        if (!bResult || bytesRead == 0)
        {
            QMessageBox::critical(this, "Error", "Read from pipe failed: " + QString::number(GetLastError()));
            CloseHandle(hPipe);
            return;
        }

        // Обробка відповіді (очікуємо "Y")
        if (vecRecvBuf[0] == 'Y')
        {
            // Успішно відправлено
        }
        else
        {
            QMessageBox::information(this, "Error", "Unexpected response from server");
        }

        CloseHandle(hPipe);
    }
}

void MainWindow::on_lsChats_clicked(const QModelIndex &index)
{
    m_strSelectedName = index.data(Qt::DisplayRole).toString();
    ui->txtChat->setPlainText(m_mapChats[m_strSelectedName]);
}

void MainWindow::updateChats()
{
    std::string strMessage = "upd " + m_strUserName.toStdString();

    if (m_strSynchMethod == "Socket")
    {
        // Створення сокету та підключення його до серверу
        SOCKET socket = connectToServer("127.0.0.1", DEFAULT_PORT);
        if (socket == INVALID_SOCKET)
        {
            return;
        }

        int iResult;

        // Надсилання запиту до сервера
        iResult = send(socket, strMessage.c_str(), strMessage.length(), 0);
        if (iResult == SOCKET_ERROR)
        {
            QMessageBox::critical(this, "Error", "send failed: " + QString::number(WSAGetLastError()));
            closesocket(socket);
            return;
        }

        const int RECVBUF_SIZE = 20480;
        std::vector<char> vecRecvBuf(RECVBUF_SIZE);

        /*
        * --------------------------------------
        * Формат даних, які приймаємо
        * <Ім'я співбесідника 1>
        * <Повідомлення 1>
        * ...
        * <Повідомлення n>
        *
        * <Ім'я співбесідника 2>
        * ...
        * <В кінці символ ' '>
        * --------------------------------------
        */

        iResult = recv(socket, vecRecvBuf.data(), vecRecvBuf.size(), 0);
        if (iResult > 0)
        {
            if (vecRecvBuf[0] != ' ') // Якщо є що обновляти
            {
                // Очищення попередніх даних
                m_vecChats.clear();
                m_mapChats.clear();

                int i = 0;
                while (vecRecvBuf[i] != ' ')
                {
                    QString strUserName;

                    // Запис чату в список
                    for(; vecRecvBuf[i] != '\n'; i++)
                    {
                        strUserName.push_back(vecRecvBuf[i]);
                    }
                    m_vecChats.push_back(strUserName);

                    i++; // Пропускаємо '\n'

                    // Запис вмісту чату в map
                    for(; vecRecvBuf[i] != '\n' || vecRecvBuf[i + 1] != '\n'; i++)
                    {
                        m_mapChats[strUserName].push_back(vecRecvBuf[i]);
                    }
                    i += 2; // пропускаємо "\n\n"
                }

                if (ui->txtSearch->text().isEmpty()) // Якщо користувач нічого не шукав
                {
                    QStringList chatList;

                    // Додаємо всі чати до списку для відображення
                    for (int i = 0; i < m_vecChats.size(); i++)
                    {
                        chatList.append(m_vecChats[i]);
                    }

                    // Отримання поточної моделі зі списку чатів
                    QStringListModel* currentModel = qobject_cast<QStringListModel*>(ui->lsChats->model());

                    // Оновлення моделі тільки якщо дані змінилися
                    if (!currentModel || currentModel->stringList() != chatList)
                    {
                        QStringListModel* model = new QStringListModel(chatList, this);

                        ui->lsChats->setModel(model);

                        if (currentModel)
                        {
                            currentModel->deleteLater(); // Видаляємо стару модель
                        }
                    }
                }

                // Відображення вибраного чату
                on_lsChats_clicked(ui->lsChats->currentIndex());
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
        HANDLE hPipe = connectToPipe();
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        DWORD bytesWritten;
        BOOL writeResult = WriteFile(
            hPipe,                        // Дескриптор каналу
            strMessage.c_str(),           // Дані для запису
            strMessage.length(),          // Кількість байтів для запису
            &bytesWritten,                // Кількість фактично записаних байтів
            NULL);                        // Немає асинхронного IO

        if (!writeResult)
        {
            QMessageBox::critical(this, "Error", "Write to pipe failed: " + QString::number(GetLastError()));
            CloseHandle(hPipe);
            return;
        }

        const int RECVBUF_SIZE = 20480;
        std::vector<char> vecRecvBuf(RECVBUF_SIZE);
        DWORD bytesRead;

        // Читання відповіді з каналу
        BOOL bResult = ReadFile(
            hPipe,                        // Дескриптор каналу
            vecRecvBuf.data(),            // Буфер для отриманих даних
            RECVBUF_SIZE,                 // Максимальний розмір буфера
            &bytesRead,                   // Кількість фактично прочитаних байтів
            NULL);                        // Немає асинхронного IO

        if (!bResult || bytesRead == 0)
        {
            QMessageBox::critical(this, "Error", "Read from pipe failed: " + QString::number(GetLastError()));
            CloseHandle(hPipe);
            return;
        }

        if (vecRecvBuf[0] != ' ') // Якщо є що обновляти
        {
            // Очищення попередніх даних
            m_vecChats.clear();
            m_mapChats.clear();

            int i = 0;
            while (vecRecvBuf[i] != ' ')
            {
                QString strUserName;

                // Запис чату в список
                for (; vecRecvBuf[i] != '\n'; i++)
                {
                    strUserName.push_back(vecRecvBuf[i]);
                }
                m_vecChats.push_back(strUserName);

                i++; // Пропускаємо '\n'

                // Запис вмісту чату в map
                for (; vecRecvBuf[i] != '\n' || vecRecvBuf[i + 1] != '\n'; i++)
                {
                    m_mapChats[strUserName].push_back(vecRecvBuf[i]);
                }
                i += 2; // пропускаємо "\n\n"
            }

            if (ui->txtSearch->text().isEmpty()) // Якщо користувач нічого не шукав
            {
                QStringList chatList;

                // Додаємо всі чати до списку для відображення
                for (int i = 0; i < m_vecChats.size(); i++)
                {
                    chatList.append(m_vecChats[i]);
                }

                // Отримання поточної моделі зі списку чатів
                QStringListModel* currentModel = qobject_cast<QStringListModel*>(ui->lsChats->model());

                // Оновлення моделі тільки якщо дані змінилися
                if (!currentModel || currentModel->stringList() != chatList)
                {
                    QStringListModel* model = new QStringListModel(chatList, this);

                    ui->lsChats->setModel(model);

                    if (currentModel)
                    {
                        currentModel->deleteLater(); // Видаляємо стару модель
                    }
                }
            }

            // Відображення вибраного чату
            on_lsChats_clicked(ui->lsChats->currentIndex());
        }

        CloseHandle(hPipe);
    }
}

void MainWindow::on_txtSearch_textChanged(const QString &arg1)
{
    if(arg1.isEmpty()) // Якщо поле пошуку пусте, відображаємо чати користувача
    {
        QStringList chatList;
        for (int i = 0; i < m_vecChats.size(); i++)
        {
            chatList.append(m_vecChats[i]);
        }

        QStringListModel* model = new QStringListModel(chatList, this);

        ui->lsChats->setModel(model);
    }
}

