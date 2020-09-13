/* ************************************************************************ */
/*!
   \file           

   \brief      VT Client application  

   \author     Erwin Hammerl
   \date       Created 17.09.2007 
   \copyright  Wegscheider Hammerl Ingenieure Partnerschaft

   \par HISTORY:

*/

/* **************************  includes ********************************** */


#include "IsoDef.h"
#include "Common/IsoUtil.h"

#ifdef _LAY6_  /* compile only if VT client is enabled */

#include "Settings/settings.h"
#include "AppMemAccess.h"
#include "AppCommon/AppOutput.h"

#include "App_VTClient.h"

#if defined(_LAY6_) && defined(ISO_VTC_GRAPHIC_AUX)
#include "../Samples/VtcWithAuxPoolUpload/GAux.h"
#endif // defined(_LAY6_) && defined(ISO_VTC_GRAPHIC_AUX)

#include "MultiStepLoad/Output/MultiStepLoad.iop.h"

#include "pools/VTCPool.h"
#define CL_SIZELC               (6u)  /**< Number of data of a language command */

/* ****************************** global data   *************************** */
static iso_s16  s16_CfHndVtClient = HANDLE_UNVALID;      // Stored CF handle of VT client
static struct VTCPool m_primaryVt;
static struct VTCPool m_auxVt;
static iso_u32 updateTick = 0;

/* ****************************** function prototypes ****************************** */
static void CbVtConnCtrl        (const ISOVT_EVENT_DATA_T* psEvData);
static void CbVtStatus          (const ISOVT_STATUS_DATA_T* psStatusData);
static void CbVtMessages        (const ISOVT_MSG_STA_T * pIsoMsgSta);
static void CbAuxPrefAssignment (VT_AUXAPP_T asAuxAss[], iso_s16* ps16MaxNumberOfAssigns, ISO_USER_PARAM_T userParam);

static void AppPoolSettings(iso_bool auxVT, struct VTCPool* vtcPool);
static void AppVTClientDoProcess(void);

static void VTC_SetObjValuesBeforeStore(void);

static void VTC_setNewVT(void);
static void VTC_setPage2(void);
static void VTC_handleSoftkeys(const ISOVT_MSG_STA_T * pIsoMsgSta);

/* ************************************************************************ */
void AppVTClientLogin(iso_s16 s16CfHandle)
{
   ISO_USER_PARAM_T  userParamVt = ISO_USER_PARAM_DEFAULT;
   uint64_t u64Name = 0;
   iso_u8 u8BootTime = 0;
   ISO_CF_NAME_T     au8NamePreferredVT = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

   u64Name = getX64("CF-A", "preferredVT", 0xFFFFFFFFFFFFFFFFU);
   au8NamePreferredVT[0] = (iso_u8)( u64Name        & 0xFFU);
   au8NamePreferredVT[1] = (iso_u8)((u64Name >>  8) & 0xFFU);
   au8NamePreferredVT[2] = (iso_u8)((u64Name >> 16) & 0xFFU);
   au8NamePreferredVT[3] = (iso_u8)((u64Name >> 24) & 0xFFU);
   au8NamePreferredVT[4] = (iso_u8)((u64Name >> 32) & 0xFFU);
   au8NamePreferredVT[5] = (iso_u8)((u64Name >> 40) & 0xFFU);
   au8NamePreferredVT[6] = (iso_u8)((u64Name >> 48) & 0xFFU);
   au8NamePreferredVT[7] = (iso_u8)((u64Name >> 56) & 0xFFU);

   u8BootTime = getU8("CF-A", "bootTimeVT", 7u);

   // Initialize the VT client instance
   (void)IsoVTInit(s16CfHandle, userParamVt, CbVtStatus, CbVtMessages, CbVtConnCtrl, &au8NamePreferredVT);      
   (void)IsoVTDataSet(VT_BOOTTIME, u8BootTime);   // Set (EE-stored) boot time of the preferred VT (in seconds)
   
   // Use preferred assignment callback function, which is called before sending the preferred assignment for the auxiliary functions to the VT
   (void)IsoAuxPrefAssignmentCbSet(&CbAuxPrefAssignment);

#if defined(_LAY6_) && defined(ISO_VTC_GRAPHIC_AUX)
   /* Add CF to graphical aux implements */
   (void)vtcGAux_CfInit(s16CfHandle, userParamVt, "pools/Pool.iop");
#endif // defined(_LAY6_) && defined(ISO_VTC_GRAPHIC_AUX)

   s16_CfHndVtClient = s16CfHandle; // Store VT client CF handle

   vtcPoolClear(&m_primaryVt);
   vtcPoolClear(&m_auxVt);
   updateTick = iso_BaseGetTimeMs();
}

