/*============================================================================
 *  HorizontalTriggerTab.h - timebase + edge-trigger source/slope/level/mode.
 *==========================================================================*/
#ifndef HORIZONTALTRIGGERTAB_H
#define HORIZONTALTRIGGERTAB_H

#include <QWidget>
#include "ScopeManager.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

class HorizontalTriggerTab : public QWidget
{
    Q_OBJECT
public:
    explicit HorizontalTriggerTab(QWidget* parent = nullptr);
    void setConnected(bool in_bConnected);

signals:
    void log(const QString& in_strText);

private slots:
    void onApply();
    void onForce();

private:
    QDoubleSpinBox* m_pTimebase;
    QComboBox*      m_pTrigSource;
    QComboBox*      m_pTrigSlope;
    QComboBox*      m_pTrigMode;
    QDoubleSpinBox* m_pTrigLevel;
    QPushButton*    m_pApplyBtn;
    QPushButton*    m_pForceBtn;
    QLabel*         m_pStateLabel;
};

#endif // HORIZONTALTRIGGERTAB_H
