/*============================================================================
 *  main.cpp - "Scope Plugin Test" entry point (dark-themed Qt Widgets app).
 *
 *  Usage: GuiTester [--smoke] [--screenshot <file>] [pluginDir]
 *    --smoke      : offscreen self-test (connect MDO34 via MockVisa, fetch wfm).
 *    --screenshot : render a connected window to an image (offscreen).
 *  Default pluginDir = <appDir>/../plugins.
 *==========================================================================*/
#include <QApplication>
#include <QCoreApplication>
#include <QPalette>
#include <QStyleFactory>

#include "ScopeTesterWindow.h"

static void applyDarkTheme(QApplication& app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette p;
    const QColor base(0x1e, 0x1f, 0x22), panel(0x2b, 0x2d, 0x30), text(0xd0, 0xd3, 0xd7);
    p.setColor(QPalette::Window, base);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, panel);
    p.setColor(QPalette::AlternateBase, base);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, panel);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::Highlight, QColor(0x2d, 0x6c, 0xdf));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase, panel);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x6a, 0x6d, 0x70));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x6a, 0x6d, 0x70));
    app.setPalette(p);

    app.setStyleSheet(
        "QGroupBox { border: 1px solid #3a3d41; border-radius: 4px; margin-top: 12px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 3px;"
        "                   color: #4da6ff; font-weight: bold; }"
        "QTabBar::tab { background: #2b2d30; color: #c0c3c7; padding: 6px 14px; }"
        "QTabBar::tab:selected { color: #4da6ff; border-bottom: 2px solid #4da6ff; }"
        "QTabWidget::pane { border: 1px solid #3a3d41; }"
        "QLineEdit, QComboBox, QDoubleSpinBox, QSpinBox, QPlainTextEdit, QListWidget, QTableWidget"
        "  { background: #26282b; border: 1px solid #3a3d41; border-radius: 3px; padding: 3px; }"
        "QPushButton { background: #2b2d30; border: 1px solid #3a3d41; border-radius: 3px; padding: 6px; }"
        "QPushButton:hover { border-color: #4da6ff; }"
        "QPushButton:disabled { color: #6a6d70; }");
}

int main(int argc, char** argv)
{
    bool bSmoke = false;
    QString strPluginDir, strShot;
    for (int i = 1; i < argc; ++i)
    {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if (a == QStringLiteral("--smoke"))
        {
            bSmoke = true;
        }
        else if (a == QStringLiteral("--screenshot") && i + 1 < argc)
        {
            strShot = QString::fromLocal8Bit(argv[++i]);
        }
        else
        {
            strPluginDir = a;
        }
    }

    QApplication app(argc, argv);
    applyDarkTheme(app);
    if (strPluginDir.isEmpty())
    {
        strPluginDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../plugins");
    }

    ScopeTesterWindow w(strPluginDir);
    if (bSmoke)
    {
        return w.runSmokeTest();
    }
    if (!strShot.isEmpty())
    {
        return w.screenshotTo(strShot);
    }

    w.show();
    return app.exec();
}
