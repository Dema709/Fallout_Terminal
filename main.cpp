#include "Fallout_Terminal/Fallout_Terminal.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Fallout_Terminal w;
    w.show();
    return a.exec();
}
