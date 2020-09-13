/* ************************************************************************ */
/*!
   \file           
   \brief      Basic layer demo implementation (Imp1)
  
   \author     Erwin Hammerl
   \date       Created xx.01.2014 
   \copyright  Wegscheider Hammerl Ingenieure Partnerschaft
   \par
*/
/* **************************  includes ********************************** */

#include <string.h>

#include "IsoDef.h"
#include "App_VTClient.h"
//#include "App_TCClient.h"

#include "AppCommon/AppOutput.h"
#include "Settings/settings.h"

#define SA_PREFERRED     0x8Cu      // Preferred source address of CF

#define MINIMUM_CF         0u

/* ****************************** local data   *************************** */
// Miscellaneous
static iso_bool q_Ignition = ISO_FALSE;

// Network member handles 
static iso_s16 s16NmHandImp1      = HANDLE_UNVALID;

static iso_s16 s16NmAlDestTest    = HANDLE_UNVALID;


// PGN handles 
static iso_s16 s16HaAlWhSpeedDis = HANDLE_UNVALID;

/* ****************************** function prototypes ******************** */
static void  AppImp_Reset( void );
static void  CbIsoNetwork( const ISONETEVENT_T* psNetEv );
static void  CbIsoDataLink(ISO_TPREP_E eDataTransStatus, const ISO_TPINFO_T* psTpInfo);
#if defined(ISO_MODULE_CLIENTS)
static void  CbCfClientEvents(const ISOCFEVENT_T* psCfData);       // optional only with _LAY6_ / _LAY10_
#endif /* defined(ISO_MODULE_CLIENTS) */
static void  AppInstallPGNsforTECU(ISO_CF_INFO_T * sUserInfo);

static void  CbPGNReceiveWheelbasedSpeed(const PGNDAT_T* psData);

static void  AppImpl_AL2(void);
static void  App_SetDTCforAddressViolation(iso_u8 u8SA);

iso_bool AppImpl_Diag(ISO_TPREP_E eTpRep, const ISO_TPINFO_T* psMsgInfo);

/* Following diagnostic data are normally the same for all CFs */
static iso_u8 au8ECUIdent[] = "WHEPS TESTIMPLEMENT*0815*PC*VIVOBOOK*WHEPS*423*";
static iso_u8 au8SWIdent[] = "xApp 11.0#ISOBUSDriver 11.00.00*";
static iso_u8 au8ProdIden[] = "ABC1234567890*Brand A*2020i*";
static iso_u8 au8ComCert[8] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };                       // Byte 3 and 4 has to be 0 !
static iso_u8 au8DiagPro[] = { 0u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu };  // No SPN defined !

/* CF Functionalities is individual for every CF - No SPNs defined */
static iso_u8 au8CFFuncImp1[] = { 0xFF, 5,  MINIMUM_CF, 1, 0,  2, 3, 0,  4, 1, 1, 5,  6, 2, 1, 5,  8, 1, 0 };

void AppImpl_Ignition( iso_bool qIgnition )
{
   if( qIgnition == ISO_TRUE && q_Ignition == ISO_FALSE )
   {
      AppImp_Reset( );     //Power on event
   }
   if( qIgnition == ISO_FALSE && q_Ignition == ISO_TRUE)
   { 
      //Power off event - Logoff possible but not necessary for implements see sample
   }      
   q_Ignition = qIgnition;
}


void AppImpl_doProcess()
{
   if( q_Ignition )
   {
      // Cyclic e. g. setting of PGNs 
   }
}


