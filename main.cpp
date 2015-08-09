#include "gwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GWidget w;
    w.show();

    return a.exec();
}
