#include "widget.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // select fusion style if available
    // for consistent look among different platforms
    QStringList style_keys = QStyleFactory::keys();
    // QApplication::setStyle("Adwaita");
    // QApplication::setStyle("Adwaita-Dark");
    // QApplication::setStyle("HighContrast");
    // QApplication::setStyle("Windows");
    QApplication::setStyle("Fusion");
    // QApplication::setStyle("Breeze");

    Widget w;
    w.setLocale(QLocale::c());
    w.show();

    return app.exec();
}
