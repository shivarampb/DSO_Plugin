/**
 * @file    SaveTab.h
 * @brief   Save / Screenshot tab - setup save/recall and image capture.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#ifndef SAVETAB_H
#define SAVETAB_H

#include <QWidget>
#include "ScopeManager.h"

class QSpinBox;
class QPushButton;
class QLabel;

class SaveTab : public QWidget
{
    Q_OBJECT
  public:
    explicit SaveTab(QWidget* parent = nullptr);
    void setConnected(bool in_bConnected);

  signals:
    void log(const QString& in_strText);

  private slots:
    void onSave();
    void onRecall();
    void onScreenshot();

  private:
    QSpinBox* m_pSlot;
    QPushButton* m_pSaveBtn;
    QPushButton* m_pRecallBtn;
    QPushButton* m_pShotBtn;
    QLabel* m_pShotLabel;
};

#endif // SAVETAB_H
