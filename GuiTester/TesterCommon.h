/**
 * @file    TesterCommon.h
 * @brief   Shared constants and helpers for the Scope Plugin Test app.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#ifndef TESTERCOMMON_H
#define TESTERCOMMON_H

#include <QPushButton>
#include <QString>

#include "ScopeTypes.h"

// The tester drives a single instrument bound to this scope number / channel.
static const U32BIT TESTER_SCOPE = 1;
static const U32BIT TESTER_CHANNEL = 1;

/**
 * @brief  Tint a push button with a background colour (Connect = green,
 *         Disconnect = red, Run = green, …) while keeping the dark disabled
 *         style.
 * @param[in,out] in_pButton  Button to restyle (must be non-null).
 * @param[in]     in_strColor CSS colour for the enabled background.
 * @pre    @p in_pButton is a valid button.
 */
inline void tintButton(QPushButton* in_pButton, const QString& in_strColor)
{
    in_pButton->setStyleSheet(QString("QPushButton { background-color: %1; color: white; font-weight: bold;"
                                      " border: 1px solid #3a3d41; border-radius: 3px; padding: 6px; }"
                                      "QPushButton:disabled { background-color: #2b2d30; color: #6a6d70; }")
                                  .arg(in_strColor));
}

#endif // TESTERCOMMON_H
