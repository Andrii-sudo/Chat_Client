#include "mainwindow.h"
#include "authorizationwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    AuthorizationWindow aw(&w);
    aw.show();

    return a.exec();
}
