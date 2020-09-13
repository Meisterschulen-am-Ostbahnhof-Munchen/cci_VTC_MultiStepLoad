#ifndef VTCPOOL_C36FCA404E774BADA460EC6010EDC239
#define VTCPOOL_C36FCA404E774BADA460EC6010EDC239
#include "IsoDef.h"

/* https://de.wikipedia.org/wiki/Liste_der_ISO-639-1-Codes */
enum VTCLanguageCode
{
    lcUndefined = 0,
    lcBase = (('x' << 8) + 'x'),  // initial loader language
    lcEN =   (('e' << 8) + 'n'),  // default pool language
    lcDE =   (('d' << 8) + 'e'),  // german language
    lcSV =   (('s' << 8) + 'v'),  // swedish language
    lcA3 =   (('A' << 8) + '3')   // pool to be used for aux and CCI-A3
};

struct VTCPool
{
    enum VTCLanguageCode m_firstLanguage;                   // pool label being transferred first
    enum VTCLanguageCode m_finalLanguage;                   // pool label being transferred last
    enum VTCLanguageCode m_activeLanguage;                  // pool label currently being active
    enum VTCLanguageCode m_transferLanguage;                // pool label currently being transferred
    enum VTCLanguageCode m_vtLanguage;                      // VT Language
    enum VTCLanguageCode m_storedLanguages[POOLVERSIONS];   // list of stored labels on VT
    iso_u8 m_countStoredLanguages;                          // number of pools stored in VT
    iso_bool m_retryPoolLoad;                               // set if IsoPoolReload() has failed; retry in next cycle.
    iso_bool initialized;                                   // true: struct is properly initialized.
};

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void vtcPoolClear(struct VTCPool* vtcPool);                     // This function resets the above struct to its default values.
void vtcPoolInit(struct VTCPool* vtcPool,                       // This function initializes the above struct prior to a pool upload.
    iso_bool auxVT, 
    enum VTCLanguageCode vtLanguage_in, 
    iso_u8 au8VersionStrings[][LENVERSIONSTR], 
    iso_u8 count);
iso_bool vtcPoolUpdateVtLanguage(struct VTCPool* vt, enum VTCLanguageCode lc);

void vtcPoolGetPool(enum VTCLanguageCode lc, iso_u8** pData, iso_u32* pSize, iso_u16* pu16NumberObjects);
void vtcPoolGetPoolLabel(enum VTCLanguageCode lc, char* label);
void vtcPoolSetPoolManipulation(void);
enum VTCLanguageCode vtcPoolGetLanguageCode(const iso_u8* lcLabel);

void vtcPoolLoadHandler(struct VTCPool* vtcPool);               // This module processes the loading of the pools.
                                                                
#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* VTCPOOL_C36FCA404E774BADA460EC6010EDC239 */
