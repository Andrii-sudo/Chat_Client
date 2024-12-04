#include "selectionwindow.h"
#include "ui_selectionwindow.h"
#include <QPushButton>
#include <QVBoxLayout>

SelectionWindow::SelectionWindow(AuthorizationWindow* pAuthWin, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::SelectionWindow)
{
    ui->setupUi(this);

    m_pAuthWin = pAuthWin; // Зв'язок з вікном авторизації
}

SelectionWindow::~SelectionWindow()
{
    delete ui;
}

void SelectionWindow::on_pushBtnSocket_clicked()
{
    m_pAuthWin->setSyncMethod("Socket");

    m_pAuthWin->show();
    this->hide();
}

void SelectionWindow::on_pushBtnPipe_clicked()
{
    m_pAuthWin->setSyncMethod("Pipe");

    m_pAuthWin->show();
    this->hide();
}