/* ************************************************************************ */
static void CbVtConnCtrl(const ISOVT_EVENT_DATA_T* psEvData)
{
   iso_u8 abLCData[6];
   iso_s16 vtHandle = (iso_s16)(IsoGetVTStatusInfo(VT_HND));

   switch (psEvData->eEvent)
   {
   case IsoEvConnSelectPreferredVT:
      //iso_DebugPrint("cf(%04X), IsoEvConnSelectPreferredVT(%d)\n", vtHandle, psEvData->eEvent);
      /* preferred VT is not alive, but one or more other VTs */
      VTC_setNewVT();
      break;
   case IsoEvMaskServerVersAvailable:
       iso_DebugPrint("cf(%04X), IsoEvMaskServerVersAvailable(%d)\n", vtHandle, psEvData->eEvent);
      if (IsoGetVTStatusInfo(VT_VERSIONNR) >= 4u)
      {
         // IsoVTObjTypeParsableSet(PNGObject);  // for test purposes (must be called here)
      }
      break;
   case IsoEvMaskLanguageCmd:
       iso_DebugPrint("cf(%04X), IsoEvMaskLanguageCmd(%d)\n", vtHandle, psEvData->eEvent);
       if (m_primaryVt.initialized)
       {
           IsoReadWorkingSetLanguageData(s16_CfHndVtClient, abLCData);
           if (vtcPoolUpdateVtLanguage(&m_primaryVt, vtcPoolGetLanguageCode(abLCData)))
           {
               IsoCmd_ActiveMask(0, 1000);
               vtcPoolLoadHandler(&m_primaryVt);
           }
       }
      break;
   case IsoEvMaskTechDataV4Request:
       iso_DebugPrint("cf(%04X), IsoEvMaskTechDataV4Request(%d)\n", vtHandle, psEvData->eEvent);
      /* If VT >= V4 then application can request some more technical data */
      if (IsoGetVTStatusInfo(VT_VERSIONNR) >= 4u)
      {
         IsoGetSupportedObjects();
         IsoGetWindowMaskData();
         //IsoGetSupportedWidechar(...)
      }
      break;
   case IsoEvMaskLoadObjects:
       iso_DebugPrint("cf(%04X), IsoEvMaskLoadObjects(%d)\n", vtHandle, psEvData->eEvent);
       AppPoolSettings(ISO_FALSE, &m_primaryVt);
      {  /* Current VT and boot time of VT can be read and stored here in EEPROM */
         iso_s16 s16HndCurrentVT = (iso_s16)IsoGetVTStatusInfo(VT_HND);   /* get CF handle of actual VT */
         ISO_CF_INFO_T cfInfo = {0};
         iso_s16 s16Err = iso_NmGetCfInfo( s16HndCurrentVT, &cfInfo );
         if (s16Err == E_NO_ERR)
         {
            uint64_t u64Name = ((uint64_t)(cfInfo.au8Name[0]))       |
                               ((uint64_t)(cfInfo.au8Name[1]) <<  8) |
                               ((uint64_t)(cfInfo.au8Name[2]) << 16) |
                               ((uint64_t)(cfInfo.au8Name[3]) << 24) |
                               ((uint64_t)(cfInfo.au8Name[4]) << 32) |
                               ((uint64_t)(cfInfo.au8Name[5]) << 40) |
                               ((uint64_t)(cfInfo.au8Name[6]) << 48) |
                               ((uint64_t)(cfInfo.au8Name[7]) << 56);
            setX64("CF-A", "preferredVT", u64Name);

            iso_u8 u8BootTime = (iso_u8)IsoGetVTStatusInfo ( VT_BOOTTIME );
            setU8("CF-A", "bootTimeVT", u8BootTime);
         }
      }
      break;
   case IsoEvMaskReadyToStore:
       iso_DebugPrint("cf(%04X), IsoEvMaskReadyToStore(%d)\n", vtHandle, psEvData->eEvent);
      /* pool upload finished - here we can change objects values which should be stored */
      VTC_SetObjValuesBeforeStore();
      break;
   case IsoEvMaskActivated:
       iso_DebugPrint("cf(%04X), IsoEvMaskActivated(%d)\n", vtHandle, psEvData->eEvent);
       /* pool is ready - here we can setup the initial mask and data which should be displayed */
       //iso_DebugPrint("IsoEvMaskActivated: %x\n", vt.transferLanguage);
       updateTick = iso_BaseGetTimeMs();
       vtcPoolLoadHandler(&m_primaryVt);
      break;
   case IsoEvMaskTick:  // Cyclic event; Called only after successful login
       if (m_primaryVt.m_retryPoolLoad != ISO_FALSE)
       {
           iso_DebugPrint("cf(%04X), IsoEvMaskTick -- pool load handler(%d)\n", vtHandle, psEvData->eEvent);
           vtcPoolLoadHandler(&m_primaryVt);
       }

       if (m_primaryVt.m_activeLanguage == lcBase)
       {
           iso_DebugPrint("cf(%04X), IsoEvMaskTick -- update progress(%d)\n", vtHandle, psEvData->eEvent);
           iso_u32 tick = iso_BaseGetTimeMs();
           iso_u32 deltaTick = tick - updateTick;
           if (deltaTick > 1000)
           {
               updateTick = tick;
               IsoCmd_NumericValueRef(OutputNumber_12000, tick);
           }
       }

      AppVTClientDoProcess();   // Sending of commands etc. for mask instance
      break;
   case IsoEvMaskLoginAborted:
       iso_DebugPrint("cf(%04X), IsoEvMaskLoginAborted(%d)\n", vtHandle, psEvData->eEvent);
      // Login failed - application has to decide if login shall be repeated and how often
      //AppVTClientLogin(s16_CfHndVtClient);
      break;
   case IsoEvConnSafeState:
       iso_DebugPrint("cf(%04X), IsoEvConnSafeState(%d)\n", vtHandle, psEvData->eEvent);
       // invalidate pool information
        vtcPoolClear(&m_primaryVt);
        vtcPoolClear(&m_auxVt);
      // Connection closed ( VT lost, VT_LOGOUT (delete object pool response was received ) )
      break;
   case IsoEvAuxServerVersAvailable:
       iso_DebugPrint("cf(%04X), IsoEvAuxServerVersAvailable(%d)\n", vtHandle, psEvData->eEvent);
      break;
   case IsoEvAuxLanguageCmd:
       iso_DebugPrint("cf(%04X), IsoEvAuxLanguageCmd(%d)\n", vtHandle, psEvData->eEvent);
       if (m_auxVt.initialized)
       {
           IsoReadWorkingSetLanguageData(s16_CfHndVtClient, abLCData);
           m_auxVt.m_vtLanguage = vtcPoolGetLanguageCode(abLCData);
       }
      //IsoClServ_ReadLCOfServer( , );
      break;
   case IsoEvAuxTechDataV4Request:
       iso_DebugPrint("cf(%04X), IsoEvAuxTechDataV4Request(%d)\n", vtHandle, psEvData->eEvent);
      break;
   case IsoEvAuxLoadObjects:
       iso_DebugPrint("cf(%04X), IsoEvAuxLoadObjects(%d)\n", vtHandle, psEvData->eEvent);
       AppPoolSettings(ISO_TRUE, &m_auxVt);
      break;
   case IsoEvAuxActivated:
       iso_DebugPrint("cf(%04X), IsoEvAuxActivated(%d)\n", vtHandle, psEvData->eEvent);
       updateTick = iso_BaseGetTimeMs();
       vtcPoolLoadHandler(&m_auxVt);
      break;
   case IsoEvAuxTick:
       if (m_auxVt.m_retryPoolLoad != ISO_FALSE)
       {
           iso_DebugPrint("cf(%04X), IsoEvAuxTick -- pool load handler(%d)\n", vtHandle, psEvData->eEvent);
           vtcPoolLoadHandler(&m_auxVt);
       }
      break;
   case IsoEvAuxLoginAborted:
       iso_DebugPrint("cf(%04X), IsoEvAuxLoginAborted(%d)\n", vtHandle, psEvData->eEvent);
      // Login failed - application has to decide if login shall be repeated and how often
      break;

   case IsoEvAuxStateChanged:
       //iso_DebugPrint("cf(%04X), IsoEvAuxStateChanged(%d, %d)\n", vtHandle, psEvData->eEvent, IsoGetVTStatusInfo(VT_STATEOFANNOUNCING));
       break;
   case IsoEvAuxPoolReloadFinished:
       iso_DebugPrint("cf(%04X), language(%04X), IsoEvAuxPoolReloadFinished(%d)\n", vtHandle, m_auxVt.m_transferLanguage, psEvData->eEvent);
       //iso_DebugPrint("IsoEvAuxPoolReloadFinished: %x\n", vt.transferLanguage);
       break;

    case IsoEvMaskPoolReloadFinished:
        iso_DebugPrint("cf(%04X), IsoEvMaskPoolReloadFinished(%d)\n", vtHandle, psEvData->eEvent);
        //iso_DebugPrint("IsoEvMaskPoolReloadFinished: %x\n", vt.transferLanguage);
        if (m_primaryVt.initialized == ISO_FALSE)
        {
            AppPoolSettings(ISO_FALSE, &m_primaryVt);
        }
        else
        {
            if (m_primaryVt.m_activeLanguage != lcUndefined)
            {
                vtcPoolLoadHandler(&m_primaryVt);
            }
            else
            {
                iso_DebugPrint("IsoEvMaskPoolReloadFinished???\n");
            }
        }
        break;

    case IsoEvMaskStateChanged:
        //iso_DebugPrint("IsoEvMaskStateChanged: %d\n", IsoGetVTStatusInfo(VT_STATEOFANNOUNCING));
        break;

   default: 
       iso_DebugPrint("cf(%04X), event(%d)\n", vtHandle, psEvData->eEvent);
       break;
   }
}

