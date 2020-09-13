#include "VTCPool.h"
#include "PreparePool.h"

extern "C"
{
#include "MultiStepLoad/Output/MultiStepLoad.c.h"
#include "MultiStepLoad/Output/MultiStepLoad.ext.h"
#include "MultiStepLoad/Output/MultiStepLoad.iop.h"
#include "MultiStepLoad/Output/MultiStepLoad_de.iop.h"
}

static bool vtcContainsLanguage(VTCPool* vtcPool,       // This function checks wether a given language code is available.
    VTCLanguageCode lc);

static const char s_basePoolLabel[LENVERSIONSTR + 1] = "xxWHEPS                         ";  // 7 characters filled with blanks

static std::vector<iso_u8> s_basePool;      // derived pool; used as a start screen and for aux pool scan of isobus driver.
static std::vector<iso_u8> s_secondaryPool; // derived pool; english pool to be loaded after basePool.
                                            // incremental language pool are included as header
static std::vector<iso_u8> s_gAuxPool;      // derived pool; used for GAux / A3.
#if(0) // temporary variables for pool debugging
static std::map<uint16_t, std::vector<uint8_t>> evalItems;
static std::map<uint16_t, std::vector<uint8_t>> poolItems;
static std::map<uint16_t, std::vector<uint8_t>> basePoolItems;
static std::map<uint16_t, std::vector<uint8_t>> secondaryPoolItems;
static std::map<uint16_t, std::vector<uint8_t>> gAuxPoolItems;
#endif

static enum VTCLanguageCode vtcPoolGetFinalLanguage(enum VTCLanguageCode vtLanguage);
static iso_bool vtcPoolParsePool(void);         // This will initialize the required pools.
static iso_bool s_init = vtcPoolParsePool();

void vtcPoolInit(VTCPool* vt, iso_bool auxVT, enum VTCLanguageCode vtLanguage_in, iso_u8 au8VersionStrings[][LENVERSIONSTR], iso_u8 count)
{
    vtcPoolClear(vt);
    vt->m_vtLanguage = vtLanguage_in;
    vt->initialized = true;
    vt->m_countStoredLanguages = 0;
    for (iso_u8 idx = 0; idx < count; ++idx)
    {
        const iso_u8* versionString = &au8VersionStrings[idx][0];

        if (memcmp(&s_basePoolLabel[2], &versionString[2], LENVERSIONSTR - 2) != 0)
        {
            // the pool label does not match the last 30 Bytes (excluding the language bytes)
            IsoDeleteVersion(versionString);
        }
        else
        {
            VTCLanguageCode lc = vtcPoolGetLanguageCode(versionString);
            switch (lc)
            {
            case lcBase:
            case lcEN:
            case lcDE:
            case lcA3:
                vt->m_storedLanguages[vt->m_countStoredLanguages++] = lc;
                iso_DebugPrint("storedPool[%d]=%.7s.\n", idx, versionString);
                break;

            default:
                // language is not supported
                IsoDeleteVersion(versionString);
                break;
            }
        }
    }

    if (auxVT)
    {
        vt->m_transferLanguage = lcA3;
        vt->m_firstLanguage = lcA3;
        vt->m_finalLanguage = lcA3;
    }
    else
    {
        vt->m_finalLanguage = vtcPoolGetFinalLanguage(vtLanguage_in);
        VTCLanguageCode lc = lcUndefined;
        if (!vtcContainsLanguage(vt, lcBase))
        {
            lc = lcBase;
        }
        else if (vtcContainsLanguage(vt, vt->m_finalLanguage))
        {
            lc = vt->m_finalLanguage;
        }
        else if (vtcContainsLanguage(vt, lcEN))
        {
            lc = lcEN;
        }
        else
        {
            lc = lcBase;
        }

        vt->m_transferLanguage = lc;
        vt->m_firstLanguage = lc;
        iso_DebugPrint("getInitialLanguage %c%c\n", (char)(lc >> 8), (char)(lc));
    }
}

iso_bool vtcPoolUpdateVtLanguage(struct VTCPool* vt, enum VTCLanguageCode lc)
{
    vt->m_vtLanguage = lc;
    enum VTCLanguageCode finalLanguage = vtcPoolGetFinalLanguage(lc);
    if (vt->m_finalLanguage != finalLanguage)
    {
        vt->m_finalLanguage = finalLanguage;
        return ISO_TRUE;
    }

    return ISO_FALSE;
}

bool vtcContainsLanguage(VTCPool* vt, VTCLanguageCode lc)
{
    bool qRet = false;
    for (iso_u8 idx = 0; (idx < POOLVERSIONS) && (idx < vt->m_countStoredLanguages); ++idx)
    {
        if (vt->m_storedLanguages[idx] == lc)
        {
            qRet = true;
            break;
        }
    }

    return qRet;
}

