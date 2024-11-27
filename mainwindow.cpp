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
#include <QStringListModel>
#include <QTimer>

#define DEFAULT_PORT "12345"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QRegularExpression regex("[^\\s]*");  // Будь-які символи, окрім пробілів
    QValidator* validator = new QRegularExpressionValidator(regex, this);

    // Призначаємо валідатор для QLineEdit
    ui->txtSearch->setValidator(validator);

    m_strUserName = "";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setName(std::string strName)
{
    m_strUserName = QString::fromStdString(strName);
    ui->labName->setText(m_strUserName);

    QTimer *ptUpdateChatsTimer = new QTimer(this);
    connect(ptUpdateChatsTimer, &QTimer::timeout, this, &MainWindow::updateChats);
    ptUpdateChatsTimer->start(2000);
}

SOCKET MainWindow::connectToServer(const std::string& strIp, const std::string& strPort)
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

void MainWindow::on_btnSearch_clicked()
{
    if (ui->txtSearch->text().isEmpty())
    {
        return;
    }

    SOCKET socket = connectToServer("127.0.0.1", DEFAULT_PORT);
    if (socket == INVALID_SOCKET)
    {
        return;
    }

    std::string strSearch = std::string("ser ") + ui->txtSearch->text().toStdString();

    int iResult;

    iResult = send(socket, strSearch.c_str(), strSearch.length(), 0);
    if (iResult == SOCKET_ERROR)
    {
        QMessageBox::critical(this, "Error", "send failed: " + QString::number(WSAGetLastError()));
        closesocket(socket);
        return;
    }

    const int RECVBUF_SIZE = 2048;
    std::vector<char> vecRecvBuf(RECVBUF_SIZE);

    iResult = recv(socket, vecRecvBuf.data(), vecRecvBuf.size(), 0);
    if (iResult > 0)
    {
        if (vecRecvBuf[0] != ' ') // Якщо щось знайдено
        {
            QString strReceivedData = QString::fromStdString(std::string(vecRecvBuf.data(), iResult));
            QStringList foundUsers = strReceivedData.split(' ', Qt::SkipEmptyParts);

            // Видалення користувача зі списку
            foundUsers.removeAll(m_strUserName);

            // Створюємо модель на основі списку знайдених користувачів
            QStringListModel* model = new QStringListModel(foundUsers, this);

            // Прив'язуємо модель до QListView
            ui->lsChats->setModel(model);
        }
        else
        {
            QMessageBox::critical(this, "Search", "No users found");
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

void MainWindow::on_btnSend_clicked()
{
    if(ui->txtMessage->text().isEmpty() || m_strSelectedName.isEmpty())
    {
        return;
    }

    SOCKET socket = connectToServer("127.0.0.1", DEFAULT_PORT);
    if (socket == INVALID_SOCKET)
    {
        return;
    }

    std::string strMessage = std::string("mes ") + m_strUserName.toStdString() + " " +  m_strSelectedName.toStdString()
                                                 + " " + ui->txtMessage->text().toStdString();

    ui->txtMessage->clear();

    int iResult;

    iResult = send(socket, strMessage.c_str(), strMessage.length(), 0);
    if (iResult == SOCKET_ERROR)
    {
        QMessageBox::critical(this, "Error", "send failed: " + QString::number(WSAGetLastError()));
        closesocket(socket);
        return;
    }

    const int RECVBUF_SIZE = 1;
    std::vector<char> vecRecvBuf(RECVBUF_SIZE);

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

void MainWindow::on_lsChats_clicked(const QModelIndex &index)
{
    m_strSelectedName = index.data(Qt::DisplayRole).toString();
    ui->txtChat->setPlainText(m_mapChats[m_strSelectedName]);
}

void MainWindow::updateChats()
{
    SOCKET socket = connectToServer("127.0.0.1", DEFAULT_PORT);
    if (socket == INVALID_SOCKET)
    {
        return;
    }

    std::string strMessage = "upd " + m_strUserName.toStdString();

    int iResult;

    iResult = send(socket, strMessage.c_str(), strMessage.length(), 0);
    if (iResult == SOCKET_ERROR)
    {
        QMessageBox::critical(this, "Error", "send failed: " + QString::number(WSAGetLastError()));
        closesocket(socket);
        return;
    }

    const int RECVBUF_SIZE = 20480;
    std::vector<char> vecRecvBuf(RECVBUF_SIZE);

    iResult = recv(socket, vecRecvBuf.data(), vecRecvBuf.size(), 0);
    if (iResult > 0)
    {
        if(vecRecvBuf[0] != ' ') // Якщо є що обновляти
        {
            m_vecChats.clear();
            m_mapChats.clear();

            int i = 0;
            while (vecRecvBuf[i] != ' ')
            {
                QString strUserName;
                for(; vecRecvBuf[i] != '\n'; i++)
                {
                    strUserName.push_back(vecRecvBuf[i]);
                }
                m_vecChats.push_back(strUserName);
                i++;

                for(; vecRecvBuf[i] != '\n' || vecRecvBuf[i + 1] != '\n'; i++)
                {
                    m_mapChats[strUserName].push_back(vecRecvBuf[i]);
                }
                i += 2;
            }

            if (ui->txtSearch->text().isEmpty())
            {
                QStringList chatList;
                for (int i = 0; i < m_vecChats.size(); i++)
                {
                    chatList.append(m_vecChats[i]);
                }

                QStringListModel* currentModel = qobject_cast<QStringListModel*>(ui->lsChats->model());
                if (!currentModel || currentModel->stringList() != chatList)
                {
                    QStringListModel* model = new QStringListModel(chatList, this);

                    ui->lsChats->setModel(model);

                    if (currentModel)
                    {
                        currentModel->deleteLater();
                    }
                }
            }

            on_lsChats_clicked(ui->lsChats->currentIndex());
        }
    }

    closesocket(socket);
}

void MainWindow::on_txtSearch_textChanged(const QString &arg1)
{
    if(arg1.isEmpty())
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

