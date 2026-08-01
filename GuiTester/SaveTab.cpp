/**
 * @file    SaveTab.cpp
 * @brief   Save / Screenshot tab implementation.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "SaveTab.h"
#include "TesterCommon.h"

#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

/**
 * @brief  Build the save/screenshot tab: setup-slot selector with save/recall
 *         buttons and a screenshot capture button with a preview area.
 * @param[in] parent  Parent widget, or nullptr for a top-level widget.
 * @pre    None.
 */
SaveTab::SaveTab(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout* root = new QVBoxLayout(this);

    QHBoxLayout* ctl = new QHBoxLayout;
    m_pSlot = new QSpinBox(this);
    m_pSlot->setRange(0, 9);
    m_pSaveBtn = new QPushButton(tr("Save setup"), this);
    m_pRecallBtn = new QPushButton(tr("Recall setup"), this);
    m_pShotBtn = new QPushButton(tr("Capture Screenshot"), this);
    ctl->addWidget(new QLabel(tr("Slot:"), this));
    ctl->addWidget(m_pSlot);
    ctl->addWidget(m_pSaveBtn);
    ctl->addWidget(m_pRecallBtn);
    ctl->addWidget(m_pShotBtn);
    ctl->addStretch(1);
    root->addLayout(ctl);

    m_pShotLabel = new QLabel(tr("no screenshot"), this);
    m_pShotLabel->setAlignment(Qt::AlignCenter);
    m_pShotLabel->setMinimumHeight(220);
    root->addWidget(m_pShotLabel, 1);

    connect(m_pSaveBtn, SIGNAL(clicked()), this, SLOT(onSave()));
    connect(m_pRecallBtn, SIGNAL(clicked()), this, SLOT(onRecall()));
    connect(m_pShotBtn, SIGNAL(clicked()), this, SLOT(onScreenshot()));
}

/**
 * @brief  Slot: save the instrument setup to the selected slot.
 * @pre    A scope is connected on TESTER_SCOPE.
 */
void SaveTab::onSave()
{
    ScopeError e = CScopeManager::instance().saveSetup(TESTER_SCOPE, static_cast<U32BIT>(m_pSlot->value()));
    emit log(e.isSuccess() ? tr("Setup saved to slot %1").arg(m_pSlot->value()) : e.toString());
}

/**
 * @brief  Slot: recall the instrument setup from the selected slot.
 * @pre    A scope is connected on TESTER_SCOPE.
 */
void SaveTab::onRecall()
{
    ScopeError e = CScopeManager::instance().recallSetup(TESTER_SCOPE, static_cast<U32BIT>(m_pSlot->value()));
    emit log(e.isSuccess() ? tr("Setup recalled from slot %1").arg(m_pSlot->value()) : e.toString());
}

/**
 * @brief  Slot: capture a PNG screenshot and show it scaled in the preview.
 * @pre    A scope is connected on TESTER_SCOPE.
 */
void SaveTab::onScreenshot()
{
    QByteArray png;
    ScopeError e =
        CScopeManager::instance().captureScreenshot(TESTER_SCOPE, Enum_Scope_ImageFormat::m_enumPng, png);
    QImage img;
    if (e.isSuccess() && img.loadFromData(png))
    {
        m_pShotLabel->setPixmap(QPixmap::fromImage(img).scaled(m_pShotLabel->size(), Qt::KeepAspectRatio,
                                                               Qt::SmoothTransformation));
        emit log(tr("Screenshot captured (%1 bytes)").arg(png.size()));
    }
    else
    {
        emit log(tr("Screenshot failed: %1").arg(e.toString()));
    }
}

/**
 * @brief  Enable/disable the tab; on disconnect clear the screenshot preview.
 * @param[in] in_bConnected  true when a scope is connected.
 * @pre    None.
 */
void SaveTab::setConnected(bool in_bConnected)
{
    setEnabled(in_bConnected);
    if (!in_bConnected)
    {
        m_pShotLabel->setPixmap(QPixmap());
        m_pShotLabel->setText(tr("no screenshot"));
    }
}
