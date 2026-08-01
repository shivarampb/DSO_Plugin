/**
 * @file    visa.h
 * @brief   Minimal, ABI-correct subset of the VPP-4.3.2 VISA C API.
 * @details Enough for the Scope model plugins to talk SCPI to instruments
 *          (including counted/binary block reads for waveform & screenshot
 *          payloads). Provided so the plugins can @code #include <visa.h> @endcode
 *          and link @c -lvisa on any platform:
 *          - On Windows with NI-VISA / Keysight IO Libraries, point the plugin's
 *            INCLUDEPATH at the vendor include dir instead of this file and link
 *            @c -lvisa64; this header is ABI-compatible with the real one.
 *          - For hardware-free development/CI, link the bundled MockVisa (built
 *            as @c libvisa) which implements exactly these symbols.
 *
 * @note    ABI — ViStatus is signed 32-bit; ViSession/ViObject are unsigned
 *          32-bit even on 64-bit platforms; ViAttrState is register-wide (64-bit
 *          on LP64/LLP64). These match the official visatype.h.
 * @note    MISRA C++:2023 — this is a C ABI header (extern "C"); it declares
 *          only types and function prototypes.
 */
#ifndef __VISA_H__
#define __VISA_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t   ViStatus;
typedef uint32_t  ViSession;
typedef uint32_t  ViObject;
typedef uint32_t  ViFindList;
typedef uint32_t  ViUInt32;
typedef int32_t   ViInt32;
typedef uint16_t  ViUInt16;
typedef uint32_t  ViAttr;
typedef uint64_t  ViAttrState;
typedef char      ViChar;
typedef unsigned char ViByte;
typedef ViChar*   ViPChar;
typedef ViByte*   ViBuf;
typedef const ViByte* ViConstBuf;
typedef ViUInt32  ViAccessMode;
typedef ViChar    ViRsrc[256];
typedef ViUInt32* ViPUInt32;
typedef ViStatus* ViPStatus;
typedef ViSession* ViPSession;

#define VI_NULL   ((ViObject)0)
#define VI_TRUE   ((ViUInt32)1)
#define VI_FALSE  ((ViUInt32)0)

#define VI_SUCCESS            ((ViStatus)0x00000000L)
#define VI_SUCCESS_TERM_CHAR  ((ViStatus)0x3FFF0005L)
#define VI_SUCCESS_MAX_CNT    ((ViStatus)0x3FFF0006L)

#define VI_ERROR_INV_OBJECT   ((ViStatus)0xBFFF000EL)
#define VI_ERROR_RSRC_NFOUND  ((ViStatus)0xBFFF0011L)
#define VI_ERROR_INV_RSRC_NAME ((ViStatus)0xBFFF0012L)
#define VI_ERROR_TMO          ((ViStatus)0xBFFF0015L)
#define VI_ERROR_CONN_LOST    ((ViStatus)0xBFFF00A6L)
#define VI_ERROR_NSUP_ATTR    ((ViStatus)0xBFFF001DL)

#define VI_ATTR_TMO_VALUE       ((ViAttr)0x3FFF001AUL)
#define VI_ATTR_TERMCHAR        ((ViAttr)0x3FFF0018UL)
#define VI_ATTR_TERMCHAR_EN     ((ViAttr)0x3FFF0038UL)
#define VI_ATTR_SEND_END_EN     ((ViAttr)0x3FFF0016UL)
#define VI_ATTR_ASRL_BAUD       ((ViAttr)0x3FFF0021UL)
#define VI_ATTR_ASRL_DATA_BITS  ((ViAttr)0x3FFF0022UL)
#define VI_ATTR_ASRL_PARITY     ((ViAttr)0x3FFF0023UL)
#define VI_ATTR_ASRL_STOP_BITS  ((ViAttr)0x3FFF0024UL)

#define VI_ASRL_PAR_NONE  ((ViUInt16)0)
#define VI_ASRL_STOP_ONE  ((ViUInt16)10)

#define VI_TMO_INFINITE   ((ViUInt32)0xFFFFFFFFUL)

/* On x86-64 there is a single calling convention, so _VI_FUNC is empty. */
#ifndef _VI_FUNC
#  define _VI_FUNC
#endif

ViStatus _VI_FUNC viOpenDefaultRM(ViPSession vi);
ViStatus _VI_FUNC viOpen(ViSession sesn, const ViChar* name, ViAccessMode mode,
                         ViUInt32 timeout, ViPSession vi);
ViStatus _VI_FUNC viClose(ViObject vi);
ViStatus _VI_FUNC viWrite(ViSession vi, ViConstBuf buf, ViUInt32 cnt, ViPUInt32 retCnt);
ViStatus _VI_FUNC viRead(ViSession vi, ViBuf buf, ViUInt32 cnt, ViPUInt32 retCnt);
ViStatus _VI_FUNC viSetAttribute(ViObject vi, ViAttr attrName, ViAttrState attrValue);
ViStatus _VI_FUNC viGetAttribute(ViObject vi, ViAttr attrName, void* attrValue);
ViStatus _VI_FUNC viFindRsrc(ViSession sesn, const ViChar* expr, ViFindList* findList,
                             ViPUInt32 retcnt, ViChar desc[]);
ViStatus _VI_FUNC viFindNext(ViFindList findList, ViChar desc[]);
ViStatus _VI_FUNC viStatusDesc(ViObject vi, ViStatus status, ViChar desc[]);
ViStatus _VI_FUNC viClear(ViSession vi);

#ifdef __cplusplus
}
#endif

#endif /* __VISA_H__ */
