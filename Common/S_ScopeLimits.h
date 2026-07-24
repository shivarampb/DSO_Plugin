/*=============================================================================
 *  S_ScopeLimits.h - INTERNAL per-model limits of the oscilloscope fleet.
 *
 *  *** THIS HEADER MUST NEVER BE SHIPPED TO END USERS. ***
 *
 *  One row per model. This header provides the S_ScopeLimits struct + the
 *  shared ScopeShared constants (used by every model plugin), plus a model
 *  CATALOG. The real model plugins are self-contained: each EMBEDS its own row
 *  in its constructor and does not read the catalog. The catalog exists for
 *  SimScope, which simulates any fleet member and looks its limits up by name
 *  - so getParameterRange reports identical limits for the simulated and the
 *  real instrument.
 *
 *  Adding a fleet member = a new self-contained model plugin (own functions +
 *  embedded row); optionally a catalog row here so SimScope knows it too - see
 *  docs/Adding_A_New_Model.md.
 *
 *  NOTE(manual): the numeric ranges below are seeded from published datasheet
 *  headline specs and standard SCPI dialects. Values tagged TODO(manual) must
 *  be verified page-by-page against each model's programming manual once the
 *  PDF is placed under manuals/<Model>/ (see the build prompt, section 1b).
 *===========================================================================*/
#ifndef S_SCOPELIMITS_H
#define S_SCOPELIMITS_H

#include <cstring>

struct S_ScopeLimits {
    const char* m_szModelName;      /* discovery / plugin name               */
    const char* m_szManufacturer;
    const char* m_szSeries;
    const char* m_szIdnMatch;       /* token expected inside *IDN?           */

    /* channel / acquisition topology                                        */
    int         m_iAnalogChannels;
    int         m_iDigitalChannels; /* 0 = not an MSO                        */
    double      m_dBandwidthHz;
    double      m_dMaxSampleRate;   /* Sa/s                                  */
    unsigned    m_u32MaxMemoryDepth;/* points                                */

    /* vertical (per channel)                                                */
    double      m_dVertScaleMin;    /* V/div min                             */
    double      m_dVertScaleMax;    /* V/div max                             */
    double      m_dVertOffsetMax;   /* +/- V                                 */

    /* horizontal / timebase                                                 */
    double      m_dTimebaseMin;     /* s/div min (fastest)                   */
    double      m_dTimebaseMax;     /* s/div max (slowest)                   */

    /* trigger                                                               */
    double      m_dTrigHoldoffMin;  /* s                                     */
    double      m_dTrigHoldoffMax;  /* s                                     */

    /* acquisition                                                           */
    int         m_iAvgCountMax;

    /* feature flags                                                         */
    bool        m_bHasAWG;
    bool        m_bHasFFT;
    bool        m_bHasDigital;
    bool        m_bHasSerialDecode;
    bool        m_bHasSegmented;

    /* built-in generator (0 if none)                                        */
    double      m_dAwgFreqMax;      /* Hz                                    */
    double      m_dAwgAmplMax;      /* Vpp                                   */

    /* USB identity                                                          */
    const char* m_szUsbVid;
    const char* m_szUsbPid;
};

/*-----------------------------------------------------------------------------
 * Constants shared by every fleet member.
 *---------------------------------------------------------------------------*/
namespace ScopeShared {

const int    GRATICULE_VDIV        = 8;        /* vertical divisions          */
const int    GRATICULE_HDIV        = 10;       /* horizontal divisions        */
const double TRIG_HOLDOFF_MIN      = 0.0;      /* s                           */
const double TRIG_HOLDOFF_MAX      = 10.0;     /* s                           */
const double MEASURE_THRESHOLD_MIN = -100.0;   /* V                           */
const double MEASURE_THRESHOLD_MAX = 100.0;    /* V                           */
const int    SETUP_MEMORY_MAX      = 10;       /* internal setup slots 0..9   */

/* discrete probe-attenuation ratios common across vendors                   */
const double PROBE_ATTEN_VALUES[]  = { 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0,
                                       20.0, 50.0, 100.0, 200.0, 500.0, 1000.0 };
const int    PROBE_ATTEN_COUNT     = 13;

/* discrete waveform-point counts a scope commonly accepts                   */
const double WAVEFORM_POINTS_VALUES[] = { 100.0, 250.0, 500.0, 1000.0, 2000.0,
                                          5000.0, 10000.0, 100000.0, 1000000.0 };
const int    WAVEFORM_POINTS_COUNT    = 9;

} /* namespace ScopeShared */

/*-----------------------------------------------------------------------------
 * The model catalog. Rows seeded from datasheet headline specs; TODO(manual).
 * Column order matches the struct declaration above.
 *---------------------------------------------------------------------------*/