// ***************************************************************************************
// Start implement
static void AppImp_Reset( void )
{
   ISO_CF_NAME_T     au8CfName;
   ISO_USER_PARAM_T  userParamCf = ISO_USER_PARAM_DEFAULT;
   iso_s16           s16CfHandle = HANDLE_UNVALID;
   iso_u8            u8SourceAddress;
   // ------------------------------------------------------------------------------
   iso_NmSetName(ISO_TRUE,      /* Selfconfigurable */
      2u,            /* Industry group */
      5u,            /* Device class */
      0u,            /* Device class instance */
      132u,          /* Function */
      getU16("CF-A", "manufacturer", 0x153u),    /* Manufacturer code; CCI = 0x153 */
      getU32("CF-A", "serialNo", 0u),            /* Identity number (Serial number) */
      0u,            /* Function instance */
      0u,            /* ECU instance */
      &au8CfName);   /* NAME - return value */

   u8SourceAddress = getU8("CF-A", "sourceAddress", SA_PREFERRED);

   s16CfHandle = iso_BaseMemberAdd(ISO_CAN_VT,
      u8SourceAddress,
      CAST_TO_CONST_ISONAME_PTR(&au8CfName),
      working_set_master,
      userParamCf,
      CbIsoNetwork,
      CbIsoDataLink
   );

#if defined(CCI_HSI)
/* for high speed ISOBUS test implementation */
   iso_BaseSetHsiActive(s16CfHandle, ISO_TRUE);
#endif /* defined(CCI_HSI) */

   // ------------------------------------------------------------------------------

   if (s16CfHandle != HANDLE_UNVALID)
   {
      #if defined(ISO_MODULE_CLIENTS)
      // Create working set master before IsoVTInit - necessary of VT and TC client 
      IsoSetWorkingSetMaster(s16CfHandle);
      // Start/Lost control of server/implements - must be called after IsoSetWorkingSetMaster() !
      IsoCFControlCbSet(s16CfHandle, userParamCf, CbCfClientEvents);
      #endif /* defined(ISO_MODULE_CLIENTS) */

      #ifdef _LAY6_
         AppVTClientLogin(s16CfHandle);    // Init Virtual terminal client
      #endif
      #ifdef _LAY10_
         //AppTCClientLogin(s16CfHandle);    // Init Task Controller client
      #endif

      //s16NmHandImp1 = s16CfHandle;      // We can store it here or later in the network event CB
   }

}


// ***************************************************************************************
// Callbacks
static void CbIsoNetwork(const ISONETEVENT_T* psNetEv)
{
   switch( psNetEv->eMemberRefer )
   {
     case thisMember:
         switch( psNetEv->eNetEvent )
         {
             case Isonet_MemberActive:
             case Isonet_MemberNewAddress:
                // Called at successful login or new address after address conflict
                s16NmHandImp1 = psNetEv->s16Handle;
                {
                     iso_u8 u8CurSA;
                     u8CurSA = getU8("CF-A", "sourceAddress", SA_PREFERRED);
                     if (u8CurSA != psNetEv->u8SAMember)
                     {  // SA must be stored in nonvolatile memory and used for next power on
                        setU8("CF-A", "sourceAddress", psNetEv->u8SAMember);
                     }
                }
                
                if( psNetEv->eNetEvent == Isonet_MemberActive )
                {  // Initialise PGNs e. g. diagnostic interface
                   AppImpl_AL2();
                }
                break;
             case Isonet_MemberAddressConflict:
             case Isonet_MemberInactive:              // logout
                s16NmHandImp1 = HANDLE_UNVALID;        
                break;
             case Isonet_MemberAddressViolation:  
                // Part 5 - 4.4.4.3: set diagnostic trouble code with SPN=2000 + SA and FMI=31
                App_SetDTCforAddressViolation(psNetEv->u8SAMember);
                break;
             default: break;
         }
         break;
     case network:
         switch( psNetEv->eNetEvent )
         {
             case Isonet_MemberActive:    // Extern CFs are only reported after intern CF is logged in!
                 {
                     ISO_CF_INFO_T sUserInfo;
                     iso_NmGetCfInfo(psNetEv->s16Handle, &sUserInfo);
                     if ( sUserInfo.eIsoUserFunct == tractor_ecu )
                     {
                        AppInstallPGNsforTECU(&sUserInfo);
                     }
                    s16NmAlDestTest = psNetEv->s16Handle;
                 }
                 break;
             case Isonet_MemberNewAddress:
             case Isonet_MemberAddressConflict: break;
             case Isonet_MemberInactive:  break;
             default: break;
         }
         break;
     default: break;
   }
   OutputNetworkEvents( psNetEv, iso_BaseGetTimeMs() );  // see Outputwindow
}


