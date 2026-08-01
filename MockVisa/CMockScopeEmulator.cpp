/**
 * @file    CMockScopeEmulator.cpp
 * @brief   Oscilloscope SCPI emulator implementation for MockVisa.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include "CMockScopeEmulator.h"

#include "S_ScopeLimits.h"

#include <QStringList>
#include <cmath>

namespace
{
const double PI = 3.14159265358979323846;

/* synthetic waveform constants (shared by the preamble and the data block) */
const double WFM_XINC = 1.0e-6; /* s per point                          */
const double WFM_YINC = 1.0e-3; /* V per code                           */
const double WFM_AMPL_V = 0.4;  /* 0-peak amplitude in volts            */
const double WFM_CYCLES = 3.0;  /* cycles across the record             */
} // namespace

/**
 * @brief  Construct an emulator that impersonates a specific scope model.
 * @param[in] in_strModelName  Catalog model name (e.g. "MDO34"); selects the
 *                             manufacturer and the *IDN? token to answer with.
 * @pre    None. An unknown model name is tolerated: the manufacturer defaults
 *         to "Mock" and the IDN match token to the model name itself.
 */
CMockScopeEmulator::CMockScopeEmulator(const QString& in_strModelName)
    : m_strModelName(in_strModelName), m_strManufacturer(QStringLiteral("Mock")),
      m_strIdnMatch(in_strModelName), m_iWfmPoints(1000), m_strMeasType(QStringLiteral("FREQ"))
{
    // Overlay the catalog manufacturer/IDN token when the model is known.
    const S_ScopeLimits* p = ScopeFindLimits(in_strModelName.toLatin1().constData());
    if (p != nullptr)
    {
        m_strManufacturer = QString::fromLatin1(p->m_szManufacturer);
        m_strIdnMatch = QString::fromLatin1(p->m_szIdnMatch);
    }
}

/**
 * @brief  Build the *IDN? response for this model.
 * @return "<manufacturer>,<model>,MOCK000001,1.0.0" — the model field is the
 *         exact token a plugin verifies against so identity checks pass.
 * @pre    None.
 */
QByteArray CMockScopeEmulator::idnString() const
{
    // Use the exact IDN token the plugin verifies against as the model field.
    return QStringLiteral("%1,%2,MOCK000001,1.0.0").arg(m_strManufacturer, m_strIdnMatch).toLatin1();
}

/**
 * @brief  Queue an error so a subsequent SYST:ERR? pops it (FIFO), matching
 *         IEEE-488.2 error-queue semantics.
 * @param[in] in_iCode     SCPI error number to report.
 * @param[in] in_szMessage NUL-terminated human-readable error text.
 * @pre    None.
 */
void CMockScopeEmulator::pushError(int in_iCode, const char* in_szMessage)
{
    m_lstErrors.append(qMakePair(in_iCode, QByteArray(in_szMessage)));
}

/**
 * @brief  Synthesize a plausible decoded-frame readout for the currently
 *         selected serial-bus type, so the readBusDecode() path returns
 *         realistic content with no hardware attached.
 * @return A one-line decode string for I2C/SPI/UART/CAN/LIN; "BUS: (no frames)"
 *         when the bus type is unset or unrecognized.
 * @pre    None. The bus type is whatever a prior BUS:TYPE/MODE set stored
 *         (see HandleLine); an empty type yields the no-frames default.
 */
QByteArray CMockScopeEmulator::busDecodeString() const
{
    const QString t = m_strBusType.toUpper();
    if (t.contains(QStringLiteral("I2C")) || t.contains(QStringLiteral("IIC")))
    {
        return "I2C: START ADDR 0x50 W ACK DATA 0x12 ACK 0x34 ACK STOP";
    }
    if (t.contains(QStringLiteral("SPI")))
    {
        return "SPI: FRAME MOSI 0xA5,0x5A MISO 0x00,0xFF";
    }
    if (t.contains(QStringLiteral("UART")) || t.contains(QStringLiteral("RS232")) ||
        t.contains(QStringLiteral("SERIAL")))
    {
        return "UART: 0x48 0x49 0x0D (\"HI\\r\") PARITY OK";
    }
    if (t.contains(QStringLiteral("CAN")))
    {
        return "CAN: ID 0x123 DLC 2 DATA 0xDE 0xAD CRC OK ACK";
    }
    if (t.contains(QStringLiteral("LIN")))
    {
        return "LIN: ID 0x21 DATA 0xFF 0x01 CHECKSUM OK";
    }
    return "BUS: (no frames)";
}

