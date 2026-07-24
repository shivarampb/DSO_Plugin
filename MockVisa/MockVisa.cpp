/*============================================================================
 *  MockVisa.cpp - a fake VISA implementation (built as libvisa) for
 *  hardware-free development and CI.
 *
 *  Exports the standard viXxx symbols declared in <visa.h> so the model
 *  plugins, which link -lvisa exactly like against a real VISA, run with no
 *  instrument attached. Each opened session is backed by a mini oscilloscope
 *  SCPI emulator (CMockScopeEmulator). The model to emulate is taken from the
 *  resource string (the first catalog-model token); a resource containing
 *  "TIMEOUT" never answers a read.
 *==========================================================================*/
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
#  define MOCKVISA_EXPORT __declspec(dllexport)
#else
#  define MOCKVISA_EXPORT __attribute__((visibility("default")))
#endif

namespace {

const int READ_CAP_BYTES = 65536;   // large enough for a waveform block in few reads

enum ESessionKind { KIND_RM, KIND_INSTR, KIND_FIND };

struct S_Session {
    ESessionKind        m_eKind;
    QString             m_strResource;
    CMockScopeEmulator* m_pEmulator;
    QByteArray          m_abyRx;
    bool                m_bTermcharEn;
    bool                m_bTimeout;
    QStringList         m_lstFind;
    int                 m_iFindIdx;

    S_Session()
        : m_eKind(KIND_RM), m_pEmulator(nullptr)
        , m_bTermcharEn(true), m_bTimeout(false), m_iFindIdx(0) {}
    ~S_Session() { delete m_pEmulator; }
};

QMutex g_mtx;
QMap<ViSession, S_Session*> g_sessions;
ViSession g_next = 1000;

QString modelFromResource(const QString& in_strResource)
{
    int iCount = 0;
    const S_ScopeLimits* cat = ScopeLimitsCatalog(&iCount);
    for (int i = 0; i < iCount; ++i) {
        const QString name = QString::fromLatin1(cat[i].m_szModelName);
        if (in_strResource.contains(name, Qt::CaseInsensitive)) {
            return name;
        }
    }
    return QStringLiteral("MDO34");
}

QStringList mockResourceList()
{
    return QStringList()
            << QStringLiteral("MOCK0::MDO34::INSTR")
            << QStringLiteral("MOCK0::RTM3004::INSTR")
            << QStringLiteral("MOCK0::TIMEOUT::INSTR");
}

S_Session* find(ViSession vi) { return g_sessions.value(vi, nullptr); }

} // namespace