/* ************************************************************************ */
static void AppVTClientDoProcess( void )
{  /* Cyclic VTClient function */

}

/* ************************************************************************ */
static void AppPoolSettings( iso_bool auxVT, struct VTCPool* vtcPool )
{
    char actPoolLabel[LENVERSIONSTR + 1];  // 7 characters filled with blanks
    iso_u8 au8VersionStrings[16][LENVERSIONSTR] = {0};
    iso_u8 poolCount = IsoVTVersionStringGet(au8VersionStrings);
    iso_u32 u32PoolSize = 0U;
    iso_u16 u16NumberObjects = 0U;
    iso_u8 abLanguageCmd[6];
    IsoReadWorkingSetLanguageData(IsoGetVTStatusInfo(CF_HND), abLanguageCmd);
    enum VTCLanguageCode vtLanguage = vtcPoolGetLanguageCode(abLanguageCmd);
    vtcPoolInit(vtcPool, auxVT, vtLanguage, au8VersionStrings, poolCount);

    iso_u8* poolData = NULL;
    size_t poolSize = 0U;
    if (auxVT)
    {
        vtcPoolGetPool(lcA3, &poolData, &u32PoolSize, &u16NumberObjects);
        vtcPoolGetPoolLabel(lcA3, actPoolLabel);
    }
    else
    {
        vtcPoolGetPool(lcBase, &poolData, &u32PoolSize, &u16NumberObjects);
        vtcPoolGetPoolLabel(vtcPool->m_transferLanguage, actPoolLabel);
    }

    iso_DebugPrint("pool %s\n", actPoolLabel);

    (void)IsoPoolInit((iso_u8*)actPoolLabel, poolData, 0,       // Version, PoolAddress, ( PoolSize not needed ) 
        u16NumberObjects, colour_256,      // Number of objects, Graphic typ, 
        ISO_DESIGNATOR_WIDTH, ISO_DESIGNATOR_HEIGHT, ISO_MASK_SIZE);                   // SKM width and height, DM res.

   // Set pool manipulations
   vtcPoolSetPoolManipulation();
}

