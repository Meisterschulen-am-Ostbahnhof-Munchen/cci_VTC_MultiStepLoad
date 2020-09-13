#include <stdint.h>
#include <map>
#include <vector>
#include <string.h>
#include <malloc.h>
#include "PreparePool.h"
#include "IsoVtcApi.h"

namespace PreparePool
{

static void itemizePool(
    const iso_u8* poolData, iso_u32 poolSize,
    std::map<uint16_t, std::vector<uint8_t>>& poolItems);

static bool preparePool(
    const iso_u8* srcPool, iso_u32 srcPoolSize,
    const iso_u8* macroList, iso_u8 macroListSize,
    iso_u8** basePool, iso_s32* basePoolSize,
    iso_u8** secondaryPool, iso_s32* secondaryPoolSize,
    iso_u8** gAuxPool, iso_s32* gAuxPoolSize);

static void releasePool(
    iso_u8** basePool,
    iso_u8** secondaryPool,
    iso_u8** gAuxPool);

static iso_u16 getU16(const iso_u8 data[])
{
    return static_cast<iso_u16>((data[1] << 8) + data[0]);
}

static void moveObject(iso_u16 objectID,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);
static void moveWorkingSet(iso_u16 objectID,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveContainer(iso_u16 objectID,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveDataMask(iso_u16 objectID,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveOutputStringField(iso_u16 objectID,
    std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
    std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveOutputNumberField(iso_u16 objectID,
    std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
    std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveTypRectangle(iso_u16 objectID,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveLineAttributesObject(iso_u16 objectID,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveFillAttributesObject(iso_u16 objectID,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveObjectPointer(iso_u16 objectID,
            std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
            std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveMacro(iso_u16 objectID,
            std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
            std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveAuxiliaryFunction2(iso_u16 objectID,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems,
                bool recursive = true);

static void moveObjectList(const std::vector<uint8_t>& poolItem,
                iso_u8 objectCount, uint32_t objectOffset,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

static void moveMacroList(const std::vector<uint8_t>& poolItem,
                iso_u8 macroCount, uint32_t macroOffset,
                std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems);

void itemizePool(const iso_u8* poolData, iso_u32 poolSize,
                   std::map<uint16_t, std::vector<uint8_t>>& poolItems)
{
    poolItems.clear();
    iso_u32 u32PoolSrcIdx = 0;
    while (u32PoolSrcIdx < poolSize)
    {
        const iso_u8* objectPoolData = &poolData[u32PoolSrcIdx];
        iso_u16 objectID = getU16(objectPoolData);
        iso_u32 objectSize = IsoPoolObjSize(objectPoolData);
        std::vector<uint8_t> poolItem(objectSize);
        memcpy(poolItem.data(), objectPoolData, objectSize);
        poolItems[objectID] = poolItem;
        u32PoolSrcIdx += objectSize;
    }
}

bool parsePool(const iso_u8 *srcPool, iso_u32 srcPoolSize, const iso_u8 *macroList, iso_u8 macroListSize, std::vector<iso_u8> &basePool, std::vector<iso_u8> &secondaryPool, std::vector<iso_u8> &gAuxPool)
{
    bool qRet = true;
    iso_u8* pBasePool = nullptr;
    iso_s32 basePoolSize = 0;
    iso_u8* pSecondaryPool = nullptr;
    iso_s32 secondaryPoolSize = 0;
    iso_u8* pGAuxPool = nullptr;
    iso_s32 gAuxPoolSize = 0;
    basePool.clear();
    secondaryPool.clear();
    gAuxPool.clear();
    if (preparePool(srcPool, srcPoolSize,
                    macroList, macroListSize,
                    &pBasePool, &basePoolSize,
                    &pSecondaryPool, &secondaryPoolSize,
                    &pGAuxPool, &gAuxPoolSize) == ISO_FALSE)
    {
        releasePool(&pBasePool, &pSecondaryPool, &pGAuxPool);
        qRet = ISO_FALSE;
    }
    else
    {
        if (basePoolSize > 0)
        {
            basePool.resize(static_cast<size_t>(basePoolSize));
            memcpy(basePool.data(),      pBasePool,      static_cast<size_t>(basePoolSize));
            basePoolSize = IsoGetNumofPoolObjs(pBasePool, basePoolSize);
        }

        if (secondaryPoolSize > 0)
        {
            secondaryPool.resize(static_cast<size_t>(secondaryPoolSize));
            memcpy(secondaryPool.data(), pSecondaryPool, static_cast<size_t>(secondaryPoolSize));
            secondaryPoolSize = IsoGetNumofPoolObjs(pSecondaryPool, secondaryPoolSize);
        }

        if (gAuxPoolSize > 0)
        {
            gAuxPool.resize(static_cast<size_t>(gAuxPoolSize));
            memcpy(gAuxPool.data(),      pGAuxPool,      static_cast<size_t>(gAuxPoolSize));
            gAuxPoolSize = IsoGetNumofPoolObjs(pGAuxPool, gAuxPoolSize);
        }

        releasePool(&pBasePool, &pSecondaryPool, &pGAuxPool);
    }

    return qRet;
}

bool preparePool(const iso_u8* srcPoolData, iso_u32 srcPoolSize,
    const iso_u8* macroList, iso_u8 macroListSize,
    iso_u8** pBasePool, iso_s32* pBasePoolSize,
    iso_u8** pSecondaryPool, iso_s32* pSecondaryPoolSize,
    iso_u8** pGAuxPool, iso_s32* pGAuxPoolSize)
{
    bool qRet = true;   // be positive
    iso_u16 u16NumberObjects = IsoGetNumofPoolObjs(srcPoolData, static_cast<iso_s32>(srcPoolSize));   //NumberObjects_glw
    if (u16NumberObjects == 0)
    {
        srcPoolSize = 0;
        qRet = false;
    }

    // split source pool into individual objects
    std::map<uint16_t, std::vector<uint8_t>> poolItems;
    itemizePool(srcPoolData, srcPoolSize, poolItems);

    // move objects relevant for the loader screen
    // 1. everything belonging to the working set
    // 2. all AuxiliaryFunction2 (with removed child objects)
    // 3. all relevent macros
    std::map<uint16_t, std::vector<uint8_t>> basePoolItems;
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = poolItems.find(0);
    if (it != poolItems.end())
    {
        std::vector<uint8_t>& poolItem = it->second;
        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == WorkingSet)
        {
            // object '0' needs to be working set
            moveWorkingSet(0, basePoolItems, poolItems);
        }
        else
        {
            poolItems.clear();
            qRet = false;
        }
    }
    else
    {
        poolItems.clear();
        qRet = false;
    }

    it = poolItems.begin();
    while (it != poolItems.end())
    {
        std::vector<uint8_t>& poolItem = it->second;
        iso_u16 objectID = getU16(&poolItem[0]);
        std::map<uint16_t, std::vector<uint8_t>>::iterator itDst = basePoolItems.find(objectID);
        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if ((eObjTyp == AuxiliaryFunction2) && (itDst == basePoolItems.end()))
        {
            moveAuxiliaryFunction2(objectID, basePoolItems, poolItems, false);

            // restaret loop, since the iterator has become invalid.
            it = poolItems.begin();
        }
        else
        {
            ++it;
        }
    }

    if ((qRet) && (macroList != nullptr) && (macroListSize>0))
    {
        for (iso_u8 idx = 0; idx < macroListSize; idx++)
        {
            moveObject(macroList[idx], basePoolItems, poolItems);
        }
    }

    std::map<uint16_t, std::vector<uint8_t>> auxPoolItems;
#if(1)
    it = poolItems.begin();
    while (it != poolItems.end())
    {
        std::vector<uint8_t> poolItem = it->second;
        iso_u16 objectID = getU16(&poolItem[0]);
        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == AuxiliaryFunction2)
        {
            moveAuxiliaryFunction2(objectID, auxPoolItems, poolItems);

            // restaret loop, since the iterator has become invalid.
            it = poolItems.begin();
        }
        else
        {
            ++it;
        }
    }
#endif

    //size_t basePoolSize = 0;
    //size_t secondaryPoolSize = 0;
    //size_t gAuxPoolSize = 0;
    uint32_t auxFunction2Count = 0;
    if (qRet)
    {
        // add aux into secondaryPool
        for (auto poolItem : auxPoolItems)
        {
            OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem.second[2]);
            if (eObjTyp == AuxiliaryFunction2)
            {
                ++auxFunction2Count;
            }

            poolItems[poolItem.first] = poolItem.second;
        }

        // add basePool into aux
        for (auto poolItem : basePoolItems)
        {
            if (auxPoolItems.find(poolItem.first) == auxPoolItems.end())
            {
                auxPoolItems[poolItem.first] = poolItem. second;
            }
        }

        iso_s32 basePoolSize = 0;
        for (auto poolItem : basePoolItems)
        {
            basePoolSize = basePoolSize + static_cast<iso_s32>(poolItem.second.size());
        }

        iso_s32 secondaryPoolSize = 0;
        for (auto poolItem : poolItems)
        {
            secondaryPoolSize = secondaryPoolSize + static_cast<iso_s32>(poolItem.second.size());
        }

        iso_s32 gAuxPoolSize = 0;
        for (auto poolItem : auxPoolItems)
        {
            gAuxPoolSize = gAuxPoolSize + static_cast<iso_s32>(poolItem.second.size());
        }

        iso_u8* basePool = (iso_u8*)malloc(basePoolSize);
        iso_u8* secondaryPool = (iso_u8*)malloc(secondaryPoolSize + gAuxPoolSize);
        iso_u8* gAuxPool = (iso_u8*)malloc(basePoolSize + gAuxPoolSize);

        iso_s32 poolIdx = 0;
        for (auto poolItem : basePoolItems)
        {
            memcpy((basePool) + poolIdx, poolItem.second.data(), poolItem.second.size());
            poolIdx += static_cast<iso_s32>(poolItem.second.size());
        }
        basePoolSize = poolIdx;

        poolIdx = 0;
        for (auto poolItem : poolItems)
        {
            memcpy((secondaryPool) + poolIdx, poolItem.second.data(), poolItem.second.size());
            poolIdx += static_cast<iso_s32>(poolItem.second.size());
        }
        secondaryPoolSize = poolIdx;

        poolIdx = 0;
        for (auto poolItem : auxPoolItems)
        {
            memcpy((gAuxPool) + poolIdx, poolItem.second.data(), poolItem.second.size());
            poolIdx += static_cast<iso_s32>(poolItem.second.size());
        }
        gAuxPoolSize = poolIdx;

        iso_u32 baseSize = IsoGetNumofPoolObjs(basePool, basePoolSize);
        iso_u32 secondarySize = IsoGetNumofPoolObjs(secondaryPool, secondaryPoolSize);
        iso_u32 gAuxSize = IsoGetNumofPoolObjs(gAuxPool, gAuxPoolSize);
        (void)gAuxSize;

        if ((secondarySize + baseSize - auxFunction2Count) != u16NumberObjects)
        {
            qRet = false;
        }

        if (pBasePool != nullptr)
        {
            if ((*pBasePool) != nullptr)
            {
                free(*pBasePool);
                *pBasePool = nullptr;
            }

            if (pBasePoolSize != nullptr)
            {
                *pBasePool = basePool;
                *pBasePoolSize = basePoolSize;
            }
            else
            {
                free(basePool);
            }
        }

        if (pSecondaryPool != nullptr)
        {
            if ((*pSecondaryPool) != nullptr)
            {
                free(*pSecondaryPool);
                *pSecondaryPool = nullptr;
            }

            if (pSecondaryPoolSize != nullptr)
            {
                *pSecondaryPool = secondaryPool;
                *pSecondaryPoolSize = secondaryPoolSize;
            }
            else
            {
                free(secondaryPool);
            }
        }

        if (pGAuxPool != nullptr)
        {
            if ((*pGAuxPool) != nullptr)
            {
                free(*pGAuxPool);
                *pGAuxPool = nullptr;
            }

            if (pGAuxPoolSize != nullptr)
            {
                *pGAuxPool = gAuxPool;
                *pGAuxPoolSize = gAuxPoolSize;
            }
            else
            {
                free(gAuxPool);
            }
        }
    }

    return qRet;
}

void releasePool(
    iso_u8** basePool,
    iso_u8** secondaryPool,
    iso_u8** gAuxPool)
{
    if ((basePool != nullptr) && ((*basePool) != nullptr))
    {
        free(*basePool);
        *basePool = nullptr;
    }

    if ((secondaryPool != nullptr) && ((*secondaryPool) != nullptr))
    {
        free(*secondaryPool);
        *secondaryPool = nullptr;
    }

    if ((gAuxPool != nullptr) && ((*gAuxPool) != nullptr))
    {
        free(*gAuxPool);
        *gAuxPool = nullptr;
    }
}

// Note: only relevent object types are moved recursively.
void moveObject(iso_u16 objectID,
                              std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
                              std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems)
{
    // TODO: recursive move
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        std::vector<uint8_t> poolItem = it->second;
        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        switch (eObjTyp)
        {
        case WorkingSet:               /*  0,  Top level object describes an implement (working set). */
            // error! There should be only one working set.
            moveWorkingSet(objectID, dstPoolItems, srcPoolItems);
            break;

        case DataMask:                /*  1,  Top level object that contains other objects. */
            moveDataMask(objectID, dstPoolItems, srcPoolItems);
            break;

        case Container:               /*  3,  Used to group objects. */
            moveContainer(objectID, dstPoolItems, srcPoolItems);
            break;

        case OutputStringField:       /* 11,   Used to output a character string. */
            moveOutputStringField(objectID, dstPoolItems, srcPoolItems);
            break;

        case OutputNumberField:       /* 12,   Used to output an integer or float numeric. */
            moveOutputNumberField(objectID, dstPoolItems, srcPoolItems);
            break;

        case TypRectangle:            /* 14,   Used to output a rectangle or square. */
            moveTypRectangle(objectID, dstPoolItems, srcPoolItems);
            break;

        case LineAttributesObject:    /* 24,   Used to group line based attributes. */
            moveLineAttributesObject(objectID, dstPoolItems, srcPoolItems);
            break;

        case FillAttributesObject:    /* 25,   Used to group fill based attributes. */
            moveFillAttributesObject(objectID, dstPoolItems, srcPoolItems);
            break;

        case ObjectPointer:           /* 27,   Used to reference another object. */
            moveObjectPointer(objectID, dstPoolItems, srcPoolItems);
            break;

        case Macro:                   /* 28,   Special object that contains a list of commands. */
            moveMacro(objectID, dstPoolItems, srcPoolItems);
            break;

        case AuxiliaryFunction2:      /* 31,   Defines the designator and function type 2. */
            //J.4.3     Auxiliary Function Type 2 object
            moveAuxiliaryFunction2(objectID, dstPoolItems, srcPoolItems);
            break;

//            /* Working set object */
//        case AlarmMask:               /*  2,  Top level object: Describes an alarm display. */
//            /* Key objects */
//        case SoftKeyMask:             /*  4,   Top level object that contains key objects. */
//        case Key:                     /*  5,   Used to describe a soft key. */
//        case Button:                  /*  6,   Used to describe a button control. */
//            /* Input field objects */
//        case InputBooleanField:       /*  7,   Used to input a ISO_TRUE/ISO_FALSE type input. */
//        case InputStringField:        /*  8,   Used to input a character string. */
//        case InputNumberField:        /*  9,   Used to input an integer or float numeric. */
//        case InputListField:          /* 10,   Used to select an item from a pre-defined list. */
//            /* Output field objects */
//        case TypLine:                 /* 13,   Used to output a line. */
//        case TypEllipse:              /* 15,   Used to output an ellipse or circle. */
//        case TypPolygon:              /* 16,   Used to output a polygon. */
//            /* Output graphic objects */
//        case Meter:                   /* 17,   Used to output a meter. */
//        case LinearBarGraph:          /* 18,   Used to output a linear bar graph. */
//        case ArchedBarGraph:          /* 19,   Used to output an arched bar graph. */
//            /* Picture graphic object */
        case PictureGraphic:          /* 20,   Used to output a picture graphic (bitmap). */
//            /* Variable objects */
//        case NumberVariable:          /* 21,   Used to store a 32-bit unsigned integer value. */
//        case StringVariable:          /* 22,   Used to store a fixed length string value. */
//            /* Attribute object */
        case FontAttributesObject:    /* 23,   Used to group font based attributes. */
//        case InputAttributesObject:   /* 26,   Used to specify a list of valid characters. */
//            /* Pointer object */
//            /* Macro object */
//            /* Auxiliary control */
//        case AuxiliaryFunction:       /* 29,   Defines the designator and function type. */
//        case AuxiliaryInput:          /* 30,   Defines the designator, key number, and function type. */
//        case AuxiliaryInput2:         /* 32,   Defines the designator, key number, and function type 2 */
//        case AuxiliaryConDesigObjPoi: /* 33,   Defines the auxiliary control designator object pointer */
//            /* Version 4 objects */
//        case WindowMaskObject:        /* 34,   Special (parent) object for user layout DM */
//        case KeyGroupObject:          /* 35,   Parent object of user layout SKM */
//        case GraphicsContextObject:   /* 36,   Graphics Context object */
//        case OutputListObject:        /* 37,   Used to output a output list */
//        case ExtInputAttributeObject: /* 38,   Extended input attribute object */
//        case ColourMapObject:         /* 39,   Changing the colour table */
//        case ObjectLabelReferList:    /* 40,   For assigning (textual/graphical) label to objects  */
//            /* Version 5 objects */
//        case ExternalObjectDef:       /* 41,   Lists objects which another WS can reference */
//        case ExternalRefName:         /* 42,   Identifies a WSM of a WS which can be referenced */
//        case ExternalObjectPointer:   /* 43,   Allows a WS to display objects from another WS */
//        case AnimationObject:         /* 44,   Used to display animations */
//            /* Version 6 objects */
//        case ColourPaletteObject:     /* 45,   Used for replacing VT standard colour palette */
//        case GraphicDataObject:       /* 46,   Graphic Data object; Used for displaying Portable Network Graphic object */
//        case WSSpecialControlsObject: /* 47,   Working Set Special Controls object */
//        case ScaledGraphicObject:     /* 48,   Scaled Graphic object */
            dstPoolItems[objectID] = poolItem;
            it = srcPoolItems.erase(it);
            it = srcPoolItems.end();
            eObjTyp = ObjectUndef;
            break;

        case ObjectUndef:
        default:
            dstPoolItems[objectID] = poolItem;
            it = srcPoolItems.erase(it);
            it = srcPoolItems.end();
            eObjTyp = ObjectUndef;
            break;
        }
    }
}

void moveWorkingSet(iso_u16 objectID,
                                  std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems,
                                  std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        // Table B.2 — Working Set attributes and record format

        // move working set sub-objects
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        uint16_t avtiveMask = getU16(&poolItem[5]);
        moveObject(avtiveMask, dstPoolItems, srcPoolItems);

        uint8_t objectCount = poolItem[7];
        uint32_t objectOffset = 10;
        moveObjectList(poolItem, objectCount, objectOffset, dstPoolItems, srcPoolItems);

        uint8_t macroCount = poolItem[8];
        uint32_t macroOffset = objectOffset + (6*objectCount);
        moveMacroList(poolItem, macroCount, macroOffset, dstPoolItems, srcPoolItems);
    }
}

void moveContainer(iso_u16 objectID, std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems, std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    // B.4   Container object
    // Table B.8 — Container attributes and record format
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        // Table B.2 — Working Set attributes and record format

        // move working set sub-objects
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == Container)
        {
            uint8_t objectCount = poolItem[8];
            uint32_t objectOffset = 10;
            moveObjectList(poolItem, objectCount, objectOffset, dstPoolItems, srcPoolItems);

            uint8_t macroCount = poolItem[9];
            uint32_t macroOffset = objectOffset + (6*objectCount);
            moveMacroList(poolItem, macroCount, macroOffset, dstPoolItems, srcPoolItems);
        }
    }
}

void moveDataMask(iso_u16 objectID, std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems, std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    // Table B.4 — Data mask attributes and record format
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        // Table B.2 — Working Set attributes and record format

        // move working set sub-objects
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == DataMask)
        {
            uint16_t softkeyMask = getU16(&poolItem[4]);
            moveObject(softkeyMask, dstPoolItems, srcPoolItems);

            uint8_t objectCount = poolItem[6];
            uint32_t objectOffset = 8;
            moveObjectList(poolItem, objectCount, objectOffset, dstPoolItems, srcPoolItems);

            uint8_t macroCount = poolItem[7];
            uint32_t macroOffset = objectOffset + (6*objectCount);
            moveMacroList(poolItem, macroCount, macroOffset, dstPoolItems, srcPoolItems);
        }
    }
}

void moveOutputStringField(iso_u16 objectID, std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems, std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    // B.9.2   Output String object
    // Table B.22 — Output String attributes and record format
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        // move working set sub-objects
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == OutputStringField)
        {
            uint16_t fontObject = getU16(&poolItem[8]);
            moveObject(fontObject, dstPoolItems, srcPoolItems);
        }
    }
}

void moveOutputNumberField(iso_u16 objectID, std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems, std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems)
{
    // B.9.3   Output Number object
    // Table B.23 — Output Number attributes and record format
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == OutputNumberField)
        {
            uint16_t fontAttributes = getU16(&poolItem[8]);
            moveObject(fontAttributes, dstPoolItems, srcPoolItems);

            uint16_t variableReference = getU16(&poolItem[11]);
            moveObject(variableReference, dstPoolItems, srcPoolItems);

            uint8_t macroCount = poolItem[28];
            uint32_t macroOffset = 29;
            moveMacroList(poolItem, macroCount, macroOffset, dstPoolItems, srcPoolItems);
        }
    }
}



