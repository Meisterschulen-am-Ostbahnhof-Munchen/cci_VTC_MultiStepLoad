/* ************************************************************************ */


/* ************************************************************************ */
#include "IsoDef.h"

#if defined(_LAY6_)

#include <iostream>
#include <sstream>
#include <string.h>
#include "Settings/settings.h"
#include "AppMemAccess.h"

#if defined(ESP_PLATFORM)
#include <sys/param.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "AppMemAccess";

#endif // def ESP_PLATFORM

using namespace std;

#if defined(_MSC_VER )
#pragma warning(disable : 4996)
#endif // defined(_MSC_VER )
#if defined(linux) || defined(ESP_PLATFORM)
#define vswprintf_s swprintf
#define vsprintf_s snprintf
#define _strtoui64 strtoull
#define vswprintf_s swprintf
#define vsprintf_s snprintf
#define sprintf_s snprintf
#endif // defined(linux) || defined(ESP_PLATFORM)

#if defined(linux)
#define USE_L_FOR_64BIT
#elif defined(__MINGW_GCC_VERSION)
#define USE_LL_FOR_64BIT
#else // defined(linux), defined(__MINGW_GCC_VERSION)
#endif // defined(linux), defined(__MINGW_GCC_VERSION)
#if defined(ESP_PLATFORM)
   #define USE_LL_FOR_64BIT
#endif // def ESP_PLATFORM

static bool parseAuxEntry(char* entry, VT_AUXAPP_T* auxEntry);
static bool getKey(const VT_AUXAPP_T& auxEntry, char* key, size_t size);
static bool getValue(const VT_AUXAPP_T& auxEntry, char* value, size_t size);

/* ****************   Object pool access   *********************************** */


#if defined(ESP_PLATFORM)
static esp_err_t register_vfs()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret;
}
#endif // def ESP_PLATFORM


iso_u32 LoadPoolFromFile(const char * pcFilename, iso_u8 ** pPoolBuff)
{

	   /* load pool from file into RAM */
	   FILE *pFile;
	   iso_u32 u32PoolSize = 0;


#if defined(ESP_PLATFORM)
    if ( register_vfs() == ESP_OK ) {
#endif // def ESP_PLATFORM

	   if (*pPoolBuff)
	   {   /* free the pool data RAM */
		  free(*pPoolBuff);
		  *pPoolBuff = 0;
	   }

	   ESP_LOGI(TAG, "Opening file");
	   pFile = fopen(pcFilename, "rb");
	   if (pFile == NULL) {
	      ESP_LOGE(TAG, "Failed to open file for reading");
	   }
	   if (pFile)
	   {
		  fseek(pFile, 0L, SEEK_END);
		  u32PoolSize = static_cast<iso_u32>(ftell(pFile));
		  *pPoolBuff = reinterpret_cast<iso_u8*>(malloc(u32PoolSize));
		  fseek(pFile, 0L, SEEK_SET);
		  u32PoolSize = (iso_u32)fread(*pPoolBuff, sizeof(iso_u8), u32PoolSize, pFile);
		  fclose(pFile);
	   }

#if defined(ESP_PLATFORM)
       ESP_LOGI(TAG, "Pool size: %d, file: %s", u32PoolSize, pcFilename);
    }
#endif // def ESP_PLATFORM

   return u32PoolSize;
}



/* ****************   Auxiliary Assignments  *********************************** */

int IsoAuxReadAssignOfFile(VT_AUXAPP_T asAuxAss[])
{
   FILE *pFile;
   char sAssignment[100];
   int iCnt = 0;

   pFile = fopen("AuxAssignOfApp.txt", "rb");
   if (pFile == NULL)
   {
      std::cout << "Error opening AuxAssignOfApp.txt (not available or in wrong directory)" << endl;
      return 0;
   }
   else
   {
      while ((fgets(sAssignment, 100, pFile) != NULL))
      {
         int i;
         string str_Ass(sAssignment);

         for (i = 0; i<8; i++) {
            (asAuxAss + iCnt)->baAuxName[i] = 0xFFu;
         }

         string resToken;
         int curFld = 0;
         std::size_t PosStart = 0, PosEnd = 0;

         while (curFld <= 5)
         {
            //TRACE("Resulting token: %s\n", resToken);
            PosStart = str_Ass.find_first_of("0123456789abcdef", PosStart);
            PosEnd = str_Ass.find_first_of(" ,;", PosStart);
            if ((PosStart == string::npos) || (PosEnd == string::npos)) break;   // npos
            resToken = str_Ass.substr(PosStart, PosEnd - PosStart);
            switch (curFld)
            {
            case 0:
               asAuxAss[iCnt].wManuCode = static_cast<iso_u16>(atoi(resToken.c_str()));
               break;
            case 1:
               asAuxAss[iCnt].wModelIdentCode = static_cast<iso_u16>(atoi(resToken.c_str()));
               break;
            case 2:
               asAuxAss[iCnt].eAuxType = static_cast<VTAUXTYP_e>(atoi(resToken.c_str()));
               break;
            case 3:
               asAuxAss[iCnt].wObjID_Fun = static_cast<iso_u16>(atoi(resToken.c_str()));
               break;
            case 4:
               asAuxAss[iCnt].wObjID_Input = static_cast<iso_u16>(atoi(resToken.c_str()));
               break;
            case 5:
               for (int ii = 0; ii < 8; ii++)
               {
                  char    *stopstring;
                  string strByte = resToken.substr(ii * 2, 2);
                  asAuxAss[iCnt].baAuxName[ii] = (iso_u8)strtol(strByte.c_str(), &stopstring, 16);
               }
               break;
            default:
               break;
            }

            curFld++;
            PosStart = PosEnd;
         };
         iCnt++;
      };
      fclose(pFile);

   }
   return iCnt;
}


