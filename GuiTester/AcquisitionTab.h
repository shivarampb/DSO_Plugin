/**
 * @file    AcquisitionTab.h
 * @brief   Acquisition tab - mode/averages and Run/Stop/Single.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#ifndef ACQUISITIONTAB_H
#define ACQUISITIONTAB_H

#include <QWidget>
#include "ScopeManager.h"

class QComboBox;
class QSpinBox;
class QLabel;
class QPushButton;

class AcquisitionTab : public QWidget
{
    Q_OBJECT
  public:
    explicit AcquisitionTab(QWidget* parent = nullptr);
    void setConnected(bool in_bConnected);

  signals:
    void log(const QString& in_strText);

  private slots:
    void onApply();
    void onRun();
    void onStop();
    void onSingle();

  private:
    void refreshReadout();

    QComboBox* m_pMode;
    QSpinBox* m_pAvgCount;
    QPushButton* m_pRunBtn;
    QPushButton* m_pStopBtn;
    QPushButton* m_pSingleBtn;
    QPushButton* m_pApplyBtn;
    QLabel* m_pReadout;
};

#endif // ACQUISITIONTAB_H