/**
 * @brief  Compute an automatic-measurement value for the given type, derived
 *         from the same 3-cycle, 0.4 Vpk sine the waveform block encodes, so
 *         measured values stay self-consistent with the returned trace.
 * @param[in] in_strType  Measurement mnemonic (case-insensitive), e.g. "VPP",
 *                        "FREQ", "RMS"; SCPI dialect aliases are accepted.
 * @return The measured value in the natural unit (volts, seconds, hertz, or
 *         percent); frequency is returned for an unrecognized type.
 * @pre    None.
 * @note   Match order is significant: max/min variants (including R&S UPEak /
 *         LPEak) are tested before the generic PEAK->Vpp mapping.
 */
double CMockScopeEmulator::measurementValue(const QString& in_strType) const
{
    const int n = (m_iWfmPoints > 0) ? m_iWfmPoints : 1000;
    const double totalTime = n * WFM_XINC;
    const double freq = (totalTime > 0.0) ? (WFM_CYCLES / totalTime) : 0.0;
    const double period = (freq > 0.0) ? 1.0 / freq : 0.0;
    const QString t = in_strType.toUpper();
    auto has = [&](const char* k) { return t.contains(QLatin1String(k)); };
    // order matters: max/min (incl. R&S UPEak/LPEak) before the generic PEAK->Vpp
    if (has("VMAX") || has("MAX") || has("UPE") || has("HIGH") || has("TOP"))
    {
        return WFM_AMPL_V;
    }
    if (has("VMIN") || has("MIN") || has("LPE") || has("LOW") || has("BASE"))
    {
        return -WFM_AMPL_V;
    }
    if (has("RMS"))
    {
        return WFM_AMPL_V / 1.4142135623730951;
    }
    if (has("VPP") || has("PK2") || has("PEAK") || has("AMPL"))
    {
        return 2.0 * WFM_AMPL_V; // 0.8
    }
    if (has("MEAN") || has("VAV") || has("AVG"))
    {
        return 0.0;
    }
    if (has("FREQ"))
    {
        return freq;
    }
    if (has("PER"))
    {
        return period;
    }
    if (has("RIS") || has("RTIM") || has("FALL") || has("FTIM"))
    {
        return period * 0.1;
    }
    if (has("WID") || has("PPW") || has("NPW"))
    {
        return period * 0.5;
    }
    if (has("DUT") || has("DCYC"))
    {
        return 50.0;
    }
    if (has("PHAS") || has("DEL"))
    {
        return 0.0;
    }
    return freq; // default
}

/**
 * @brief  Build a binary waveform #-block (WORD, signed 16-bit, big-endian) of
 *         a 3-cycle sine together with the matching IVI-style preamble CSV.
 * @param[in]  in_iPoints          Requested record length; values <= 0 fall
 *                                 back to 1000 points.
 * @param[out] out_abyPreambleCsv  Receives the preamble CSV
 *                                 (format,type,points,count,xInc,xOrig,xRef,
 *                                 yInc,yOrig,yRef) describing the same block.
 * @return An IEEE-488.2 definite-length "#<w><len><payload>\n" waveform block.
 * @pre    None.
 * @note   The preamble and payload are generated from one set of synthetic
 *         constants, so parsing the preamble reconstructs the encoded signal.
 */
QByteArray CMockScopeEmulator::buildWaveformBlock(int in_iPoints, QByteArray& out_abyPreambleCsv)
{
    const int n = (in_iPoints > 0) ? in_iPoints : 1000;
    const double xOrigin = -(n / 2.0) * WFM_XINC;
    const double codeAmpl = WFM_AMPL_V / WFM_YINC; // 0.4 / 0.001 = 400 codes

    // preamble CSV: format,type,points,count,xInc,xOrig,xRef,yInc,yOrig,yRef
    out_abyPreambleCsv = QStringLiteral("1,0,%1,1,%2,%3,0,%4,0,0")
                             .arg(n)
                             .arg(WFM_XINC, 0, 'E', 6)
                             .arg(xOrigin, 0, 'E', 6)
                             .arg(WFM_YINC, 0, 'E', 6)
                             .toLatin1();

    QByteArray payload;
    payload.reserve(n * 2);
    for (int i = 0; i < n; ++i)
    {
        const double phase = WFM_CYCLES * static_cast<double>(i) / static_cast<double>(n);
        const int code = static_cast<int>(std::lround(codeAmpl * std::sin(2.0 * PI * phase)));
        const qint16 c16 = static_cast<qint16>(code);
        payload.append(static_cast<char>((c16 >> 8) & 0xFF)); // MSB first
        payload.append(static_cast<char>(c16 & 0xFF));
    }

    const QByteArray lenStr = QByteArray::number(payload.size());
    QByteArray block;
    block.append('#');
    block.append(QByteArray::number(lenStr.size()));
    block.append(lenStr);
    block.append(payload);
    block.append('\n');
    return block;
}