int IsoAuxWriteAssignToFile(VT_AUXAPP_T asAuxAss[], iso_s16 iNumberOfAssigns)
{
   FILE *pFile;
   pFile = fopen("AuxAssignOfApp.txt", "wb");
   if (pFile == nullptr)
   {
      std::cout << "Error writing AuxAssignOfApp.txt " << endl;
   }
   else
   {
      for (iso_s16 iI = 0; iI < iNumberOfAssigns; iI++)
      {
         std::string strLine, strName;
         stringstream ss;
         for (iso_s16 iII = 0; iII < 8; iII++)
         {
            char cstr[4];
            sprintf(cstr, "%02x", asAuxAss[iI].baAuxName[iII]);
            strName.append((string)cstr);
         }
         ss << "  " << asAuxAss[iI].wManuCode << "   ";
         ss << asAuxAss[iI].wModelIdentCode << "   ";
         ss << asAuxAss[iI].eAuxType << "   ";
         ss << asAuxAss[iI].wObjID_Fun << "   ";
         ss << asAuxAss[iI].wObjID_Input << "   ";
         strLine.append(ss.str());
         strLine.append(strName);
         strLine.append("\r\n");

         fputs(strLine.c_str(), pFile);
      }
      fclose(pFile);
   }
   return 0;
}


int getAuxAssignment(const char auxSection[], VT_AUXAPP_T asAuxAss[])
{
   char sectionData[4096] = { '\0' };
   size_t sectionChars = getSection(auxSection, &sectionData[0], sizeof(sectionData));
   size_t idxData = 0U;
   size_t idxAux = 0U;
   while (idxData < sectionChars)
   {
      char* entry = &(sectionData[idxData]);
      VT_AUXAPP_T* auxEntry = &asAuxAss[idxAux];
      if (parseAuxEntry(entry, auxEntry))
      {
          char key[64];
          char value[64];
          getKey(*auxEntry, key, sizeof(key));
          getValue(*auxEntry, value, sizeof(value));
          iso_DebugPrint("getAuxAssignment: %d %s %s\n", idxAux, key, value);
          idxAux++;
      }

      idxData += (strlen(entry) + 1U);
   }

   return (int)idxAux;
}


bool parseAuxEntry(char* entry, VT_AUXAPP_T* auxEntry)
{
    int wObjID_Fun;
    int wObjID_Input;
    int eAuxType;
    int wManuCode;
    int wModelIdentCode;
    int qPrefAssign;
    int bFuncAttribute;
    uint64_t name;
    int parameterCount = sscanf(entry,
#if defined(USE_L_FOR_64BIT)
        "%d=%d,%d,%d,%d,%d,%d,%lX", &wObjID_Fun,
#elif defined(USE_LL_FOR_64BIT)
        "%d=%d,%d,%d,%d,%d,%d,%llX", &wObjID_Fun,
#else // !defined(USE_L_FOR_64BIT)
        "%d=%d,%d,%d,%d,%d,%d,%I64X", &wObjID_Fun,
#endif //!defined(USE_L_FOR_64BIT)
        &wObjID_Input, &eAuxType, &wManuCode, &wModelIdentCode,
        &qPrefAssign, &bFuncAttribute, &name);
    if (parameterCount == 8)
    {
        auxEntry->wObjID_Fun = static_cast<iso_u16>(wObjID_Fun);              /* Object ID of auxiliary function */
        auxEntry->wObjID_Input = static_cast<iso_u16>(wObjID_Input);          /* Object ID of auxiliary input */
        auxEntry->eAuxType = static_cast<VTAUXTYP_e>(eAuxType);      /* Function/input type (without attribute bits)
                                                            - only because of downwards compatibility  */
        auxEntry->wManuCode = static_cast<iso_u16>(wManuCode);                /* Manufacturer Code of auxiliary input device */
        auxEntry->wModelIdentCode = static_cast<iso_u16>(wModelIdentCode);    /* Model identification code of aux input device */
        auxEntry->qPrefAssign = static_cast<iso_bool>(qPrefAssign);            /* This assignment shall used for preferred assignment */
        auxEntry->bFuncAttribute = static_cast<iso_u8>(bFuncAttribute);      /* Complete function attribute byte of auxiliary function */
        memcpy(&auxEntry->baAuxName[0], &name, 8);      /* ISO name of the auxiliary input device. The bytes must be set to 0xFF if not used. */
        return true;
    }

    return false;
}

