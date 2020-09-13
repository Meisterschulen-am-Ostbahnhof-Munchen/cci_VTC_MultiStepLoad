/* ************************************************************************ */
/*! 
   \file
   \brief      Hardware simulation ( Windows PC CAN BUS implementation )
   \author     Wegscheider Peter
   \date       Created XX.02.15
   \copyright  Wegscheider Hammerl Ingenieure Partnerschaft
   \par        History:
   \par
   02.02.2015 - author P. Wegscheider
   - created
   \par
   22.08.2016 - author P. Wegscheider
   - modified
   \par
   01.12.2017 - author P. Wegscheider
   - Bug ID 8120:  Fixed declaration of HW_CanMsgPrint() and updated calls
*/
/* ************************************************************************ */

#include <stdio.h>
#include "AppHW.h"
#include "driver/can.h"
#include "esp_err.h"
#include "esp_log.h"

#define USE_APP_OUTPUT
#if defined(USE_APP_OUTPUT)
   #include "AppOutput.h"
#endif /* defined(USE_APP_OUTPUT) */

/* ************************************************************************ */

static void HW_CanMsgPrint(uint8_t canNode_u8, can_message_t* can_msg_ps, uint8_t isRX);


#define CANBUS_TAG              "CAN Master"
#define TX_GPIO_NUM             21
#define RX_GPIO_NUM             22

static const can_timing_config_t t_config = CAN_TIMING_CONFIG_250KBITS();
static const can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
//static const can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_GPIO_NUM, (gpio_num_t)RX_GPIO_NUM, CAN_MODE_NORMAL);
static const can_general_config_t g_config = {.mode = CAN_MODE_NORMAL,
                                              .tx_io = (gpio_num_t)TX_GPIO_NUM, .rx_io = (gpio_num_t)RX_GPIO_NUM,
                                              .clkout_io = (gpio_num_t)CAN_IO_UNUSED, .bus_off_io = (gpio_num_t)CAN_IO_UNUSED,
                                              .tx_queue_len = 150, .rx_queue_len = 120,
                                              .alerts_enabled = CAN_ALERT_NONE,
                                              .clkout_divider = 0};


/* ################### CAN Functions ################ */

#if !defined(CCI_CAN_API)  // the implementation is not required if CAN is out sourced into a DLL
void hw_CanInit(uint8_t maxCanNodes_u8)
{
    //Install CAN driver
    ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(CANBUS_TAG, "Driver installed");

    ESP_ERROR_CHECK(can_start());
    ESP_LOGI(CANBUS_TAG, "Driver started");
}

void hw_CanClose(void)
{
    //Uninstall CAN driver
    ESP_ERROR_CHECK(can_stop());
    ESP_LOGI(CANBUS_TAG, "Driver stopped");

    //Uninstall CAN driver
    ESP_ERROR_CHECK(can_driver_uninstall());
    ESP_LOGI(CANBUS_TAG, "Driver uninstalled");
}


int16_t hw_CanSendMsg(uint8_t canNode_u8, uint32_t canId_u32, const uint8_t canData_au8[], uint8_t canDataLength_u8)
{
   can_message_t can_msg_send;
   int16_t  ret_16 = 0;
   int iLoop = 0;

   can_msg_send.identifier = canId_u32;
   can_msg_send.data_length_code = canDataLength_u8;
   can_msg_send.flags = CAN_MSG_FLAG_EXTD; /* extended */

   for (iLoop = 0; (iLoop < 8) && (iLoop < canDataLength_u8); iLoop++)
      can_msg_send.data[iLoop] = canData_au8[iLoop];

   

   
   if (can_transmit(&can_msg_send, pdMS_TO_TICKS(1)) == ESP_OK)
   {
       HW_CanMsgPrint(canNode_u8, &can_msg_send, 0u);
   }
   else
   {
      ret_16 = -6; /* E_OVERFLOW */
      hw_DebugPrint("Tx error: %x %x \n", can_msg_send.identifier, can_msg_send.data[0]);
   }
   return ret_16;
}

int16_t hw_CanReadMsg(uint8_t canNode_u8, uint32_t *canId_pu32, uint8_t canData_pau8[], uint8_t *canDataLength_pu8)
{
   can_message_t can_msg_read;



   if (can_receive(&can_msg_read, pdMS_TO_TICKS(1)) == ESP_OK)
   {
      if (can_msg_read.identifier != 0xCCCCCCCCuL)
      {
         HW_CanMsgPrint(canNode_u8, &can_msg_read, 1u);
         *canId_pu32 = can_msg_read.identifier;
         *canDataLength_pu8 = can_msg_read.data_length_code;
         for (uint8_t i_u8 = 0u; i_u8 < can_msg_read.data_length_code; i_u8++)
         {
            canData_pau8[i_u8] = can_msg_read.data[i_u8];
         }
         return 1;
      }
   }
   return 0;
}

