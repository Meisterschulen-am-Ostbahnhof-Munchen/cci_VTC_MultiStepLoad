/* ************************************************************************ */
/*!
   \file
   \brief       Helper functions for reading and writing settings to a file.

*/
/* ************************************************************************ */

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include "settings.h"

/* ************************************************************************ */

static const char FILENAME[] = ".\\settings.ini";
#define MAXSTRLEN      1024U

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
   char buffer[MAXSTRLEN];
   int64_t ret = defaultValue;
   uint32_t charCount = GetPrivateProfileStringA(section, key, NULL, buffer, sizeof(buffer), FILENAME);
   if (charCount == 0U)
   {
      sprintf_s(buffer, sizeof(buffer), "%I64d", defaultValue);
      WritePrivateProfileStringA(section, key, buffer, FILENAME);
   }
   else
   {
      ret = _strtoi64(buffer, NULL, 10);
   }

   return ret;
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
      sprintf_s(buffer, sizeof(buffer), "%I64d", defaultValue);
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
      sprintf_s(buffer, sizeof(buffer), "%I64x", defaultValue);
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
      strcpy_s(caption, size, defaultValue);
   }
   else
   {
      strcpy_s(caption, size, buffer);
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
   sprintf_s(buffer, sizeof(buffer), "%I64d", value);
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
    sprintf_s(buffer, sizeof(buffer), "%I64u", value);
    WritePrivateProfileStringA(section, key, buffer, FILENAME);
}

void setX64(const char section[], const char key[], const uint64_t value)
{
    char buffer[MAXSTRLEN];
    sprintf_s(buffer, sizeof(buffer), "%I64x", value);
    WritePrivateProfileStringA(section, key, buffer, FILENAME);
}

void setString(const char section[], const char key[], const char value[])
{
    WritePrivateProfileStringA(section, key, value, FILENAME);
}

size_t getSection(const char section[], char string[], size_t stringSize)
{
   size_t sectionChars = (size_t)GetPrivateProfileSectionA(section, string, (DWORD)stringSize, FILENAME);
   return sectionChars;
}

void clearSection(const char section[])
{
   // erase complete section
   WritePrivateProfileStringA(section, NULL, NULL, FILENAME);
}

/* ************************************************************************ */
