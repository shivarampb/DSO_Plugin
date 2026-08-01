/**
 * @file    CMockScopeEmulator.h
 * @brief   Mini oscilloscope SCPI emulator for MockVisa (dev-only; never shipped).
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#ifndef CMOCKSCOPEEMULATOR_H
#define CMOCKSCOPEEMULATOR_H

#include <QByteArray>
#include <QMap>
#include <QPair>
#include <QString>
#include <QList>

class CMockScopeEmulator
{
  public:
    explicit CMockScopeEmulator(const QString& in_strModelName);

    /* Feeds one complete SCPI line; the response (if any) is appended to
     * out_abyResponse (may be a binary block, not just a text line).        */
    void HandleLine(const QByteArray& in_abyLine, QByteArray& out_abyResponse);

    static QByteArray buildWaveformBlock(int in_iPoints, QByteArray& out_abyPreambleCsv);
    static QByteArray buildScreenshotPng();

  private:
    void pushError(int in_iCode, const char* in_szMessage);
    QByteArray idnString() const;
    /* value of an automatic measurement computed from the synthetic sine */
    double measurementValue(const QString& in_strType) const;
    /* a synthetic decoded-frame string for the current serial-bus type */
    QByteArray busDecodeString() const;

    QString m_strModelName;
    QString m_strManufacturer;
    QString m_strIdnMatch;
    int m_iWfmPoints;
    QString m_strMeasType;                     /* Tek IMMed:TYPe state    */
    QString m_strBusType;                      /* current serial-bus type */
    QMap<QString, QByteArray> m_mapValues;     /* header -> last value    */
    QList<QPair<int, QByteArray>> m_lstErrors; /* SYST:ERR? queue         */
};

#endif /* CMOCKSCOPEEMULATOR_H */