int16_t  hw_CanGetFreeSendMsgBufferSize(uint8_t canNode_u8)
{  /* we return always 20 free buffer entries.... */
   return 20;
}

static void HW_CanMsgPrint(uint8_t canNode_u8, can_message_t* can_msg_ps, uint8_t isRX)
{
   return; //disable CANPrint.
   const char_t *pcMsgTxt;
   const char_t *pcRxTx;
   /* printf hw_DebugPrint hw_DebugTrace */
   #define CAN_PRINT hw_DebugTrace

   pcRxTx = (isRX > 0u) ? "Rx" : "Tx";
   CAN_PRINT("%2u %2s %8x %1u ", canNode_u8, pcRxTx, can_msg_ps->identifier, can_msg_ps->data_length_code);

#if defined(USE_APP_OUTPUT)
   {  // Get PGN and extra text if available
      uint32_t u32PGN;
      u32PGN = (can_msg_ps->identifier & 0x03FFFF00uL) >> 8u;
      if ((u32PGN & 0x00FF00uL) < PGN_PDU2_240_X)
      {  /* PDU 1 -> remove DA */
         u32PGN &= 0x03FF00uL;
      }

      switch (u32PGN)
      {
         case PGN_VTtoECU        : 
         case PGN_ECUtoVT        : pcMsgTxt = VTSublistTextout(can_msg_ps->data[0] ); break;
         //case PGN_SCC_TO_SCM     : 
         //case PGN_SCM_TO_SCC     : pcMsgTxt = SCSublistTextout(can_msg_ps->Data[0] ); break;
         case PGN_PROCESS_DATA   : pcMsgTxt = TCSublistTextout(can_msg_ps->data[0] ); break;
         case PGN_TP_DT          : pcMsgTxt = TPSublistTextout(can_msg_ps->data[0], can_msg_ps->data[1] ); break;
         //case PGN_FSC_TO_FS      : pcMsgTxt = FSCSublistTextout( PGN_FSC_TO_FS, can_msg_ps->Data );   break;
         //case PGN_FS_TO_FSC      : pcMsgTxt = FSCSublistTextout( PGN_FS_TO_FSC, can_msg_ps->Data );   break;
         case PGN_ADDRESS_CLAIMED: pcMsgTxt = ACLSublistTextout(can_msg_ps->identifier, &(can_msg_ps->data) ); break;
         case PGN_TP_CM          :
         case PGN_ETP_CM         : pcMsgTxt = TPCMSublistTextOut(can_msg_ps->identifier, can_msg_ps->data); break;
         case PGN_N_ACK          : pcMsgTxt = ACKSublistTextOut(can_msg_ps->identifier, can_msg_ps->data); break;
         case PGN_WORKING_SET_MEMBER: pcMsgTxt = "Working set member ";   break;
         case PGN_WORKING_SET_MASTER: pcMsgTxt = "Working set master ";   break;
         case PGN_LANGUAGE_COMMAND  : pcMsgTxt = "Language command ";     break;
         case PGN_ACTIVE_DIAG_TROUBLE_CODES: pcMsgTxt = "DM1 ";      break;
         default                    : pcMsgTxt = " ";                break;
      }

   }
#else 
   pcMsgTxt = " ";
#endif /* defined(USE_APP_OUTPUT) */ 

   if (can_msg_ps->data_length_code == 0)
   {
      CAN_PRINT("  %s\n", pcMsgTxt);
   }
   if (can_msg_ps->data_length_code == 3)
   {
      CAN_PRINT("%2.2x %2.2x %2.2x  %s\n", can_msg_ps->data[0], can_msg_ps->data[1], can_msg_ps->data[2], pcMsgTxt);
   }
   else
   {
      CAN_PRINT("%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x  %s\n",
         can_msg_ps->data[0], can_msg_ps->data[1], can_msg_ps->data[2], can_msg_ps->data[3],
         can_msg_ps->data[4], can_msg_ps->data[5], can_msg_ps->data[6], can_msg_ps->data[7], pcMsgTxt);
   }
}
#endif // !defined(CCI_CAN_API)