/* ************************************************************************ */
/* This function is called in case of every page change - you can do e. g. initialisations ...  */
static void CbVtStatus(const ISOVT_STATUS_DATA_T* psStatusData)
{
   switch (psStatusData->wPage)
   {
   case DataMask_1001 /*DM_PAGE1*/:
      //IsoDeleteVersion((iso_u8*)"WHEPS18");    // Avoid deleting pool after each test
      break;
   case DataMask_1002 /*DM_PAGE2*/:
      VTC_setPage2();         // Sending strings ...
      break;
   case DataMask_1003 /*DM_PAGE3*/:
      //IsoCmd_NumericValue( 27010, 20820 ); // Show copied picture
      break;
   default:
       break;
   }
}


/* ************************************************************************ */
/*!                                                                               
   \brief       Receiving all messages of VT                                      
   \verbatim                                                                                 
    Callback function for responses, VT activation messages ...                                                                                            
      
    VT-Function:                Parameter of       Meaning:
                                ISOVT_MSG_STA_T:      
                                                   
    softkey_activation            wObjectID       key object ID                   
                                  wPara1          parent object ID                
                                  bPara           key number (hard coded)         
                                  lValue          activation code (0, 1, 2, 3(Version 4)) see [1]   
    button_activation             wObjectID       button object ID                
                                  wPara1          parent object ID                
                                  bPara           button key number               
                                  lValue          activation code (0, 1, 2, 3(Version 4)) see [1]
    pointing_event                wPara1          X position in pixel             
                                  wPara2          Y position in pixel             
    VT_select_input_object        wObjectID       input object ID                 
                                  wPara1          Selected/Deselected
                                  wPara2          Bitmask (Version 5 and later)
    VT_esc                        wObjectID       ID of input field where aborted 
                                  iErrorCode      error code see ISO Doku.        
    VT_change_numeric_value       wObjectID       ID of input object              
                                  lValue          new value                       
    VT_change_active_mask         wObjectID       momentan active mask            
                                  iErrorCode      error code see ISO Doku.        
    VT_change_softkey_mask        wObjectID       data or alarm mask object ID    
                                  wPara1          soft key mask object ID         
                                  iErrorCode      error code see ISO Doku         
    VT_change_string_value        wObjectID       ID of String object             
                                  bPara           Length of string                
                                  pabVtData       Pointer to characters 
    ( Version 4 )                 
    VT_onUserLayout_hideShow      wObjectID       Object ID of WM, DM, SKM, KG
                                  wPara2          Hide/show
                                  wPara1          Object ID of second WM, DM, SKM, KG
                                  bPara           Hide/show of second
    get_attribute_value           wObjectID       Object ID
                                  bPara           AID
                                  wPara1          Current value of attribute
                                  iErrorCode      ErrorCode (see F.59)
    ( Version 3 )                 
    preferred_assignment          wObjectID       Auxiliary function object if faulty
                                  iErrorCode      Error code see ISO Doku.
    auxiliary_assign_type_1, .._2 wObjectID       Object ID auxiliary function    
                                  wPara1          Object ID auxiliary input (or input number type 1) 
                                                  0xFFFF for unassign             
                                  wPara2          Type of auxiliary incl. Attribute bits see [2]
                                  bPara           ISO_TRUE: Store as pref. assign, else not (only type 2) 
                                  lValue          Bit 16 - 27: Manufacturer code,         
                                                  Bit  0 - 15 Model Identification code of auxiliary input 
                                                  (only type 2)
                                  pabVtData       only for auxiliary_assign_type_2:
                                                  Pointer to the last/current aux unit ISONAME or 8*0xFF
    auxiliary_input_status_type_2
    aux_input_status_type_1       wObjectID       Object ID Auxiliary function type          
                                  wPara1          Input object ID (type 1 = input number     
                                  lValue          Value 1                                    
                                  wPara2          Value 2                                    
                                  iErrorCode      E_NO_ERR, E_CANMSG_MISSED (Alive of input)
    ( Version 5 )
    auxiliary_capabilities        bPara           Number of auxiliary Units
                                  pabVtData       Pointer to data ( Byte 3 ... )

               [1] Timeout control of softkeys and buttons have to be done of application !
               [2] Attribute bits are given to application as additional information
                   For getting the Auxiliary type application have to mask out it.
   \endverbatim
                                                                                  
   \param[in]     \wpp{pIsoMsgSta, const #ISOVT_MSG_STA_T *}                                                   
                       Pointer on received IS0 messages                                                                                
*/           
static void CbVtMessages( const ISOVT_MSG_STA_T * pIsoMsgSta )
{
    OutputVtMessages(pIsoMsgSta, IsoClientsGetTimeMs());

   switch ( pIsoMsgSta->iVtFunction )
   {
   case softkey_activation :
      VTC_handleSoftkeys(pIsoMsgSta);
      break;
   case VT_change_numeric_value :
      break;
   case VT_change_string_value :
      // Receiving string see Page 3
      //VTC_process_VT_change_string_value(pIsoMsgSta);
      break;
   case auxiliary_assign_type_1 :
       break;
   case auxiliary_assign_type_2 :
#if defined(_LAY6_) && defined(ISO_VTC_GRAPHIC_AUX)
   {  /* Forward assignment message to aux unit */
       iso_s16 s16CfHandle;
       s16CfHandle = IsoGetVTStatusInfo(CF_HND); /* or use the stored (s16CfHandleWs) */
       (void)vtcGAux_ForwardAssignment(s16CfHandle, pIsoMsgSta);
   }
#endif // defined(_LAY6_) && defined(ISO_VTC_GRAPHIC_AUX)
   {
       VT_AUXAPP_T auxEntry = { 0 };
       auxEntry.wObjID_Fun = pIsoMsgSta->wObjectID;
       auxEntry.wObjID_Input = pIsoMsgSta->wPara1;
       auxEntry.eAuxType = (VTAUXTYP_e)(pIsoMsgSta->wPara2 & 0x1F);
       auxEntry.wManuCode = (iso_u16)(pIsoMsgSta->lValue >> 16);
       auxEntry.wModelIdentCode = (iso_u16)(pIsoMsgSta->lValue);
       auxEntry.qPrefAssign = pIsoMsgSta->bPara;
       auxEntry.bFuncAttribute = (iso_u8)(pIsoMsgSta->wPara2);
       iso_ByteCpy(auxEntry.baAuxName, &pIsoMsgSta->pabVtData[0], 8);
       updateAuxAssignment("CF-A-AuxAssignment", &auxEntry);
#if(0)
       /* Assignment is stored only in case of Byte 10, Bit 7 is zero (use as preferred assignment) */
       if (pIsoMsgSta->bPara != 0)
       {
           static iso_s16  iNumberOfFunctions = 0;
           static VT_AUXAPP_T asAuxAss[20];      // AUXINPUTMAX !
           /* Reading the complete actual assignment and storing this in a file or EE */
           IsoAuxAssignmentRead(asAuxAss, &iNumberOfFunctions);
           setAuxAssignment("CF-A-AuxAssignment", asAuxAss, iNumberOfFunctions);
           //IsoAuxWriteAssignToFile(asAuxAss, iNumberOfFunctions);  // Assignment -> File
       }

       /* Assignment is stored only in case of Byte 10, Bit 7 is zero (use as preferred assignment) */
       if (pIsoMsgSta->bPara != 0)
       {
           static iso_s16  iNumberOfFunctions = 0;
           static VT_AUXAPP_T asAuxAss[20];      // AUXINPUTMAX !
           /* Reading the complete actual assignment and storing this in a file or EE */
           IsoAuxAssignmentRead(asAuxAss, &iNumberOfFunctions);
           setAuxAssignment("CF-A-AuxAssignment", asAuxAss, iNumberOfFunctions);
           //IsoAuxWriteAssignToFile(asAuxAss, iNumberOfFunctions);  // Assignment -> File
       }
#endif
   }
       break;
   case auxiliary_input_status_type_2 :
      // Here the application gets all Auxfunctions events 
       break;
   default:
       break;
   }
}

