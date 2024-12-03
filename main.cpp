#include "mainwindow.h"
#include "authorizationwindow.h"
#include "selectionwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    SelectionWindow selectionWindow;
    selectionWindow.show();

    return app.exec();
}
