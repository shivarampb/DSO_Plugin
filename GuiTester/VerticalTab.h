/*============================================================================
 *  VerticalTab.h - per-channel scale / offset / coupling / probe + Apply.
 *==========================================================================*/
#ifndef VERTICALTAB_H
#define VERTICALTAB_H

#include <QWidget>
#include "ScopeManager.h"

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;

class VerticalTab : public QWidget
{
    Q_OBJECT
public:
    explicit VerticalTab(QWidget* parent = nullptr);
    void setConnected(bool in_bConnected);

signals:
    void log(const QString& in_strText);

private slots:
    void onChannelChanged(int idx);
    void onApply();

private:
    void loadFromInstrument();

    QSpinBox*       m_pChannel;
    QDoubleSpinBox* m_pScale;
    QDoubleSpinBox* m_pOffset;
    QComboBox*      m_pCoupling;
    QComboBox*      m_pProbe;
    QPushButton*    m_pApplyBtn;
};

#endif // VERTICALTAB_H
