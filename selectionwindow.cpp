#include "selectionwindow.h"
#include "ui_selectionwindow.h"
#include <QPushButton>
#include <QVBoxLayout>

SelectionWindow::SelectionWindow(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::SelectionWindow), // Ініціалізація інтерфейсу
    authWindow(nullptr),
    commMethod("")
{
    ui->setupUi(this);

}

SelectionWindow::~SelectionWindow() {
    delete authWindow;  // Звільнення пам'яті
}

void SelectionWindow::onPushBtnSocketClicked() {
    //commMethod = "Socket";  // Встановлення методу зв'язку
    if (!authWindow) {
        authWindow = new AuthorizationWindow(this);  // Створення вікна, якщо його ще немає
    }
    //authWindow->setCommMethod(commMethod);  // Передача методу зв'язку
    authWindow->show();
    this->hide();  // Приховування поточного вікна
}

void SelectionWindow::onPushBtnPipeClicked() {
    //commMethod = "Pipe";  // Встановлення методу зв'язку
    if (!authWindow) {
        authWindow = new AuthorizationWindow(this);  // Створення вікна, якщо його ще немає
    }
    //authWindow->setCommMethod(commMethod);  // Передача методу зв'язку
    authWindow->show();
    this->hide();  // Приховування поточного вікна
}