enum VTCLanguageCode vtcPoolGetFinalLanguage(enum VTCLanguageCode vtLanguage)
{
    VTCLanguageCode lc = lcUndefined;
    switch (vtLanguage)
    {
    case lcDE:
    case lcSV:
        lc = lcDE;
        break;

    case lcEN:
    default:
        lc = lcEN;
        break;
    }

    return lc;
}

extern "C"
void vtcPoolGetPool(enum VTCLanguageCode lc, iso_u8** pData, iso_u32* pSize, iso_u16* pu16NumberObjects)
{
    iso_u8* data = nullptr;
    size_t size = 0U;
    iso_u16 numberOfObjects = 0U;

    switch (lc)
    {
    case lcA3:
        data = s_gAuxPool.data();
        size = s_gAuxPool.size();
        break;

    case lcBase:
        data = s_basePool.data();
        size = s_basePool.size();
        break;

    case lcDE:
    case lcSV:
        data = (iso_u8*)MultiStepLoad_de_iop;
        size = sizeof(MultiStepLoad_de_iop);
        break;

    case lcEN:
    default:
        data = s_secondaryPool.data();
        size = s_secondaryPool.size();
        break;
    }

    numberOfObjects = IsoGetNumofPoolObjs(data, static_cast<iso_s32>(size));
    *pData = data;
    *pSize = static_cast<iso_u32>(size);
    *pu16NumberObjects = numberOfObjects;
}

void vtcPoolGetPoolLabel(enum VTCLanguageCode lc, char* label)
{
    memcpy(label, s_basePoolLabel, LENVERSIONSTR + 1);
    label[0] = (char)(lc >> 8);
    label[1] = (char)(lc);
}

void vtcPoolSetPoolManipulation(void)
{
    iso_u16  u16DM_Scal = 10000u;          // Scaling factor * 10000
    iso_u16  u16SKM_Scal = 10000u;

    // ------------------------------------------------------------------------------

    // IsoPoolSetIDRangeMode(0, 60000, 10000, NoScaling);          // Switch off automatic scaling

    u16DM_Scal = (iso_u16)IsoPoolReadInfo(PoolDataMaskScalFaktor);       // Call only after PoolInit !!
    u16SKM_Scal = (iso_u16)IsoPoolReadInfo(PoolSoftKeyMaskScalFaktor);

    IsoPoolSetIDRangeMode(5100u, 5300u, u16SKM_Scal, Centering);       // Scale and center Keys
    IsoPoolSetIDRangeMode(20700u, 20799u, u16SKM_Scal, Scaling);         // Scale Pictures in keys


    // ------------------------------------------------------------------------------

    // Objects to be reloaded
    IsoPoolSetIDRangeMode(1100u, 1100u, 0u, NotLoad);
    IsoPoolSetIDRangeMode(40000u, 42000u, 0u, NotLoad);
    IsoPoolSetIDRangeMode(35000u, 35000u, 0u, NotLoad);

    IsoPoolSetIDRangeMode(0u, 0u, u16SKM_Scal, Centering);  // Working set object
    IsoPoolSetIDRangeMode(20000u, 20000u, u16SKM_Scal, Scaling);    // Working set designator
    IsoPoolSetIDRangeMode(29000u, 29099u, u16SKM_Scal, Centering);  // Auxiliary function
    IsoPoolSetIDRangeMode(20900u, 20999u, u16SKM_Scal, Scaling);    // Auxiliary bitmaps
    (void)u16DM_Scal;
}

iso_bool vtcPoolParsePool(void)
{
    //note: depending on the ISO Desigenr version the offset '1' is required.
    bool qRet = PreparePool::parsePool(isoOP_MultiStepLoad + 1, ISO_OP_MultiStepLoad_Size - 1,
        nullptr, 0,
        s_basePool,
        s_secondaryPool,
        s_gAuxPool);

#if(0) // temporary variables for pool debugging  
    itemizePool((iso_u8*)x0000602a00840aa0_xxWHEPS_iop, sizeof(x0000602a00840aa0_xxWHEPS_iop), evalItems);
    itemizePool((iso_u8*)&isoOP_MultiStepLoad[1], ISO_OP_MultiStepLoad_Size - 1U, poolItems);
    itemizePool(basePool.data(), static_cast<iso_u32>(basePool.size()), basePoolItems);
    itemizePool(secondaryPool.data(), static_cast<iso_u32>(secondaryPool.size()), secondaryPoolItems);
    itemizePool(gAuxPool.data(), static_cast<iso_u32>(gAuxPool.size()), gAuxPoolItems);
#endif

    if (!qRet)
    {
        iso_DebugPrint("pool parsing has failed.\n");
    }
    else
    {
        if (s_basePool.empty())
        {
            iso_DebugPrint("s_basePool is empty\n");
        }

        if (s_secondaryPool.empty())
        {
            iso_DebugPrint("s_secondaryPool is empty\n");
        }

        if (s_gAuxPool.empty())
        {
            iso_DebugPrint("s_gAuxPool is empty\n");
        }
    }

    return ISO_TRUE;
}

