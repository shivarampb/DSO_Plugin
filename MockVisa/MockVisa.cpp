/**
 * @file    MockVisa.cpp
 * @brief   Hardware-free VISA (built as libvisa) implementing the vi* ABI over the emulator.
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023-aligned; Allman braces; see docs/Coding_Standard.md.
 */

#include <QByteArray>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QStringList>
#include <cstring>

#include "visa.h"
#include "CMockScopeEmulator.h"
#include "S_ScopeLimits.h"

#if defined(_WIN32)
#define MOCKVISA_EXPORT __declspec(dllexport)
#else
#define MOCKVISA_EXPORT __attribute__((visibility("default")))
#endif

namespace
{

const int READ_CAP_BYTES = 65536; // large enough for a waveform block in few reads

enum ESessionKind
{
    KIND_RM,
    KIND_INSTR,
    KIND_FIND
};

struct S_Session
{
    ESessionKind m_eKind;
    QString m_strResource;
    CMockScopeEmulator* m_pEmulator;
    QByteArray m_abyRx;
    bool m_bTermcharEn;
    bool m_bTimeout;
    QStringList m_lstFind;
    int m_iFindIdx;

    S_Session()
        : m_eKind(KIND_RM), m_pEmulator(nullptr), m_bTermcharEn(true), m_bTimeout(false), m_iFindIdx(0)
    {
    }
    ~S_Session()
    {
        delete m_pEmulator;
    }
};

QMutex g_mtx;
QMap<ViSession, S_Session*> g_sessions;
ViSession g_next = 1000;

/**
 * @brief  Pick the catalog model whose name appears in a resource string.
 * @param[in] in_strResource  VISA resource string to scan (case-insensitive).
 * @return The matched model name; "MDO34" when nothing in the catalog matches.
 * @pre    None.
 */
QString modelFromResource(const QString& in_strResource)
{
    int iCount = 0;
    const S_ScopeLimits* cat = ScopeLimitsCatalog(&iCount);
    for (int i = 0; i < iCount; ++i)
    {
        const QString name = QString::fromLatin1(cat[i].m_szModelName);
        if (in_strResource.contains(name, Qt::CaseInsensitive))
        {
            return name;
        }
    }
    return QStringLiteral("MDO34");
}

/**
 * @brief  Enumerate the mock instrument resources advertised by viFindRsrc.
 * @return Fixed list: MDO34, RTM3004, and a TIMEOUT resource that forces a
 *         VI_ERROR_TMO on read for error-path testing.
 * @pre    None.
 */
QStringList mockResourceList()
{
    return QStringList() << QStringLiteral("MOCK0::MDO34::INSTR") << QStringLiteral("MOCK0::RTM3004::INSTR")
                         << QStringLiteral("MOCK0::TIMEOUT::INSTR");
}

/**
 * @brief  Look up a live session record by its VISA handle.
 * @param[in] vi  Session/find-list handle to resolve.
 * @return The session pointer, or nullptr if the handle is unknown.
 * @pre    Caller holds g_mtx.
 */
S_Session* find(ViSession vi)
{
    return g_sessions.value(vi, nullptr);
}

} // namespace