inline const S_ScopeLimits* ScopeLimitsCatalog(int* out_piCount)
{
    static const S_ScopeLimits s_asCatalog[] = {
        /* MDO34 - Tektronix 3 Series MDO (mixed-domain, MSO opt, AWG opt) */
        { "MDO34", "Tektronix", "3 Series MDO", "MDO3",
          4, 16, 1.0e9, 2.5e9, 10000000u,
          1.0e-3, 10.0, 100.0,
          4.0e-10, 1000.0,
          0.0, 10.0,
          512,
          true, true, true, true, true,
          5.0e7, 5.0,
          "0x0699", "0x0522" },

        /* RTM3004 - Rohde & Schwarz RTM3000 (MSO opt, built-in gen) */
        { "RTM3004", "Rohde & Schwarz", "RTM3000", "RTM3004",
          4, 16, 1.0e9, 5.0e9, 40000000u,
          1.0e-3, 10.0, 100.0,
          1.0e-9, 500.0,
          0.0, 10.0,
          100000,
          true, true, true, true, true,
          2.5e7, 6.0,
          "0x0AAD", "0x01D6" },

        /* DSO7104B - Keysight InfiniiVision 7000 */
        { "DSO7104B", "Keysight", "InfiniiVision 7000", "DSO7104B",
          4, 0, 1.0e9, 4.0e9, 8000000u,
          1.0e-3, 5.0, 100.0,
          5.0e-10, 50.0,
          0.0, 10.0,
          65536,
          false, true, false, true, true,
          0.0, 0.0,
          "0x0957", "0x1745" },

        /* DSOS204A - Keysight Infiniium S-Series (deep memory, high sample) */
        { "DSOS204A", "Keysight", "Infiniium S-Series", "DSOS204A",
          4, 0, 2.0e9, 20.0e9, 100000000u,
          1.0e-3, 5.0, 100.0,
          1.0e-11, 20.0,
          0.0, 10.0,
          65536,
          false, true, false, true, true,
          0.0, 0.0,
          "0x2A8D", "0x900E" },

        /* DSOX2012A - Keysight InfiniiVision 2000 X (2-ch, WaveGen opt) */
        { "DSOX2012A", "Keysight", "InfiniiVision 2000 X", "DSOX2012A",
          2, 0, 1.0e8, 2.0e9, 100000u,
          1.0e-3, 5.0, 100.0,
          5.0e-9, 50.0,
          0.0, 10.0,
          65536,
          true, true, false, true, false,
          2.0e7, 5.0,
          "0x0957", "0x1796" },

        /* MSO6054A - Agilent 6000 Series MSO (4 analog + 16 digital) */
        { "MSO6054A", "Agilent", "6000 Series MSO", "MSO6054A",
          4, 16, 5.0e8, 4.0e9, 8000000u,
          1.0e-3, 5.0, 100.0,
          1.0e-9, 50.0,
          0.0, 10.0,
          65536,
          false, true, true, true, true,
          0.0, 0.0,
          "0x0957", "0x143B" },

        /* RTO2064 - Rohde & Schwarz RTO2000 (high end) */
        { "RTO2064", "Rohde & Schwarz", "RTO2000", "RTO2064",
          4, 0, 6.0e8, 10.0e9, 200000000u,
          1.0e-3, 10.0, 100.0,
          2.5e-11, 500.0,
          0.0, 10.0,
          16000000,
          false, true, false, true, true,
          0.0, 0.0,
          "0x0AAD", "0x0135" },

        /* TDS1012B - Tektronix TDS1000B (entry / legacy SCPI) */
        { "TDS1012B", "Tektronix", "TDS1000B", "TDS1012B",
          2, 0, 1.0e8, 1.0e9, 2500u,
          2.0e-3, 5.0, 50.0,
          2.5e-9, 50.0,
          0.0, 8.0,
          128,
          false, true, false, false, false,
          0.0, 0.0,
          "0x0699", "0x0367" },

        /* TDS2024C - Tektronix TDS2000C (entry / legacy SCPI) */
        { "TDS2024C", "Tektronix", "TDS2000C", "TDS2024C",
          4, 0, 2.0e8, 2.0e9, 2500u,
          2.0e-3, 5.0, 50.0,
          1.0e-9, 50.0,
          0.0, 8.0,
          128,
          false, true, false, false, false,
          0.0, 0.0,
          "0x0699", "0x03A4" },

        /* WaveSurfer 42Xs - Teledyne LeCroy WaveSurfer Xs (MAUI / X-Stream) */
        { "WaveSurfer42Xs", "Teledyne LeCroy", "WaveSurfer Xs", "WS42XS",
          4, 0, 4.0e8, 5.0e9, 25000000u,
          1.0e-3, 10.0, 100.0,
          2.0e-10, 50.0,
          0.0, 20.0,
          1000000,
          false, true, false, true, true,
          0.0, 0.0,
          "0x05FF", "0x1023" },
    };
    if (out_piCount != NULL) {
        *out_piCount = static_cast<int>(sizeof(s_asCatalog) / sizeof(s_asCatalog[0]));
    }
    return s_asCatalog;
}

inline const S_ScopeLimits* ScopeFindLimits(const char* in_szModelName)
{
    int iCount = 0;
    const S_ScopeLimits* psCatalog = ScopeLimitsCatalog(&iCount);
    for (int i = 0; i < iCount; ++i) {
        if (std::strcmp(psCatalog[i].m_szModelName, in_szModelName) == 0) {
            return &psCatalog[i];
        }
    }
    return NULL;
}

#endif /* S_SCOPELIMITS_H */
