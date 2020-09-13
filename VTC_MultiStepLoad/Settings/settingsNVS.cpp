/* ************************************************************************ */
/*!
   \file
   \brief       Helper functions for reading and writing settings to a file.

*/
/* ************************************************************************ */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include "settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#if 1
#define vswprintf_s swprintf
#define vsprintf_s snprintf
#define _strtoui64 strtoull
#define vswprintf_s swprintf
#define vsprintf_s snprintf
#define sprintf_s snprintf
#define strcpy_s strcpy
#endif // 1

/* ************************************************************************ */

static const char FILENAME[] = ".\\settings.ini";
#define MAXSTRLEN      1024U
static class Settings 
{
private:
	nvs_handle_t my_handle;
public :
    Settings()
    {
    	my_handle = 0;
    }
    void init(void)
    {
		// Initialize NVS
		esp_err_t err = nvs_flash_init();
		if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
		{
			// NVS partition was truncated and needs to be erased
			// Retry nvs_flash_init
			ESP_ERROR_CHECK (nvs_flash_erase());
			err = nvs_flash_init();
		}
		ESP_ERROR_CHECK(err);
		// Open
		printf("\n");
		printf("Opening Non-Volatile Storage (NVS) handle... ");
		err = nvs_open("storage", NVS_READWRITE, &my_handle);
		if (err != ESP_OK) {
			printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
		} else {
			printf("Done\n");
		}
        return;
    }

    ~Settings()
    {
        // Close
        nvs_close(my_handle);
    }

    void setString(const char* section, const char* key, const char* value)
    {

    }

    uint32_t getString(const char* section, const char* key, const char* defaultValue, char* captionOut, uint32_t captionSize)
    {
        char *captionTemp = nullptr;
        if (captionTemp == nullptr)
        {
            return 0U;
        }

        std::string caption(captionTemp);
        strcpy(captionOut, caption.c_str());
        return static_cast<uint32_t>(caption.size());
    }

    int64_t getS64(const char* section, const char* key, const int64_t defaultValue)
    {


        return 0;
    }


} s_settings;

uint32_t GetPrivateProfileStringA(
    const char* section,
    const char* lpKeyName,
    const char* lpDefault,
    char* lpReturnedString,
    uint32_t nSize,
    const char* lpFileName )
{
    (void)lpFileName;
    return s_settings.getString(section, lpKeyName, lpDefault, lpReturnedString, nSize);
}

bool WritePrivateProfileStringA(
    const char* section,
    const char* lpKeyName,
    const char* lpString,
    const char* lpFileName
    )
{
    s_settings.setString(section, lpKeyName, lpString);
    (void)lpFileName;
    return true;
}

uint32_t GetPrivateProfileSectionA(
    const char* section,
    char* lpReturnedString,
    uint32_t nSize,
    const char* lpFileName
    )
{
    return 0;
}

/* ************************************************************************ */

int8_t getS8(const char section[], const char key[], const int8_t defaultValue)
{
   return 0;
}

int16_t getS16(const char section[], const char key[], const int16_t defaultValue)
{
	   return 0;
}

int32_t getS32(const char section[], const char key[], const int32_t defaultValue)
{
	   return 0;
}

int64_t getS64(const char section[], const char key[], const int64_t defaultValue)
{
    return s_settings.getS64(section, key, defaultValue);
}

uint8_t getU8(const char section[], const char key[], const uint8_t defaultValue)
{
	   return 0;
}

uint16_t getU16(const char section[], const char key[], const uint16_t defaultValue)
{
	   return 0;
}

uint32_t getU32(const char section[], const char key[], const uint32_t defaultValue)
{
	   return 0;
}

uint64_t getU64(const char section[], const char key[], const uint64_t defaultValue)
{
	   return 0;
}

uint64_t getX64(const char section[], const char key[], const uint64_t defaultValue)
{
	   return 0;
}

void getString(const char section[], const char key[], const char defaultValue[], char caption[], size_t size)
{
   static char buffer[MAXSTRLEN];
   uint32_t charCount = GetPrivateProfileStringA(section, key, NULL, buffer, sizeof(buffer), FILENAME);
   if (charCount == 0U)
   {
      WritePrivateProfileStringA(section, key, defaultValue, FILENAME);
      strcpy_s(caption, defaultValue);
   }
   else
   {
      strcpy_s(caption, buffer);
   }
}

void setS8(const char section[], const char key[], const int8_t value)
{

}

void setS16(const char section[], const char key[], const int16_t value)
{

}

void setS32(const char section[], const char key[], const int32_t value)
{

}

void setS64(const char section[], const char key[], const int64_t value)
{

}

void setU8(const char section[], const char key[], const uint8_t value)
{

}

void setU16(const char section[], const char key[], const uint16_t value)
{

}

void setU32(const char section[], const char key[], const uint32_t value)
{

}

void setU64(const char section[], const char key[], const uint64_t value)
{

}

void setX64(const char section[], const char key[], const uint64_t value)
{

}

void setString(const char section[], const char key[], const char value[])
{

}

size_t getSection(const char section[], char string[], size_t stringSize)
{
   size_t sectionChars = 0;
   return sectionChars;
}

void clearSection(const char section[])
{
   // erase complete section

}

/* ************************************************************************ */