/**
 * @brief  Build a screenshot response as an IEEE-488.2 definite-length block
 *         wrapping a minimal valid 1x1 PNG.
 * @return "#<w><len><png>\n" so the plugin's binary-block reader extracts a
 *         decodable PNG image.
 * @pre    None.
 */
QByteArray CMockScopeEmulator::buildScreenshotPng()
{
    // minimal valid 1x1 PNG
    static const char* const kPngB64 = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+M8"
                                       "AAAMBAQAY3Y2wAAAAAElFTkSuQmCC";
    const QByteArray png = QByteArray::fromBase64(QByteArray(kPngB64));
    const QByteArray lenStr = QByteArray::number(png.size());
    QByteArray block;
    block.append('#');
    block.append(QByteArray::number(lenStr.size()));
    block.append(lenStr);
    block.append(png);
    block.append('\n');
    return block;
}

/**
 * @brief  Handle one SCPI line: dispatch common commands, waveform/screenshot
 *         blocks, measurements and serial-bus decode, else store or echo values.
 * @param[in]  in_abyLine     One raw SCPI line (leading/trailing space and the
 *                            terminator are trimmed internally).
 * @param[out] out_abyResponse For a query, receives the reply (terminated);
 *                            left untouched for a set command with no response.
 * @pre    None. An empty line is ignored. Setter/getter pairs are matched by
 *         header, so a query returns the value a prior matching set stored.
 * @note   Dispatch order is deliberate: more specific matches (preamble before
 *         data block, screenshot before generic DATA?) are tested first.
 */