// Optional and only with clients
#if defined(ISO_MODULE_CLIENTS)
static void CbCfClientEvents(const ISOCFEVENT_T* psCfData)
{
   // Advanced network member control based on Cyclic Status messages
   if (psCfData->eCFEvent == IsoCF_Active)                // IsoCF_Lost ...
   {
      if (psCfData->eIsoUserFunct == task_controller)    // virtual_terminal ...
      {
         // Task controller has started sending the TC status message ...
      }
   }
  

}
#endif /* defined(ISO_MODULE_CLIENTS) */

static void CbIsoDataLink( ISO_TPREP_E eDataTransStatus, const ISO_TPINFO_T* psTpInfo )
{
   //OutputDataLink( eDataTransStatus, psTpInfo );
   switch ( eDataTransStatus )
   {
   case record_request :
      /* NACK/ACK of requests need to be handled manual like in this example
       -> following if() must be part of every application */
      if ( psTpInfo->s16HndIntern != HANDLE_GLOBAL )
      {
       switch ( psTpInfo->dwPGN )
       {
         case PGN_DIAGNOSTIC_DATA_CLEAR:
            // DM3: Application has to erase trouble codes here and then respond with 
            iso_DlPgnAcknowledge( ACK, PGN_DIAGNOSTIC_DATA_CLEAR, 0xFFu, 0xFFu,
                                  psTpInfo->s16HndIntern, psTpInfo->s16HndPartner );
            break;
         default:
            if ( psTpInfo->s16HndPartner != HANDLE_UNVALID )
            {
               iso_DlPgnAcknowledge( NACK, psTpInfo->dwPGN, 0xFFu, 0xFFu,
                                     psTpInfo->s16HndIntern, psTpInfo->s16HndPartner );
            }
            break;
        }
      }
      break; 
   case record_finish :
      // Receiving PGNs which are installed and dont use a own callback function 
      break;
   case send_finished :
      break;
   case datatrans_abort :
      // Abort of data transport - possible actions like repeating request, ..  
      break;
   default:
      break;
   }
}




// ***************************************************************************************
// Communication with the TECU
static void AppInstallPGNsforTECU(ISO_CF_INFO_T * sUserInfo)
{
#if defined(_LAY78_)
   ISO_USER_PARAM_T userParamAl = ISO_USER_PARAM_DEFAULT;
   
   s16HaAlWhSpeedDis = iso_AlPgnRxNew( s16NmHandImp1,
                                       PGN_WHEEL_BASED_SPEED,
                                       HANDLE_GLOBAL,
                                       8u, 0, 3, 200, userParamAl, CbPGNReceiveWheelbasedSpeed);
   iso_SpnDefineSpn(s16HaAlWhSpeedDis, SPN_WHEELBASEDMACHINESPEED, 1u, 1u, 16u, SpnValStandard);
   iso_SpnDefineSpn(s16HaAlWhSpeedDis, SPN_WHEELBASEDMACHINEDISTANCE, 3u, 1u, 32u, SpnValStandard);
   iso_AlPgnActivate(s16HaAlWhSpeedDis);
   (void)sUserInfo;
#else /* defined(_LAY78_) */
    (void)sUserInfo;
#endif /* defined(_LAY78_) */
}


// Callback function for Wheel-based speed and distance 
static void CbPGNReceiveWheelbasedSpeed(const PGNDAT_T* psData)
{
#if defined(_LAY78_)
   if (psData->qTimedOut == ISO_FALSE)
   {
#if defined(_LAY10_) /* TC client enabled */
      iso_u32 u32DatVal2 = 0uL;
      iso_SpnDataReadCom(16, 32, psData->pau8Data, &u32DatVal2);
      IsoTC_SetDistance(u32DatVal2);      // Giving distance to TC client
#endif /* defined(_LAY10_) */
   }
   else
   {
      iso_DebugTrace("AL - Time out - Wheel based speed not received \n");
   }
#endif /* defined(_LAY78_) */
}