static void VTC_SetObjValuesBeforeStore(void)
{
   iso_u8 au8L1[] = "WHEPS                 ";
   iso_u8 au8L2[] = "p.wegscheider@wheps.de";
   iso_u8 au8L3[] = "e.hammerl@wheps.de    ";

   IsoCmd_StringRef(StringVariable_22000 /*OUTSTR_L1*/, au8L1);
   IsoCmd_StringRef(StringVariable_22001 /*OUTSTR_L2*/, au8L2);
   IsoCmd_StringRef(StringVariable_22002 /*OUTSTR_L3*/, au8L3);
}

/* ************************************************************************ */
// Delete stored pool
iso_s16 VTC_PoolDeleteVersion(void)
{  
   iso_s16 s16Ret = E_NO_INSTANCE;
   // If called outside of a callback function, we must set the VT client instance before calling any other API function 
   if (IsoWsSetMaskInst(s16_CfHndVtClient) == E_NO_ERR)
   {
      iso_u16 u16WSVersion, u16VTVersion;
      // ISO version string (C-string with termination or 32 bytes; if VT < 5: only 7 Bytes used )
      iso_u8 au8VersionString[] = "       "; // We use spaces to delete the currently loaded pool from flash
      u16WSVersion = IsoGetVTStatusInfo(WS_VERSION_NR);
      u16VTVersion = IsoGetVTStatusInfo(VT_VERSIONNR);
      if ((u16WSVersion >= VT_V5_SE_UT3) && (u16VTVersion >= VT_V5_SE_UT3))
      {  // 32 byte version label -> use IsoExtendedDeleteVersion()
         #ifdef ISO_VTC_UT3 /* only if compiled for UT3 */
         s16Ret = IsoExtendedDeleteVersion(au8VersionString);
         #endif /* ISO_VTC_UT3 */
      }
      else
      {  // 7 byte version label -> use IsoDeleteVersion()
         s16Ret = IsoDeleteVersion(au8VersionString);
      }
   }

   return s16Ret;
}

