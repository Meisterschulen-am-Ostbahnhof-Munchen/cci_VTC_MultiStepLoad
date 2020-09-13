/* ************************************************************************ */
/*!
   \file
   \brief       Helper functions for reading and writing settings to a file.

*/
/* ************************************************************************ */
#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include "settings.h"

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
    (void)section;
    (void)key;
    return defaultValue;
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
    (void)section;
    (void)key;
    return defaultValue;
}

uint64_t getX64(const char section[], const char key[], const uint64_t defaultValue)
{
    (void)section;
    (void)key;
    return defaultValue;
}

void getString(const char section[], const char key[], const char defaultValue[], char captionOut[], size_t size)
{
    (void)section;
    (void)key;
    std::string caption(defaultValue);
    strcpy(captionOut, caption.c_str());
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
    (void)section;
    (void)key;
    (void)value;
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
    (void)section;
    (void)key;
    (void)value;
}

void setX64(const char section[], const char key[], const uint64_t value)
{
    (void)section;
    (void)key;
    (void)value;
}

void setString(const char section[], const char key[], const char value[])
{
    (void)section;
    (void)key;
    (void)value;
}

size_t getSection(const char section[], char string[], size_t stringSize)
{
    (void)section;
    (void)string;
    (void)stringSize;
   return 0U;
}

void clearSection(const char section[])
{
    (void)section;
}

/* ************************************************************************ */