extern "C" {

MOCKVISA_EXPORT ViStatus viOpenDefaultRM(ViPSession vi)
{
    if (vi == nullptr) return VI_ERROR_INV_OBJECT;
    QMutexLocker lock(&g_mtx);
    S_Session* s = new S_Session();
    s->m_eKind = KIND_RM;
    *vi = g_next++;
    g_sessions.insert(*vi, s);
    return VI_SUCCESS;
}

MOCKVISA_EXPORT ViStatus viOpen(ViSession sesn, const ViChar* name, ViAccessMode,
                                ViUInt32, ViPSession vi)
{
    if (vi == nullptr || name == nullptr) return VI_ERROR_INV_OBJECT;
    QMutexLocker lock(&g_mtx);
    if (find(sesn) == nullptr) return VI_ERROR_INV_OBJECT;

    const QString strResource = QString::fromLatin1(name);
    S_Session* s = new S_Session();
    s->m_eKind = KIND_INSTR;
    s->m_strResource = strResource;
    if (strResource.contains(QStringLiteral("TIMEOUT"), Qt::CaseInsensitive)) {
        s->m_bTimeout = true;
    } else {
        s->m_pEmulator = new CMockScopeEmulator(modelFromResource(strResource));
    }
    *vi = g_next++;
    g_sessions.insert(*vi, s);
    return VI_SUCCESS;
}

MOCKVISA_EXPORT ViStatus viClose(ViObject vi)
{
    QMutexLocker lock(&g_mtx);
    S_Session* s = g_sessions.take(static_cast<ViSession>(vi));
    if (s == nullptr) return VI_ERROR_INV_OBJECT;
    delete s;
    return VI_SUCCESS;
}

MOCKVISA_EXPORT ViStatus viWrite(ViSession vi, ViConstBuf buf, ViUInt32 cnt, ViPUInt32 retCnt)
{
    QMutexLocker lock(&g_mtx);
    S_Session* s = find(vi);
    if (s == nullptr || s->m_eKind != KIND_INSTR) return VI_ERROR_INV_OBJECT;
    if (retCnt) *retCnt = cnt;
    if (s->m_bTimeout || s->m_pEmulator == nullptr) return VI_SUCCESS;

    const QByteArray aby(reinterpret_cast<const char*>(buf), static_cast<int>(cnt));
    const QList<QByteArray> lines = aby.split('\n');
    for (const QByteArray& line : lines) {
        if (!line.trimmed().isEmpty()) {
            s->m_pEmulator->HandleLine(line, s->m_abyRx);
        }
    }
    return VI_SUCCESS;
}

MOCKVISA_EXPORT ViStatus viRead(ViSession vi, ViBuf buf, ViUInt32 cnt, ViPUInt32 retCnt)
{
    QMutexLocker lock(&g_mtx);
    S_Session* s = find(vi);
    if (s == nullptr || s->m_eKind != KIND_INSTR) return VI_ERROR_INV_OBJECT;
    if (retCnt) *retCnt = 0;
    if (s->m_bTimeout || s->m_abyRx.isEmpty()) return VI_ERROR_TMO;

    int iToCopy = qMin(static_cast<int>(cnt), qMin(READ_CAP_BYTES, s->m_abyRx.size()));
    ViStatus status;
    if (s->m_bTermcharEn) {
        const int iTerm = s->m_abyRx.indexOf('\n');
        if (iTerm >= 0 && iTerm < iToCopy) {
            iToCopy = iTerm + 1;
            status = VI_SUCCESS_TERM_CHAR;
        } else {
            status = (s->m_abyRx.size() > iToCopy) ? VI_SUCCESS_MAX_CNT : VI_SUCCESS;
        }
    } else {
        status = (s->m_abyRx.size() > iToCopy) ? VI_SUCCESS_MAX_CNT : VI_SUCCESS;
    }
    memcpy(buf, s->m_abyRx.constData(), static_cast<size_t>(iToCopy));
    s->m_abyRx.remove(0, iToCopy);
    if (retCnt) *retCnt = static_cast<ViUInt32>(iToCopy);
    return status;
}

MOCKVISA_EXPORT ViStatus viSetAttribute(ViObject vi, ViAttr attrName, ViAttrState attrValue)
{
    QMutexLocker lock(&g_mtx);
    S_Session* s = find(static_cast<ViSession>(vi));
    if (s == nullptr) return VI_ERROR_INV_OBJECT;
    if (attrName == VI_ATTR_TERMCHAR_EN) s->m_bTermcharEn = (attrValue != 0);
    return VI_SUCCESS;
}

MOCKVISA_EXPORT ViStatus viGetAttribute(ViObject vi, ViAttr attrName, void* attrValue)
{
    QMutexLocker lock(&g_mtx);
    S_Session* s = find(static_cast<ViSession>(vi));
    if (s == nullptr || attrValue == nullptr) return VI_ERROR_INV_OBJECT;
    if (attrName == VI_ATTR_TERMCHAR_EN) {
        *static_cast<ViUInt32*>(attrValue) = s->m_bTermcharEn ? 1u : 0u;
        return VI_SUCCESS;
    }
    return VI_ERROR_NSUP_ATTR;
}

MOCKVISA_EXPORT ViStatus viFindRsrc(ViSession sesn, const ViChar*, ViFindList* findList,
                                    ViPUInt32 retcnt, ViChar desc[])
{
    QMutexLocker lock(&g_mtx);
    if (find(sesn) == nullptr || findList == nullptr || retcnt == nullptr || desc == nullptr)
        return VI_ERROR_INV_OBJECT;
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

MOCKVISA_EXPORT ViStatus viFindNext(ViFindList findList, ViChar desc[])
{
    QMutexLocker lock(&g_mtx);
    S_Session* s = find(static_cast<ViSession>(findList));
    if (s == nullptr || s->m_eKind != KIND_FIND || desc == nullptr) return VI_ERROR_INV_OBJECT;
    if (s->m_iFindIdx >= s->m_lstFind.size()) return VI_ERROR_RSRC_NFOUND;
    qstrncpy(desc, s->m_lstFind.at(s->m_iFindIdx).toLatin1().constData(), 256);
    ++s->m_iFindIdx;
    return VI_SUCCESS;
}

MOCKVISA_EXPORT ViStatus viStatusDesc(ViObject, ViStatus status, ViChar desc[])
{
    if (desc == nullptr) return VI_ERROR_INV_OBJECT;
    const char* szText = "(MockVisa status)";
    switch (status) {
    case VI_ERROR_TMO:           szText = "Timeout expired (MockVisa)";       break;
    case VI_ERROR_RSRC_NFOUND:   szText = "Resource not found (MockVisa)";    break;
    case VI_ERROR_INV_OBJECT:    szText = "Invalid object (MockVisa)";        break;
    case VI_ERROR_INV_RSRC_NAME: szText = "Invalid resource name (MockVisa)"; break;
    default: break;
    }
    qstrncpy(desc, szText, 256);
    return VI_SUCCESS;
}

MOCKVISA_EXPORT ViStatus viClear(ViSession vi)
{
    QMutexLocker lock(&g_mtx);
    S_Session* s = find(vi);
    if (s == nullptr) return VI_ERROR_INV_OBJECT;
    s->m_abyRx.clear();
    return VI_SUCCESS;
}

} // extern "C"