/* ************************************************************************ */
// Reload of objects during "application running"
iso_s16 VTC_PoolReload(void)
{
   iso_s16 iRet = E_NO_ERR;
   /* pointer to the pool data */
   static iso_u8*  pu8PoolData = 0;
   static iso_u32  u32PoolSize = 0UL;
   static iso_u16  u16NumberObjects;
   // If called outside of a callback function, we must set the VT client instance before calling any other API function 
   if (IsoWsSetMaskInst(s16_CfHndVtClient) == E_NO_ERR)
   {
      u32PoolSize = LoadPoolFromFile("pools/pool.iop", &pu8PoolData);
      u16NumberObjects = IsoGetNumofPoolObjs(pu8PoolData, (iso_s32)u32PoolSize);
      if (IsoPoolReload(pu8PoolData, u16NumberObjects))
      {
         iso_u16 wSKM_Scal = 0u;
         /* Reload ranges */
         IsoPoolSetIDRangeMode(0, 1099, 0, NotLoad);
         IsoPoolSetIDRangeMode(1100, 1100, 1002, LoadMoveID);  /* 1002 = target(start)ID */
         IsoPoolSetIDRangeMode(1101, 39999, 0, NotLoad);
         IsoPoolSetIDRangeMode(42001, 65334, 0, NotLoad);
         /* Manipulating these objects */
         wSKM_Scal = (iso_u16)IsoPoolReadInfo(PoolSoftKeyMaskScalFaktor);
         IsoPoolSetIDRangeMode(40012, 40012, wSKM_Scal, Centering);  /* Auxiliary function */
      }
      else
      {
         iRet = E_ERROR_INDI;
      }
   }
   return iRet;
}

