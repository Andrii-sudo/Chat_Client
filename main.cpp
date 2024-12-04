#include "mainwindow.h"
#include "authorizationwindow.h"
#include "selectionwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    AuthorizationWindow authWindow(&mainWindow);

    SelectionWindow selectionWindow(&authWindow);
    selectionWindow.show();

    return app.exec();
}
