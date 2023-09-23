#include <QApplication>
#include <QPalette>

#include "MainWindow.h"

// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    MainWindow window;

    QPalette palette;
    palette.setColor(QPalette::Window, Qt::black);

    window.setAutoFillBackground(true);
    window.setPalette(palette);
    window.resize(MainWindow::DEFAULT_WIDTH,
                  MainWindow::DEFAULT_HEIGHT);
    window.setFixedSize(MainWindow::DEFAULT_WIDTH,
                        MainWindow::DEFAULT_HEIGHT);

    window.show();

    return application.exec();
}
