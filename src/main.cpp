#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("aiselectstock");
    app.setOrganizationName("aiselectstock");

    MainWindow w;
    w.show();

    return app.exec();
}