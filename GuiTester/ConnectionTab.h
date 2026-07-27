/*============================================================================
 *  ConnectionTab.h - model + interface + address, Connect/Disconnect, identity.
 *==========================================================================*/
#ifndef CONNECTIONTAB_H
#define CONNECTIONTAB_H

#include <QWidget>

#include "ScopeManager.h"

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;

class ConnectionTab : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectionTab(QWidget* parent = nullptr);

    QString selectedModel() const;
    bool    connectTo(const QString& in_strModel, const QString& in_strResource); // for smoke

signals:
    void connected(const QString& in_strModel);
    void disconnected();
    void log(const QString& in_strText);

private slots:
    void onConnect();
    void onDisconnect();
    void onInterfaceChanged();

private:
    S_Scope_ConnectionConfig buildConfig() const;

    QComboBox*   m_pModelCombo;
    QComboBox*   m_pInterfaceCombo;
    QLineEdit*   m_pAddressEdit;
    QPushButton* m_pConnectBtn;
    QPushButton* m_pDisconnectBtn;
    QLabel*      m_pIdentityLabel;
};

#endif // CONNECTIONTAB_H