void CMockScopeEmulator::HandleLine(const QByteArray& in_abyLine, QByteArray& out_abyResponse)
{
    const QByteArray line = in_abyLine.trimmed();
    if (line.isEmpty())
    {
        return;
    }

    // split header / args on the first whitespace
    int iSp = line.indexOf(' ');
    const QByteArray headerRaw = (iSp < 0) ? line : line.left(iSp);
    const QByteArray args = (iSp < 0) ? QByteArray() : line.mid(iSp + 1).trimmed();
    const QString header = QString::fromLatin1(headerRaw).toUpper();

    auto isQuery = [&]() { return header.endsWith(QLatin1Char('?')); };

    /*---- IEEE-488.2 common commands ------------------------------------*/
    if (header == QLatin1String("*IDN?"))
    {
        out_abyResponse.append(idnString()).append('\n');
        return;
    }
    if (header == QLatin1String("*RST"))
    {
        m_mapValues.clear();
        m_lstErrors.clear();
        m_iWfmPoints = 1000;
        return;
    }
    if (header == QLatin1String("*CLS"))
    {
        m_lstErrors.clear();
        return;
    }
    if (header == QLatin1String("*OPC?"))
    {
        out_abyResponse.append("1\n");
        return;
    }
    if (header == QLatin1String("*OPC"))
    {
        return;
    }
    if (header == QLatin1String("*ESR?"))
    {
        out_abyResponse.append("0\n");
        return;
    }
    if (header == QLatin1String("*STB?"))
    {
        out_abyResponse.append("0\n");
        return;
    }
    if (header == QLatin1String("*TST?"))
    {
        out_abyResponse.append("0\n");
        return;
    }
    if (header == QLatin1String("*OPT?"))
    {
        out_abyResponse.append("0\n");
        return;
    }

    /*---- SYST:ERR? queue -----------------------------------------------*/
    if (header == QLatin1String("SYST:ERR?") || header == QLatin1String("SYSTEM:ERROR?"))
    {
        if (m_lstErrors.isEmpty())
        {
            out_abyResponse.append("0,\"No error\"\n");
        }
        else
        {
            const QPair<int, QByteArray> e = m_lstErrors.takeFirst();
            out_abyResponse.append(QByteArray::number(e.first)).append(",\"").append(e.second).append("\"\n");
        }
        return;
    }

    /*---- waveform point count (affects the generated block) -------------
     *  IVI/R&S :WAV:POIN / ACQ:POIN, and Tektronix DATa:STOP.              */
    if (!isQuery() && (header.contains(QStringLiteral("POIN")) ||
                       (header.contains(QStringLiteral("DATA")) && header.contains(QStringLiteral("STOP")))))
    {
        bool ok = false;
        const int n = args.toInt(&ok);
        if (ok && n > 0)
        {
            m_iWfmPoints = n;
        }
        m_mapValues.insert(header, args);
        return;
    }

    /*---- screenshot block (check before generic DATA? queries) ---------*/
    if (isQuery() && (header.contains(QStringLiteral("HCOP")) || header.contains(QStringLiteral("IMAG")) ||
                      (header.contains(QStringLiteral("DISP")) && header.contains(QStringLiteral("DATA")))))
    {
        out_abyResponse.append(buildScreenshotPng());
        return;
    }
    if ((header.contains(QStringLiteral("HCOP")) || header.contains(QStringLiteral("HARDC"))) && !isQuery())
    {
        // Tektronix HARDCopy STARt style -> emit the image block
        out_abyResponse.append(buildScreenshotPng());
        return;
    }

    /*---- waveform preamble (Tek WFMOutpre? / IVI :WAV:PRE? / R&S :DATA:HEAD?)
     *     checked before the data block: an R&S header query contains DATA
     *     too, but is distinguished by the PRE/HEAD token.                  */
    if (isQuery() && (header.contains(QStringLiteral("PRE")) || header.contains(QStringLiteral("HEAD"))) &&
        (header.contains(QStringLiteral("WAV")) || header.contains(QStringLiteral("WFM")) ||
         header.contains(QStringLiteral("CHAN")) || header.contains(QStringLiteral("CURV")) ||
         header.contains(QStringLiteral("DATA"))))
    {
        QByteArray csv;
        buildWaveformBlock(m_iWfmPoints, csv);
        out_abyResponse.append(csv).append('\n');
        return;
    }

    /*---- waveform data block (Tek CURVe? / IVI :WAV:DATA? / R&S CHAN:DATA?) */
    if (isQuery() && (header == QLatin1String("CURVE?") || header == QLatin1String("CURV?") ||
                      (header.contains(QStringLiteral("DATA")) &&
                       (header.contains(QStringLiteral("WAV")) || header.contains(QStringLiteral("CHAN"))))))
    {
        QByteArray csv;
        out_abyResponse.append(buildWaveformBlock(m_iWfmPoints, csv));
        return;
    }

    /*---- automatic measurements ----------------------------------------
     *  A set command carrying the measurement type (Tek MEASUrement:IMMed:TYPe,
     *  R&S MEASurement:MAIN) stores it; a MEAS query returns a computed value,
     *  using the type in the header if present else the stored type.        */
    if (header.contains(QStringLiteral("MEAS")) && !isQuery() && !args.isEmpty() &&
        (header.contains(QStringLiteral("TYPE")) || header.contains(QStringLiteral("MAIN"))))
    {
        m_strMeasType = QString::fromLatin1(args).toUpper();
        m_mapValues.insert(header, args);
        return;
    }
    if (header.contains(QStringLiteral("MEAS")) && isQuery())
    {
        // does the header itself name a measurement type?
        static const char* kTypes[] = {"VPP",  "VMAX", "VMIN", "VRMS", "MEAN", "VAV",  "FREQ", "PER", "RIS",
                                       "FALL", "WID",  "DUT",  "PHAS", "DEL",  "AMPL", "TOP",  "BASE"};
        QString type = m_strMeasType;
        for (const char* k : kTypes)
        {
            if (header.contains(QLatin1String(k)))
            {
                type = QLatin1String(k);
                break;
            }
        }
        out_abyResponse.append(QByteArray::number(measurementValue(type), 'E', 6)).append('\n');
        return;
    }

    /*---- serial bus: capture type, synthesize a decoded-frame readout ---*/
    if (header.contains(QStringLiteral("BUS")) && !isQuery() && !args.isEmpty() &&
        (header.contains(QStringLiteral("TYPE")) || header.contains(QStringLiteral("MODE"))))
    {
        m_strBusType = QString::fromLatin1(args).toUpper();
        m_mapValues.insert(header, args);
        return;
    }
    if (header.contains(QStringLiteral("BUS")) && isQuery() && header.contains(QStringLiteral("DATA")))
    {
        out_abyResponse.append(busDecodeString()).append('\n');
        return;
    }

    /*---- generic query / set store -------------------------------------*/
    if (isQuery())
    {
        // strip trailing '?' to find the value stored by the matching setter
        QString setHeader = header;
        setHeader.chop(1);
        if (m_mapValues.contains(setHeader))
        {
            const QByteArray val = m_mapValues.value(setHeader);
            const QString up = QString::fromLatin1(val).trimmed().toUpper();
            // normalise boolean keywords so numeric getters (e.g. isKeyLocked) parse
            if (up == QLatin1String("ON") || up == QLatin1String("1"))
            {
                out_abyResponse.append("1\n");
            }
            else if (up == QLatin1String("OFF") || up == QLatin1String("0"))
            {
                out_abyResponse.append("0\n");
            }
            else
            {
                out_abyResponse.append(val).append('\n');
            }
        }
        else
        {
            out_abyResponse.append("0\n"); // benign default
        }
        return;
    }

    // plain set command: remember the value for a later query
    m_mapValues.insert(header, args);
}
