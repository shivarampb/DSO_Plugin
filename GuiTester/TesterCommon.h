/*============================================================================
 *  TesterCommon.h - shared helpers for the GuiTester ("Scope Plugin Test").
 *  Mirrors the ELoad GuiTester's TesterCommon.h.
 *==========================================================================*/
#ifndef TESTERCOMMON_H
#define TESTERCOMMON_H

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QPushButton>
#include <QStyleFactory>

/* The logical scope number and default channel the tester drives. */
static const U32BIT TESTER_SCOPE   = 1;
static const U32BIT TESTER_CHANNEL = 1;

/* Tint a push button (green = go/connect, red = stop/disconnect). */
inline void tintButton(QPushButton* in_pButton, const QColor& in_color)
{
    if (in_pButton == nullptr) return;
    in_pButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: white; font-weight: bold;"
        " border: none; padding: 6px 14px; border-radius: 3px; }"
        "QPushButton:disabled { background-color: #555; color: #999; }")
        .arg(in_color.name()));
}

/* Apply a dark Fusion theme to the whole application. */
inline void applyDarkTheme(QApplication& app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QPalette p;
    p.setColor(QPalette::Window,          QColor(37, 37, 38));
    p.setColor(QPalette::WindowText,      QColor(220, 220, 220));
    p.setColor(QPalette::Base,            QColor(30, 30, 30));
    p.setColor(QPalette::AlternateBase,   QColor(45, 45, 48));
    p.setColor(QPalette::ToolTipBase,     QColor(220, 220, 220));
    p.setColor(QPalette::ToolTipText,     QColor(30, 30, 30));
    p.setColor(QPalette::Text,            QColor(220, 220, 220));
    p.setColor(QPalette::Button,          QColor(53, 53, 53));
    p.setColor(QPalette::ButtonText,      QColor(220, 220, 220));
    p.setColor(QPalette::Highlight,       QColor(0, 120, 215));
    p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(120, 120, 120));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));
    app.setPalette(p);
}

#endif // TESTERCOMMON_H
