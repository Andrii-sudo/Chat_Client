#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setName(std::string strName);

private slots:
    void on_btnSearch_clicked();
    void on_btnSend_clicked();

    void on_lsChats_clicked(const QModelIndex &index);
    void on_txtSearch_textChanged(const QString &arg1);

    void updateChats(); // Викликається раз в 1.5 секунди

private:
    Ui::MainWindow* ui;

    QString m_strUserName;
    QString m_strSelectedName;

    std::vector<QString> m_vecChats;
    std::unordered_map<QString, QString> m_mapChats;

    SOCKET connectToServer(const std::string& strIp, const std::string& strPort);
};
#endif // MAINWINDOW_H