void moveTypRectangle(iso_u16 objectID, std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems, std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    // Table B.29 — Output Rectangle attributes and record format
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == TypRectangle)
        {
            uint16_t lineAttributes = getU16(&poolItem[3]);
            moveObject(lineAttributes, dstPoolItems, srcPoolItems);

            uint16_t fillAttributes = getU16(&poolItem[10]);
            moveObject(fillAttributes, dstPoolItems, srcPoolItems);

            uint8_t macroCount = poolItem[12];
            uint32_t macroOffset = 13;
            moveMacroList(poolItem, macroCount, macroOffset, dstPoolItems, srcPoolItems);
        }
    }
}

void moveLineAttributesObject(iso_u16 objectID, std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems, std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    // Table B.48 — Line Attributes attributes and record format
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == LineAttributesObject)
        {
            uint8_t macroCount = poolItem[7];
            uint32_t macroOffset = 8;
            moveMacroList(poolItem, macroCount, macroOffset, dstPoolItems, srcPoolItems);
        }
    }
}

void moveFillAttributesObject(iso_u16 objectID, std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems, std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    // Table B.50 — Fill Attributes attributes and record format
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == FillAttributesObject)
        {
            uint8_t macroCount = poolItem[7];
            uint32_t macroOffset = 8;
            moveMacroList(poolItem, macroCount, macroOffset, dstPoolItems, srcPoolItems);
        }
    }
}

