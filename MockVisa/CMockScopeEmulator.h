/*=============================================================================
 *  CMockScopeEmulator.h - mini oscilloscope SCPI emulator inside MockVisa.
 *
 *  DEV-ONLY (never shipped). Emulates enough of a bench oscilloscope to
 *  exercise the REAL plugin code path (SCPI over VISA) without hardware:
 *    - IEEE-488.2 common commands (*IDN?, *RST, *CLS, *OPC?, *ESR?, *STB?, *TST?)
 *    - SYST:ERR? queue semantics
 *    - a generic "header value" store so any set/get round-trips
 *    - a valid binary waveform #-block for the DATA? queries, whose matching
 *      preamble round-trips to a known synthetic sine (3 cycles, 0.4 Vpk)
 *    - a small PNG screenshot #-block
 *
 *  It recognises both the Tektronix and the IVI/Keysight/R&S dialects for the
 *  waveform + screenshot queries, so different model plugins share one mock.
 *===========================================================================*/
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

    QString                        m_strModelName;
    QString                        m_strManufacturer;
    QString                        m_strIdnMatch;
    int                            m_iWfmPoints;
    QMap<QString, QByteArray>      m_mapValues;   /* header -> last value    */
    QList<QPair<int, QByteArray> > m_lstErrors;   /* SYST:ERR? queue         */
};

#endif /* CMOCKSCOPEEMULATOR_H */