void vtcSetVTLanguage(VTCPool* vt, VTCLanguageCode lc)
{
    //    case IsoEvMaskLanguageCmd:
    iso_DebugPrint("setVTLanguage %c%c\n", (char)(lc >> 8), (char)(lc));
    vt->m_vtLanguage = lc;
}

VTCLanguageCode vtcPoolGetLanguageCode(const iso_u8* lc)
{
    VTCLanguageCode languageCode = lcUndefined;
    if (lc != nullptr)
    {
        iso_u16 temp = static_cast<iso_u16>((lc[0] << 8) | lc[1]);
        languageCode = static_cast<VTCLanguageCode>(temp);
        switch (languageCode)
        {
        case lcBase:
        case lcA3:
        case lcDE:
        case lcEN:
        case lcSV:
            break;

        default:
            languageCode = lcUndefined;
            break;
        }
    }

    return languageCode;
}

void vtcPoolLoadHandler(VTCPool* vt)
{
    vt->m_retryPoolLoad = false;
    if (vt->m_transferLanguage != lcUndefined)
    {
        if (vtcContainsLanguage(vt, vt->m_transferLanguage) == false)      
        {
            //TODO: this fails if being called through case "IsoEvMaskActivated"
            //      it works when being called through case "IsoEvMaskPoolReloadFinished" or "IsoEvAuxActivated"
            char actPoolLabel[LENVERSIONSTR + 1] = "xxWHEPS                         ";  // 7 characters filled with blanks
            actPoolLabel[0] = (char)(vt->m_transferLanguage >> 8);
            actPoolLabel[1] = (char)(vt->m_transferLanguage);
            if (vt->m_activeLanguage != lcUndefined)
            {
                // pool has not been stored through initial load
                IsoCmd_NumericValueRef(OutputNumber_12000, 0);
                IsoStoreVersion((iso_u8*)actPoolLabel);
            }

            vt->m_storedLanguages[vt->m_countStoredLanguages++] = vt->m_transferLanguage;
            iso_DebugPrint("poolLoadHandler: store pool %s\n", actPoolLabel);
        }

        vt->m_activeLanguage = vt->m_transferLanguage;
        vt->m_transferLanguage = lcUndefined;
    }

    VTCLanguageCode finalPoolLanguage = vt->m_finalLanguage;
    if (vt->m_activeLanguage == finalPoolLanguage)
    {
        // no further pool upload; change to active mask.
        iso_s16 s16Err = IsoCmd_ActiveMask(0, 1001);  /* Test of relaoded objects */
        iso_DebugPrint("poolLoadHandler: change active mask %d 1001\n", s16Err);
    }
    else
    {
        //qDebug() << "poolReload -- load finished:" << QString::fromStdString(m_poolLabel) << iso_BaseGetTimeMs();
        if (vtcContainsLanguage(vt, lcEN))
        {
            vt->m_transferLanguage = finalPoolLanguage;
        }
        else if (vtcContainsLanguage(vt, lcBase))
        {
            vt->m_transferLanguage = lcEN;
        }
        else
        {
            vt->m_transferLanguage = lcBase;
        }

        iso_u8* poolData = nullptr;
        iso_u32 poolSize = 0;
        iso_u16 u16NumberObjects = 0U;
        vtcPoolGetPool(vt->m_transferLanguage, &poolData, &poolSize, &u16NumberObjects);
        iso_bool success = IsoPoolReload(poolData, u16NumberObjects);
        iso_DebugPrint("poolLoadHandler, IsoPoolReload: %d = %x %d %d\n", success, vt->m_transferLanguage, poolSize, u16NumberObjects);
        if (success == ISO_FALSE)
        {
            vt->m_transferLanguage = lcUndefined;
            vt->m_retryPoolLoad = true;
            iso_DebugPrint("poolReload -- failed:\n");
        }
        else
        {
            vtcPoolSetPoolManipulation();
            iso_DebugPrint("poolReload -- next pool: %x\n", vt->m_transferLanguage);// << u16NumberObjects << iso_BaseGetTimeMs();
        }
    }
}

void vtcPoolClear(VTCPool* vt)
{
    vt->m_firstLanguage = lcUndefined;      // pool label being transferred first
    vt->m_finalLanguage = lcUndefined;      // pool label being transferred last
    vt->m_activeLanguage = lcUndefined;     // pool label currently being active
    vt->m_transferLanguage = lcUndefined;   // pool label to be transferred
    vt->m_vtLanguage = lcUndefined;         // VT Language
    for (iso_u8 idx = 0; idx < POOLVERSIONS; ++idx)
    {
        vt->m_storedLanguages[idx] = lcUndefined;    // list of stored labels on VT
    }

    vt->m_countStoredLanguages = 0;
    vt->m_retryPoolLoad = false;                       // set if IsoPoolReload() has failed; retry in next cycle.
    vt->initialized = false;                                // true: structure / class is properly initialized.
}
