/*============================================================================
 *  MeasurementCursorTab.h - automatic measurements table + cursor controls.
 *==========================================================================*/
#ifndef MEASUREMENTCURSORTAB_H
#define MEASUREMENTCURSORTAB_H

#include <QWidget>
#include "ScopeManager.h"

class QComboBox;
class QSpinBox;
class QTableWidget;
class QPushButton;

class MeasurementCursorTab : public QWidget
{
    Q_OBJECT
public:
    explicit MeasurementCursorTab(QWidget* parent = nullptr);
    void setConnected(bool in_bConnected);

signals:
    void log(const QString& in_strText);

private slots:
    void onAddMeasurement();
    void onApplyCursor();

private:
    QSpinBox*     m_pChannel;
    QComboBox*    m_pMeasType;
    QTableWidget* m_pTable;
    QComboBox*    m_pCursorType;
    QPushButton*  m_pAddBtn;
    QPushButton*  m_pCursorBtn;
};

#endif // MEASUREMENTCURSORTAB_H
