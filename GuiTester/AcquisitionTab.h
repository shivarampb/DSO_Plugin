/*============================================================================
 *  AcquisitionTab.h - acquisition mode / averages + Run / Stop / Single.
 *==========================================================================*/
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

    QComboBox*   m_pMode;
    QSpinBox*    m_pAvgCount;
    QPushButton* m_pRunBtn;
    QPushButton* m_pStopBtn;
    QPushButton* m_pSingleBtn;
    QPushButton* m_pApplyBtn;
    QLabel*      m_pReadout;
};

#endif // ACQUISITIONTAB_H
