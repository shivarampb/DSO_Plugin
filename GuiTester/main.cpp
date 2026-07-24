/*============================================================================
 *  GuiTester/main.cpp - dark-themed tabbed "Scope Plugin Test" application.
 *
 *  Usage:
 *      GuiTester [pluginDir]                 interactive
 *      GuiTester --smoke [pluginDir]         headless: load plugins, open a
 *                                            SimScope, fetch one waveform, exit
 *      GuiTester --screenshot <png> [dir]    headless: render + save a PNG
 *==========================================================================*/
#include <QApplication>
#include <QStringList>

#include "ScopeManager.h"
#include "ScopeTesterWindow.h"
#include "TesterCommon.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    applyDarkTheme(app);

    QStringList args = app.arguments();
    bool bSmoke = args.removeAll(QStringLiteral("--smoke")) > 0;
    QString strShot;
    const int iShot = args.indexOf(QStringLiteral("--screenshot"));
    if (iShot >= 0 && iShot + 1 < args.size()) {
        strShot = args.at(iShot + 1);
        args.removeAt(iShot + 1);
        args.removeAt(iShot);
    }

    // remaining positional arg (after argv[0]) is the plugin dir
    QString strPluginDir = QStringLiteral("plugins");
    for (int i = 1; i < args.size(); ++i) {
        if (!args.at(i).startsWith(QStringLiteral("--"))) { strPluginDir = args.at(i); break; }
    }

    ScopeTesterWindow win(strPluginDir);

    if (bSmoke) {
        return win.runSmokeTest();
    }
    if (!strShot.isEmpty()) {
        win.show();
        const bool ok = win.screenshotTo(strShot);
        return ok ? 0 : 1;
    }

    win.show();
    return app.exec();
}