/* ************************************************************************ */
// Callback function for setting the preferred assignment
static void CbAuxPrefAssignment(VT_AUXAPP_T asAuxAss[], iso_s16* ps16MaxNumberOfAssigns, ISO_USER_PARAM_T userParam)
{
   iso_s16 s16I, s16NumbOfPrefAssigns = 0;
   VT_AUXAPP_T asPrefAss[20];

   /* Reading stored preferred assignment */
//   s16NumbOfPrefAssigns = IsoAuxReadAssignOfFile(asPrefAss);
   s16NumbOfPrefAssigns = getAuxAssignment("CF-A-AuxAssignment", asPrefAss);

   for (s16I = 0; s16I < s16NumbOfPrefAssigns; s16I++)
   {
      asAuxAss[s16I] = asPrefAss[s16I];
   }
   *ps16MaxNumberOfAssigns = s16NumbOfPrefAssigns;
   (void)userParam;
}


/* ************************************************************************ */
// Multiple VT
iso_s16 VTC_NextVTButtonPressed( )
{
   #define VT_LIST_MAX   5       /* Array size for VT cf handle entries */

   if (IsoWsSetMaskInst(s16_CfHndVtClient) == E_NO_ERR)
   {
      iso_s16 s16NumberOfVTs = 0, s16NumberAct = VT_LIST_MAX, iI = 0;
      iso_s16 s16HndCurrentVT;
      iso_s16 as16HandList[VT_LIST_MAX];

      s16HndCurrentVT = (iso_s16)IsoGetVTStatusInfo(VT_HND);   /* get CF handle of actual VT */
      /* Determine list number of actual VT */
      (void)IsoClientsReadListofExtHandles(virtual_terminal, VT_LIST_MAX, as16HandList, &s16NumberOfVTs);

      for (iI = 0; iI < s16NumberOfVTs; iI++)
      {
         if (s16HndCurrentVT == as16HandList[iI])
         {
            s16NumberAct = iI;
            break;
         }
      }

      /* Select next VT in list */ 
      if ((s16NumberOfVTs > 1) && (s16NumberAct != VT_LIST_MAX))
      {
         ISO_CF_INFO_T sUserVT;
         s16NumberAct++;
         if (s16NumberAct >= s16NumberOfVTs)
         {  /* select first VT in list*/
            s16NumberAct = 0;
         }
         iso_NmGetCfInfo(as16HandList[s16NumberAct], &sUserVT);
         IsoVTMultipleNextVT(&sUserVT.au8Name);
         // NAME could be stored here in EEPROM
         // Application must go into safe state !!!
      }
   }
   return 0;
}


