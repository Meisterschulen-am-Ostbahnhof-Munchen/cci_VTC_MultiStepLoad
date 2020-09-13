/* ************************************************************************ */
/*!
   \file
   \brief       Helper functions for reading and writing settings to a file.

*/
/* ************************************************************************ */
#ifdef WIN32
#include <windows.h>
#endif
#include <glib.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include "settings.h"

#if defined(linux)
#define vswprintf_s swprintf
#define vsprintf_s snprintf
#define _strtoui64 strtoull
#define vswprintf_s swprintf
#define vsprintf_s snprintf
#define sprintf_s snprintf
#define strcpy_s strcpy
#endif // defined(linux)

/* ************************************************************************ */

static const char FILENAME[] = ".\\settings.ini";
#define MAXSTRLEN      1024U

static class Settings
{
public :
    Settings()
        : keyfile(g_key_file_new())
    {
        GKeyFileFlags flags = static_cast<GKeyFileFlags>(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);
        GError *error = nullptr;
        if (!g_key_file_load_from_file (keyfile, "./settings.ini", flags, &error))
        {
            FILE* handle = fopen("settings.ini", "w");
            if (handle != nullptr)
            {
                fclose(handle);
            }
        }

        g_key_file_set_string(keyfile, "HW", "test1", "Hello World!");
        g_key_file_set_integer(keyfile, "HW", "test2", 123);
        g_key_file_save_to_file(keyfile, "./settings.ini", nullptr);
        return;
    }

    ~Settings()
    {
        g_key_file_save_to_file(keyfile, "./settings.ini", nullptr);
    }

    void setString(const char* section, const char* key, const char* value)
    {
        g_key_file_set_string(keyfile, section, key, value);
        g_key_file_save_to_file(keyfile, "./settings.ini", nullptr);
    }

    uint32_t getString(const char* section, const char* key, const char* defaultValue, char* captionOut, uint32_t captionSize)
    {
        char *captionTemp = g_key_file_get_string(keyfile, section, key , nullptr);
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
        GError *error = nullptr;
        gint64 value = g_key_file_get_int64(keyfile, section, key, &error);
        if (error != nullptr)
        {
            g_key_file_set_int64(keyfile, section, key, defaultValue);
            g_key_file_save_to_file(keyfile, "./settings.ini", nullptr);
            return defaultValue;
        }

        return value;
    }


    GKeyFile *keyfile = nullptr;

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
   int64_t ret = getS64(section, key, defaultValue);
   return ((ret > INT8_MAX) || (ret < INT8_MIN)) ? defaultValue : (int8_t)(ret);
}

int16_t getS16(const char section[], const char key[], const int16_t defaultValue)
{
   int64_t ret = getS64(section, key, defaultValue);
   return ((ret > INT16_MAX) || (ret < INT16_MIN)) ? defaultValue : (int16_t)(ret);
}

int32_t getS32(const char section[], const char key[], const int32_t defaultValue)
{
   int64_t ret = getS64(section, key, defaultValue);
   return ((ret > INT32_MAX) || (ret < INT32_MIN)) ? defaultValue : (int32_t)(ret);
}

int64_t getS64(const char section[], const char key[], const int64_t defaultValue)
{
    return s_settings.getS64(section, key, defaultValue);
}

uint8_t getU8(const char section[], const char key[], const uint8_t defaultValue)
{
   uint64_t ret = getU64(section, key, defaultValue);
   return (ret > UINT8_MAX) ? defaultValue : (uint8_t)(ret);
}

uint16_t getU16(const char section[], const char key[], const uint16_t defaultValue)
{
   uint64_t ret = getU64(section, key, defaultValue);
   return (ret > UINT16_MAX) ? defaultValue : (uint16_t)(ret);
}

uint32_t getU32(const char section[], const char key[], const uint32_t defaultValue)
{
   uint64_t ret = getU64(section, key, defaultValue);
   return (ret > UINT32_MAX) ? defaultValue : (uint32_t)(ret);
}

uint64_t getU64(const char section[], const char key[], const uint64_t defaultValue)
{
   char buffer[MAXSTRLEN];
   uint64_t ret = defaultValue;
   uint32_t charCount = GetPrivateProfileStringA(section, key, NULL, buffer, sizeof(buffer), FILENAME);
   if (charCount == 0U)
   {
      sprintf_s(buffer, sizeof(buffer), "%ld", defaultValue);
      WritePrivateProfileStringA(section, key, buffer, FILENAME);
   }
   else
   {
      ret = _strtoui64(buffer, NULL, 10);
   }

   return ret;
}

uint64_t getX64(const char section[], const char key[], const uint64_t defaultValue)
{
   char buffer[MAXSTRLEN];
   uint64_t ret = defaultValue;
   uint32_t charCount = GetPrivateProfileStringA(section, key, NULL, buffer, sizeof(buffer), FILENAME);
   if (charCount == 0U)
   {
      sprintf_s(buffer, sizeof(buffer), "%lx", defaultValue);
      WritePrivateProfileStringA(section, key, buffer, FILENAME);
   }
   else
   {
      ret = _strtoui64(buffer, NULL, 16);
   }

   return ret;
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
   setS64(section, key, value);
}

void setS16(const char section[], const char key[], const int16_t value)
{
   setS64(section, key, value);
}

void setS32(const char section[], const char key[], const int32_t value)
{
   setS64(section, key, value);
}

void setS64(const char section[], const char key[], const int64_t value)
{
   char buffer[MAXSTRLEN];
   sprintf_s(buffer, sizeof(buffer), "%ld", value);
   WritePrivateProfileStringA(section, key, buffer, FILENAME);
}

void setU8(const char section[], const char key[], const uint8_t value)
{
    setU64(section, key, value);
}

void setU16(const char section[], const char key[], const uint16_t value)
{
    setU64(section, key, value);
}

void setU32(const char section[], const char key[], const uint32_t value)
{
    setU64(section, key, value);
}

void setU64(const char section[], const char key[], const uint64_t value)
{
    char buffer[MAXSTRLEN];
    sprintf_s(buffer, sizeof(buffer), "%lu", value);
    WritePrivateProfileStringA(section, key, buffer, FILENAME);
}

void setX64(const char section[], const char key[], const uint64_t value)
{
    char buffer[MAXSTRLEN];
    sprintf_s(buffer, sizeof(buffer), "%lx", value);
    WritePrivateProfileStringA(section, key, buffer, FILENAME);
}

void setString(const char section[], const char key[], const char value[])
{
    WritePrivateProfileStringA(section, key, value, FILENAME);
}

size_t getSection(const char section[], char string[], size_t stringSize)
{
   size_t sectionChars = (size_t)GetPrivateProfileSectionA(section, string, (uint32_t)stringSize, FILENAME);
   return sectionChars;
}

void clearSection(const char section[])
{
   // erase complete section
   WritePrivateProfileStringA(section, NULL, NULL, FILENAME);
}

/* ************************************************************************ */