extern "C"
{
    /**
     * @brief  Open the default VISA resource manager (mock).
     * @param[out] vi  Receives the new resource-manager session handle.
     * @return VI_SUCCESS; VI_ERROR_INV_OBJECT if @p vi is null.
     * @pre    None.
     */
    MOCKVISA_EXPORT ViStatus viOpenDefaultRM(ViPSession vi)
    {
        if (vi == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }
        QMutexLocker lock(&g_mtx);
        S_Session* s = new S_Session();
        s->m_eKind = KIND_RM;
        *vi = g_next++;
        g_sessions.insert(*vi, s);
        return VI_SUCCESS;
    }

    /**
     * @brief  Open a session to a mock instrument named by its resource string.
     * @param[in]  sesn  A resource-manager session from viOpenDefaultRM.
     * @param[in]  name  VISA resource string; a "TIMEOUT" resource opens a session
     *                   whose reads always time out, else an emulator is created.
     * @param[out] vi    Receives the new instrument session handle.
     * @return VI_SUCCESS; VI_ERROR_INV_OBJECT for a null argument or unknown @p sesn.
     * @pre    @p sesn must be a valid resource-manager session.
     */
    MOCKVISA_EXPORT ViStatus viOpen(ViSession sesn, const ViChar* name, ViAccessMode, ViUInt32, ViPSession vi)
    {
        if (vi == nullptr || name == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }
        QMutexLocker lock(&g_mtx);
        if (find(sesn) == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }

        const QString strResource = QString::fromLatin1(name);
        S_Session* s = new S_Session();
        s->m_eKind = KIND_INSTR;
        s->m_strResource = strResource;
        if (strResource.contains(QStringLiteral("TIMEOUT"), Qt::CaseInsensitive))
        {
            s->m_bTimeout = true;
        }
        else
        {
            s->m_pEmulator = new CMockScopeEmulator(modelFromResource(strResource));
        }
        *vi = g_next++;
        g_sessions.insert(*vi, s);
        return VI_SUCCESS;
    }

    /**
     * @brief  Close a session (RM, instrument, or find-list) and free its state.
     * @param[in] vi  Handle to close.
     * @return VI_SUCCESS; VI_ERROR_INV_OBJECT if the handle is unknown.
     * @pre    None.
     */
    MOCKVISA_EXPORT ViStatus viClose(ViObject vi)
    {
        QMutexLocker lock(&g_mtx);
        S_Session* s = g_sessions.take(static_cast<ViSession>(vi));
        if (s == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }
        delete s;
        return VI_SUCCESS;
    }

    /**
     * @brief  Write bytes to a mock instrument, dispatching each SCPI line to the
     *         emulator and queuing any responses for a later viRead.
     * @param[in]  vi      Instrument session handle.
     * @param[in]  buf     Bytes to write (may contain multiple newline-split lines).
     * @param[in]  cnt     Number of bytes in @p buf.
     * @param[out] retCnt  Receives the byte count reported as written (== cnt).
     * @return VI_SUCCESS; VI_ERROR_INV_OBJECT if not an instrument session. A
     *         timeout session accepts the write but produces no response.
     * @pre    None.
     */
    MOCKVISA_EXPORT ViStatus viWrite(ViSession vi, ViConstBuf buf, ViUInt32 cnt, ViPUInt32 retCnt)
    {
        QMutexLocker lock(&g_mtx);
        S_Session* s = find(vi);
        if (s == nullptr || s->m_eKind != KIND_INSTR)
        {
            return VI_ERROR_INV_OBJECT;
        }
        if (retCnt)
        {
            *retCnt = cnt;
        }
        if (s->m_bTimeout || s->m_pEmulator == nullptr)
        {
            return VI_SUCCESS;
        }

        const QByteArray aby(reinterpret_cast<const char*>(buf), static_cast<int>(cnt));
        const QList<QByteArray> lines = aby.split('\n');
        for (const QByteArray& line : lines)
        {
            if (!line.trimmed().isEmpty())
            {
                s->m_pEmulator->HandleLine(line, s->m_abyRx);
            }
        }
        return VI_SUCCESS;
    }

    /**
     * @brief  Read queued response bytes from a mock instrument, honoring the
     *         termchar-enable attribute for line- vs block-oriented transfers.
     * @param[in]  vi      Instrument session handle.
     * @param[out] buf     Receives up to @p cnt bytes.
     * @param[in]  cnt     Capacity of @p buf.
     * @param[out] retCnt  Receives the number of bytes copied.
     * @return VI_SUCCESS / VI_SUCCESS_TERM_CHAR / VI_SUCCESS_MAX_CNT per the read;
     *         VI_ERROR_TMO if the session is a timeout resource or has no data;
     *         VI_ERROR_INV_OBJECT if not an instrument session.
     * @pre    None. When termchar is enabled the read stops at the first newline.
     */
    MOCKVISA_EXPORT ViStatus viRead(ViSession vi, ViBuf buf, ViUInt32 cnt, ViPUInt32 retCnt)
    {
        QMutexLocker lock(&g_mtx);
        S_Session* s = find(vi);
        if (s == nullptr || s->m_eKind != KIND_INSTR)
        {
            return VI_ERROR_INV_OBJECT;
        }
        if (retCnt)
        {
            *retCnt = 0;
        }
        if (s->m_bTimeout || s->m_abyRx.isEmpty())
        {
            return VI_ERROR_TMO;
        }

        int iToCopy = qMin(static_cast<int>(cnt), qMin(READ_CAP_BYTES, s->m_abyRx.size()));
        ViStatus status;
        if (s->m_bTermcharEn)
        {
            const int iTerm = s->m_abyRx.indexOf('\n');
            if (iTerm >= 0 && iTerm < iToCopy)
            {
                iToCopy = iTerm + 1;
                status = VI_SUCCESS_TERM_CHAR;
            }
            else
            {
                status = (s->m_abyRx.size() > iToCopy) ? VI_SUCCESS_MAX_CNT : VI_SUCCESS;
            }
        }
        else
        {
            status = (s->m_abyRx.size() > iToCopy) ? VI_SUCCESS_MAX_CNT : VI_SUCCESS;
        }
        memcpy(buf, s->m_abyRx.constData(), static_cast<size_t>(iToCopy));
        s->m_abyRx.remove(0, iToCopy);
        if (retCnt)
        {
            *retCnt = static_cast<ViUInt32>(iToCopy);
        }
        return status;
    }

    /**
     * @brief  Set a session attribute; only VI_ATTR_TERMCHAR_EN is significant.
     * @param[in] vi         Session handle.
     * @param[in] attrName   Attribute id.
     * @param[in] attrValue  New value (0 disables termchar, non-zero enables).
     * @return VI_SUCCESS; VI_ERROR_INV_OBJECT if the handle is unknown. Unhandled
     *         attributes are accepted and ignored.
     * @pre    None.
     */
    MOCKVISA_EXPORT ViStatus viSetAttribute(ViObject vi, ViAttr attrName, ViAttrState attrValue)
    {
        QMutexLocker lock(&g_mtx);
        S_Session* s = find(static_cast<ViSession>(vi));
        if (s == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }
        if (attrName == VI_ATTR_TERMCHAR_EN)
        {
            s->m_bTermcharEn = (attrValue != 0);
        }
        return VI_SUCCESS;
    }

    /**
     * @brief  Get a session attribute; only VI_ATTR_TERMCHAR_EN is supported.
     * @param[in]  vi         Session handle.
     * @param[in]  attrName   Attribute id.
     * @param[out] attrValue  Receives the attribute value.
     * @return VI_SUCCESS; VI_ERROR_INV_OBJECT for a null buffer or unknown handle;
     *         VI_ERROR_NSUP_ATTR for an unsupported attribute.
     * @pre    None.
     */
    MOCKVISA_EXPORT ViStatus viGetAttribute(ViObject vi, ViAttr attrName, void* attrValue)
    {
        QMutexLocker lock(&g_mtx);
        S_Session* s = find(static_cast<ViSession>(vi));
        if (s == nullptr || attrValue == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }
        if (attrName == VI_ATTR_TERMCHAR_EN)
        {
            *static_cast<ViUInt32*>(attrValue) = s->m_bTermcharEn ? 1u : 0u;
            return VI_SUCCESS;
        }
        return VI_ERROR_NSUP_ATTR;
    }

    /**
     * @brief  Begin a resource search, returning the first match and a find list.
     * @param[in]  sesn      Resource-manager session.
     * @param[out] findList  Receives a find-list handle for viFindNext.
     * @param[out] retcnt    Receives the total number of matches.
     * @param[out] desc      Receives the first resource string.
     * @return VI_SUCCESS; VI_ERROR_INV_OBJECT for a null argument or bad @p sesn.
     * @pre    @p sesn must be a valid resource-manager session. The expression
     *         argument is ignored; the full mock resource list is always returned.
     */
    MOCKVISA_EXPORT ViStatus viFindRsrc(ViSession sesn, const ViChar*, ViFindList* findList, ViPUInt32 retcnt,
                                        ViChar desc[])
    {
        QMutexLocker lock(&g_mtx);
        if (find(sesn) == nullptr || findList == nullptr || retcnt == nullptr || desc == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }
        const QStringList lst = mockResourceList();
        S_Session* s = new S_Session();
        s->m_eKind = KIND_FIND;
        s->m_lstFind = lst;
        s->m_iFindIdx = 1;
        const ViSession fl = g_next++;
        g_sessions.insert(fl, s);
        *findList = fl;
        *retcnt = static_cast<ViUInt32>(lst.size());
        qstrncpy(desc, lst.first().toLatin1().constData(), 256);
        return VI_SUCCESS;
    }

    /**
     * @brief  Return the next resource from a find list created by viFindRsrc.
     * @param[in]  findList  Find-list handle.
     * @param[out] desc      Receives the next resource string.
     * @return VI_SUCCESS; VI_ERROR_RSRC_NFOUND once the list is exhausted;
     *         VI_ERROR_INV_OBJECT for a bad handle or null buffer.
     * @pre    @p findList must come from viFindRsrc.
     */
    MOCKVISA_EXPORT ViStatus viFindNext(ViFindList findList, ViChar desc[])
    {
        QMutexLocker lock(&g_mtx);
        S_Session* s = find(static_cast<ViSession>(findList));
        if (s == nullptr || s->m_eKind != KIND_FIND || desc == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }
        if (s->m_iFindIdx >= s->m_lstFind.size())
        {
            return VI_ERROR_RSRC_NFOUND;
        }
        qstrncpy(desc, s->m_lstFind.at(s->m_iFindIdx).toLatin1().constData(), 256);
        ++s->m_iFindIdx;
        return VI_SUCCESS;
    }

    /**
     * @brief  Render a human-readable description for a VISA status code.
     * @param[in]  status  Status code to describe.
     * @param[out] desc    Receives the description (max 256 bytes).
     * @return VI_SUCCESS; VI_ERROR_INV_OBJECT if @p desc is null.
     * @pre    None.
     */
    MOCKVISA_EXPORT ViStatus viStatusDesc(ViObject, ViStatus status, ViChar desc[])
    {
        if (desc == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }
        const char* szText = "(MockVisa status)";
        switch (status)
        {
        case VI_ERROR_TMO:
            szText = "Timeout expired (MockVisa)";
            break;
        case VI_ERROR_RSRC_NFOUND:
            szText = "Resource not found (MockVisa)";
            break;
        case VI_ERROR_INV_OBJECT:
            szText = "Invalid object (MockVisa)";
            break;
        case VI_ERROR_INV_RSRC_NAME:
            szText = "Invalid resource name (MockVisa)";
            break;
        default:
            break;
        }
        qstrncpy(desc, szText, 256);
        return VI_SUCCESS;
    }

    /**
     * @brief  Discard any buffered response bytes for a session.
     * @param[in] vi  Session handle.
     * @return VI_SUCCESS; VI_ERROR_INV_OBJECT if the handle is unknown.
     * @pre    None.
     */
    MOCKVISA_EXPORT ViStatus viClear(ViSession vi)
    {
        QMutexLocker lock(&g_mtx);
        S_Session* s = find(vi);
        if (s == nullptr)
        {
            return VI_ERROR_INV_OBJECT;
        }
        s->m_abyRx.clear();
        return VI_SUCCESS;
    }

} // extern "C"