void moveObjectPointer(iso_u16 objectID, std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems, std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    // B.15 Object Pointer object

    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        // move working set sub-objects
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == ObjectPointer)
        {
            uint16_t nextObject = getU16(&poolItem[3]);
            moveObject(nextObject, dstPoolItems, srcPoolItems);
        }
    }
}

static void moveMacro(iso_u16 objectID,
            std::map<uint16_t, std::vector<uint8_t>>& dstPoolItems,
            std::map<uint16_t, std::vector<uint8_t>>& srcPoolItems)
{
    // B.16 Macro object
    // Table B.56 — Macro attributes and record format
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        std::vector<uint8_t> poolItem = it->second;
        dstPoolItems[objectID] = poolItem;
        it = srcPoolItems.erase(it);

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == Macro)
        {
            //TODO: check wether additional dependencies are required
        }
    }
}

void moveAuxiliaryFunction2(iso_u16 objectID,
                                          std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems,
                                          std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems,
                                          bool recursive)
{
    //J.4.3     Auxiliary Function Type 2 object
    std::map<uint16_t, std::vector<uint8_t>>::iterator it = srcPoolItems.find(objectID);
    if (it != srcPoolItems.end())
    {
        std::vector<uint8_t> poolItem = it->second;
        if (recursive)
        {
            dstPoolItems[objectID] = poolItem;
            it = srcPoolItems.erase(it);
        }

        OBJTYP_e eObjTyp = static_cast<OBJTYP_e>(poolItem[2]);
        if (eObjTyp == AuxiliaryFunction2)
        {
            if (recursive)
            {
                uint8_t objectCount = poolItem[5];
                moveObjectList(poolItem, objectCount, 6, dstPoolItems, srcPoolItems);
            }
            else
            {
                poolItem.resize(6);
                poolItem[5] = 0;
                dstPoolItems[objectID] = poolItem;
            }
        }
    }
}

void moveObjectList(const std::vector<uint8_t> &poolItem, iso_u8 objectCount, uint32_t objectOffset, std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems, std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    for (uint8_t idx = 0; idx < objectCount; ++idx)
    {
        uint16_t objectID = getU16(&poolItem[objectOffset + (6*idx)]);
        moveObject(objectID, dstPoolItems, srcPoolItems);
    }
}

void moveMacroList(const std::vector<uint8_t> &poolItem, iso_u8 macroCount, uint32_t macroOffset, std::map<uint16_t, std::vector<uint8_t> > &dstPoolItems, std::map<uint16_t, std::vector<uint8_t> > &srcPoolItems)
{
    for (uint8_t idx = 0; idx < macroCount; ++idx)
    {
        uint8_t objectID = poolItem[macroOffset + (2*idx) + 1];
        moveObject(objectID, dstPoolItems, srcPoolItems);
    }
}

} /* namespace PreparePool */
