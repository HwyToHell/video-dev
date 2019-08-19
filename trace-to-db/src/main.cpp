#include "../inc/tracetodb.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TraceToDb w;
    w.show();

    return a.exec();
}