void setAuxAssignment(const char section[], VT_AUXAPP_T asAuxAss[], iso_s16 iNumberOfAssigns)
{
   char buffer[512];
   char key[16];

   // erase complete section
   clearSection(section);

   // write aux entries
   for (int8_t idx = 0; idx < iNumberOfAssigns; idx++)
   {
      VT_AUXAPP_T* auxEntry = &asAuxAss[idx];
      sprintf_s(key, sizeof(key), "%d", auxEntry->wObjID_Fun);
      uint64_t name = 0;
      memcpy(&name, &auxEntry->baAuxName[0], 8);            /* ISO name of the auxiliary input device. The bytes must be set to 0xFF if not used. */
#if defined(USE_L_FOR_64BIT)
      sprintf_s(buffer, sizeof(buffer), "%d,%d,%d,%d,%d,%d,%lX",
#elif defined(USE_LL_FOR_64BIT)
      sprintf_s(buffer, sizeof(buffer), "%d,%d,%d,%d,%d,%d,%llX",
#else // !defined(USE_L_FOR_64BIT)
      sprintf_s(buffer, sizeof(buffer), "%d,%d,%d,%d,%d,%d,%I64X",
#endif //!defined(USE_L_FOR_64BIT)
         auxEntry->wObjID_Input, auxEntry->eAuxType, auxEntry->wManuCode, auxEntry->wModelIdentCode,
         auxEntry->qPrefAssign, auxEntry->bFuncAttribute, name);
      setString(section, key, buffer);
   }
}

void updateAuxAssignment(const char auxSection[], VT_AUXAPP_T* sAuxAss)
{
    if (sAuxAss->wObjID_Input != 0xFFFF)
    {
        char key[64];
        char value[64];
        getKey(*sAuxAss, key, sizeof(key));
        getValue(*sAuxAss, value, sizeof(value));
        iso_DebugPrint("updateAuxAssignment add: %s %s\n", key, value);
        setString(auxSection, key, value);
    }
    else
    {
        iso_s16 auxCfHandle = IsoClGetCfHandleToName(ISO_CAN_VT, &sAuxAss->baAuxName);
        iso_u16 wModelIdentCode = 0;
        if (IsoReadAuxInputDevModIdentCode(auxCfHandle, &wModelIdentCode) == E_NO_ERR)
        {
            sAuxAss->wModelIdentCode = wModelIdentCode;
        }

        char key[64];
        getKey(*sAuxAss, key, sizeof(key));
        iso_DebugPrint("updateAuxAssignment remove: %s\n", key);
        setString(auxSection, key, nullptr);
    }
}

static bool getKey(const VT_AUXAPP_T& auxEntry, char* key, size_t size)
{
#if defined(linux)
    sprintf_s(key, size, "%d",
#else // defined(linux)
    sprintf_s(key, size, "%d",
#endif // defined(linux)
        auxEntry.wObjID_Fun);
    return true;
}

static bool getValue(const VT_AUXAPP_T& auxEntry, char* value, size_t size)
{
    uint64_t name = 0;
    memcpy(&name, &auxEntry.baAuxName[0], 8);            /* ISO name of the auxiliary input device. The bytes must be set to 0xFF if not used. */
#if defined(USE_L_FOR_64BIT)
    sprintf_s(value, size, "%d,%d,%d,%d,%d,%d,%lX",
#elif defined(USE_LL_FOR_64BIT)
    sprintf_s(value, size, "%d,%d,%d,%d,%d,%d,%llX",
#else // !defined(USE_L_FOR_64BIT)
    sprintf_s(value, size, "%d,%d,%d,%d,%d,%d,%I64X",
#endif // !defined(USE_L_FOR_64BIT)
        auxEntry.wObjID_Input, auxEntry.eAuxType, auxEntry.wManuCode, auxEntry.wModelIdentCode,
        auxEntry.qPrefAssign, auxEntry.bFuncAttribute, name);
    return true;
}

/* ************************************************************************ */
#endif /* defined(_LAY6_) */
/* ************************************************************************ */