void VTC_setNewVT(void)
{
    // ------------------------------------------------------------------------------




    // ------------------------------------------------------------------------------
}

void VTC_handleSoftkeys(const ISOVT_MSG_STA_T * pIsoMsgSta)
{
    switch (pIsoMsgSta->wObjectID)
    {
    case SoftKey_5100 /*KEY_NEXTPAGE*/:   // soft key F5 (book)
        if (pIsoMsgSta->lValue == 1)
        {
            if (IsoGetVTStatusInfo(ID_VISIBLE_DATA_MASK) == DataMask_1001 /*DM_PAGE1*/)
            {
                IsoCmd_ActiveMask(WorkingSet_0 /*WS_OBJECT*/, DataMask_1002 /*DM_PAGE2*/);  // next page with parent ID
            }
            else if (IsoGetVTStatusInfo(ID_VISIBLE_DATA_MASK) == DataMask_1002 /*DM_PAGE2*/)
            {
                IsoCmd_ActiveMask(WorkingSet_0 /*WS_OBJECT*/, DataMask_1003 /*DM_PAGE3*/);  // next page with parent ID
            }
            else { /* unused */ }
        }
        break;
        // ------------------------------------------------------------------------------
    case SoftKey_5101 /*KEY_RESETCNT*/:    // soft key F2

        break;
    case SoftKey_5102 /*KEY_CNT*/:    // soft key F3

        break;
    case SoftKey_5104 /*KEY_F1*/:    // soft key F1

        break;
        // ------------------------------------------------------------------------------
    default:
        break;
    }
}

void VTC_setPage2(void)
{

}

/* ************************************************************************ */
#endif /* _LAY6_ */
/* ************************************************************************ */