#define DTC_ARRAYSIZE       30
static iso_u8  au8DM1[DTC_ARRAYSIZE];   /* Array for DTC DM1 message */
static iso_s16 s16DM1NumbOfActDTCs = 0;
static iso_s16 s16HaC1TxDM1 = HANDLE_UNVALID;


/* Setup DM! with application layer */
static void AppImpl_AL2()
{
#if defined(_LAY78_)
   ISO_USER_PARAM_T userParamAl = ISO_USER_PARAM_DEFAULT;

   // DM1 message send 
   s16HaC1TxDM1 = iso_AlPgnTxNew(s16NmHandImp1,
      PGN_ACTIVE_DIAG_TROUBLE_CODES,
      HANDLE_GLOBAL,
      DTC_ARRAYSIZE, au8DM1, 6, 10000u, userParamAl, 0);
   iso_AlPgnTxSetLimitsReqForRepet(s16HaC1TxDM1, 1000, 10000);
   iso_AlPgnTxChangeDataSize(s16HaC1TxDM1, 8u);
   iso_SpnDMResetDTC(au8DM1, DTC_ARRAYSIZE, &s16DM1NumbOfActDTCs);
   iso_AlPgnActivate(s16HaC1TxDM1);
#endif /* defined(_LAY78_) */
}


static void App_SetDTCforAddressViolation(iso_u8 u8SA)
{
#if defined(_LAY78_)
    (void)u8SA;
#else /* defined(_LAY78_) */
    (void)u8SA;
#endif /* defined(_LAY78_) */
}


// callback function for diagnostic request messages
// a sample can be found at <Samples/12_Diagnostic/AppIso_Diag.cpp>
iso_bool AppImpl_Diag(ISO_TPREP_E eTpRep, const ISO_TPINFO_T* psMsgInfo)
{
	iso_bool qHandled = ISO_FALSE;
	iso_u8 * pauData = 0;
	iso_u32  u32StrLen = 8uL;

	iso_DebugPrint("Request of %x to %x for diagnostic PGN received: %x\n",
		psMsgInfo->u8SAPartner, psMsgInfo->u8SAIntern, psMsgInfo->dwPGN);

	switch (psMsgInfo->dwPGN)
	{
	case PGN_ECU_IDENTIFICATION_INFO:
		pauData = au8ECUIdent;
		u32StrLen = (iso_u32)strlen((char*)au8ECUIdent);
		break;
	case PGN_SOFTWARE_IDENTIFICATION:
		pauData = au8SWIdent;
		u32StrLen = (iso_u32)strlen((char*)au8SWIdent);
		break;
	case PGN_PRODUCT_IDENTIFICATION:
		pauData = au8ProdIden;
		u32StrLen = (iso_u32)strlen((char*)au8ProdIden);
		break;
	case PGN_ISOBUS_COMPLIANCE_CERTIFICA:
		pauData = au8ComCert;
		u32StrLen = sizeof(au8ComCert);
		break;
	case PGN_ECU_DIAGNOSTIC_PROTOCOL:
		pauData = au8DiagPro;
		u32StrLen = sizeof(au8DiagPro);
		break;
	case PGN_FUNCTIONALI_CHARACTERISTICS:
		pauData = au8CFFuncImp1;
		u32StrLen = sizeof(au8CFFuncImp1);
		break;
	default:
		break;
	}

	if ((pauData != NULL) && (iso_DlPgnSend(6, psMsgInfo->dwPGN, u32StrLen, pauData, psMsgInfo->s16HndIntern, psMsgInfo->s16HndPartner) == E_NO_ERR))
	{
		qHandled = ISO_TRUE;
	}
	return qHandled;
}

/* ************************************************************************ */
