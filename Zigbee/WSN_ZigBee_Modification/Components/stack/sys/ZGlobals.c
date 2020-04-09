/**************************************************************************************************
  Filename:       ZGlobals.c
  Revised:        $Date: 2007-01-08 12:56:09 -0800 (Mon, 08 Jan 2007) $
  Revision:       $Revision: 13236 $

  Description:    User definable Z-Stack parameters.


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "OSAL_Nv.h"
#include "ZDObject.h"
#include "ZGlobals.h"
#include "ZDNwkMgr.h"

#if defined(ZCL_KEY_ESTABLISH)
#include "zcl_key_establish.h"
#endif

#include "OnBoard.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

typedef struct zgItem
{
  uint16 id;
  uint16 len;
  void *buf;
} zgItem_t;

/*********************************************************************
 * NWK GLOBAL VARIABLES
 */

// Polling values
uint16 zgPollRate = POLL_RATE;
uint16 zgQueuedPollRate = QUEUED_POLL_RATE;
uint16 zgResponsePollRate = RESPONSE_POLL_RATE;
uint16 zgRejoinPollRate = REJOIN_POLL_RATE;

// Transmission retries numbers
uint8 zgMaxDataRetries = NWK_MAX_DATA_RETRIES;
uint8 zgMaxPollFailureRetries = MAX_POLL_FAILURE_RETRIES;

// Default channel list
uint32 zgDefaultChannelList = DEFAULT_CHANLIST;

// Default starting scan duration
uint8 zgDefaultStartingScanDuration = STARTING_SCAN_DURATION;

// Stack profile Id
uint8 zgStackProfile = STACK_PROFILE_ID;

// Default indirect message holding timeout
uint8 zgIndirectMsgTimeout = NWK_INDIRECT_MSG_TIMEOUT;

// Security mode
uint8 zgSecurityMode = ZG_SECURITY_MODE;

// Secure permit join
uint8 zgSecurePermitJoin = true;

#if defined(ZCL_KEY_ESTABLISH)
// Certificate Authority Public Key
uint8 zgCAPublicKey[ZCL_KEY_ESTABLISHMENT_PUBLIC_KEY_LENGTH];

// Local Certificate for AMI CBKE
uint8 zgLocalCertificate[ZCL_KEY_ESTABLISHMENT_CERTIFICATE_LENGTH];

// Static Private Key
uint8 zgStaticPrivateKey[ZCL_KEY_ESTABLSIHMENT_PRIVATE_KEY_LENTGH];

// For test only
uint8 zgRemotePublicKey[ZCL_KEY_ESTABLISHMENT_PUBLIC_KEY_LENGTH];
#endif

// Route expiry
uint8 zgRouteExpiryTime = ROUTE_EXPIRY_TIME;

// Extended PAN Id
uint8 zgExtendedPANID[Z_EXTADDR_LEN];

// Broadcast parameters
uint8 zgMaxBcastRetires   = MAX_BCAST_RETRIES;
uint8 zgPassiveAckTimeout = PASSIVE_ACK_TIMEOUT;
uint8 zgBcastDeliveryTime = BCAST_DELIVERY_TIME;

// Network mode
uint8 zgNwkMode = NWK_MODE;

// Many-to-one values
uint8 zgConcentratorEnable = CONCENTRATOR_ENABLE;
uint8 zgConcentratorDiscoveryTime = CONCENTRATOR_DISCOVERY_TIME;
uint8 zgConcentratorRadius = CONCENTRATOR_RADIUS;
uint8 zgMaxSourceRoute = MAX_SOURCE_ROUTE;
uint8 zgConcentratorRC = CONCENTRATOR_ROUTE_CACHE;   // concentrator with route cache (no memory constraints)

/*********************************************************************
 * APS GLOBAL VARIABLES
 */

// The maximum number of retries allowed after a transmission failure
uint8 zgApscMaxFrameRetries = APSC_MAX_FRAME_RETRIES;

// The maximum number of seconds (milliseconds) to wait for an
// acknowledgement to a transmitted frame.

// This number is used by polled devices.
uint16 zgApscAckWaitDurationPolled = APSC_ACK_WAIT_DURATION_POLLED;

// This number is used by non-polled devices in the following formula:
//   (100 mSec) * (_NIB.MaxDepth * zgApsAckWaitMultiplier)
uint8 zgApsAckWaitMultiplier = 2;

// The maximum number of milliseconds for the end device binding
uint16 zgApsDefaultMaxBindingTime = APS_DEFAULT_MAXBINDING_TIME;

// The 64-big identifier of the network to join or form.
// Default set to all zeros
uint8 zgApsUseExtendedPANID[Z_EXTADDR_LEN] = {00,00,00,00,00,00,00,00};

// A boolean flag that indicates whether it is OK to use insecure join
// on startup. Default set to true
uint8 zgApsUseInsecureJoin = TRUE;

// The size of a tx window when using fragmentation
uint8 zgApscMaxWindowSize = APS_DEFAULT_WINDOW_SIZE;

// The delay between tx packets when using fragmentaition
uint16 zgApsInterframeDelay = APS_DEFAULT_INTERFRAME_DELAY;

// The radius of broadcast multicast transmissions
uint8 zgApsNonMemberRadius = APS_DEFAULT_NONMEMBER_RADIUS;
/*********************************************************************
 * SECURITY GLOBAL VARIABLES
 */

// This is the pre-configured key in use (from NV memory)
uint8 zgPreConfigKey[SEC_KEY_LEN];

// If true, preConfigKey should be configured on all devices on the network
// If false, it is configured only on the coordinator and sent to other
// devices upon joining.
uint8 zgPreConfigKeys = FALSE;// TRUE;

// This is the pre-configured Trust Center Link Key for the AMI profile
uint8 zgPreConfigTCLinkKey[SEC_KEY_LEN];


/*********************************************************************
 * ZDO GLOBAL VARIABLES
 */

// Configured PAN ID
uint16 zgConfigPANID = ZDAPP_CONFIG_PAN_ID;

// Device Logical Type
uint8 zgDeviceLogicalType = DEVICE_LOGICAL_TYPE;

// Startup Delay
uint8 zgStartDelay = START_DELAY;

/*********************************************************************
 * APPLICATION GLOBAL VARIABLES
 */

// Network Manager Mode
uint8 zgNwkMgrMode = ZDNWKMGR_ENABLE;

/*********************************************************************
 * NON-STANDARD GLOBAL VARIABLES
 */

// Simple API Endpoint
uint8 zgSapiEndpoint = SAPI_ENDPOINT;

/*********************************************************************
 * LOCAl VARIABLES
 */

/*********************************************************************
 * ZGlobal Item Table
 */
static CONST zgItem_t zgItemTable[] =
{
#if defined ( NV_INIT )
  {
    ZCD_NV_LOGICAL_TYPE, sizeof(zgDeviceLogicalType), &zgDeviceLogicalType
  },
  {
    ZCD_NV_POLL_RATE, sizeof(zgPollRate), &zgPollRate
  },
  {
   ZCD_NV_QUEUED_POLL_RATE, sizeof(zgQueuedPollRate), &zgQueuedPollRate
  },
  {
    ZCD_NV_RESPONSE_POLL_RATE, sizeof(zgResponsePollRate), &zgResponsePollRate
  },
  {
    ZCD_NV_REJOIN_POLL_RATE, sizeof(zgRejoinPollRate), &zgRejoinPollRate
  },
  {
   ZCD_NV_DATA_RETRIES, sizeof(zgMaxDataRetries), &zgMaxDataRetries
  },
  {
     ZCD_NV_POLL_FAILURE_RETRIES, sizeof(zgMaxPollFailureRetries), &zgMaxPollFailureRetries
  },
  {
    ZCD_NV_CHANLIST, sizeof(zgDefaultChannelList), &zgDefaultChannelList
  },
  {
    ZCD_NV_SCAN_DURATION, sizeof(zgDefaultStartingScanDuration), &zgDefaultStartingScanDuration
  },
  {
    ZCD_NV_STACK_PROFILE, sizeof(zgStackProfile), &zgStackProfile
  },
  {
    ZCD_NV_INDIRECT_MSG_TIMEOUT, sizeof(zgIndirectMsgTimeout), &zgIndirectMsgTimeout
  },
  {
    ZCD_NV_ROUTE_EXPIRY_TIME, sizeof(zgRouteExpiryTime), &zgRouteExpiryTime
  },
  {
    ZCD_NV_EXTENDED_PAN_ID, Z_EXTADDR_LEN, zgExtendedPANID
  },
  {
    ZCD_NV_BCAST_RETRIES, sizeof(zgMaxBcastRetires), &zgMaxBcastRetires
  },
  {
    ZCD_NV_PASSIVE_ACK_TIMEOUT, sizeof(zgPassiveAckTimeout), &zgPassiveAckTimeout
  },
  {
    ZCD_NV_BCAST_DELIVERY_TIME, sizeof(zgBcastDeliveryTime), &zgBcastDeliveryTime
  },
  {
    ZCD_NV_NWK_MODE, sizeof(zgNwkMode), &zgNwkMode
  },
  {
    ZCD_NV_CONCENTRATOR_ENABLE, sizeof(zgConcentratorEnable), &zgConcentratorEnable
  },
  {
    ZCD_NV_CONCENTRATOR_DISCOVERY, sizeof(zgConcentratorDiscoveryTime), &zgConcentratorDiscoveryTime
  },
  {
    ZCD_NV_CONCENTRATOR_RADIUS, sizeof(zgConcentratorRadius), &zgConcentratorRadius
  },
  {
    ZCD_NV_MAX_SOURCE_ROUTE, sizeof(zgMaxSourceRoute), &zgMaxSourceRoute
  },
  {
    ZCD_NV_CONCENTRATOR_RC, sizeof(zgConcentratorRC), &zgConcentratorRC
  },
#ifndef NONWK
  {
    ZCD_NV_PANID, sizeof(zgConfigPANID), &zgConfigPANID
  },
  {
    ZCD_NV_PRECFGKEY, SEC_KEY_LEN, &zgPreConfigKey
  },
  {
    ZCD_NV_SECURE_PRECFG_TCLINKKEY, SEC_KEY_LEN, &zgPreConfigTCLinkKey
  },
  {
    ZCD_NV_PRECFGKEYS_ENABLE, sizeof(zgPreConfigKeys), &zgPreConfigKeys
  },
  {
    ZCD_NV_SECURITY_MODE, sizeof(zgSecurityMode), &zgSecurityMode
  },
  {
    ZCD_NV_SECURE_PERMIT_JOIN, sizeof(zgSecurePermitJoin), &zgSecurePermitJoin
  },
#if defined(ZCL_KEY_ESTABLISH)
  {
    ZCD_NV_CA_PUBLIC_KEY, ZCL_KEY_ESTABLISHMENT_PUBLIC_KEY_LENGTH, zgCAPublicKey
  },
  {
    ZCD_NV_lOCAL_CERTIFICATE, ZCL_KEY_ESTABLISHMENT_CERTIFICATE_LENGTH, zgLocalCertificate
  },
  {
    ZCD_NV_STATIC_PRIVATE_KEY, ZCL_KEY_ESTABLSIHMENT_PRIVATE_KEY_LENTGH, zgStaticPrivateKey
  },
  {
    ZCD_NV_STATIC_PUBLIC_KEY, ZCL_KEY_ESTABLISHMENT_PUBLIC_KEY_LENGTH, zgRemotePublicKey
  },
#endif // CBKE
#endif // NONWK
  {
    ZCD_NV_APS_FRAME_RETRIES, sizeof(zgApscMaxFrameRetries), &zgApscMaxFrameRetries
  },
  {
    ZCD_NV_APS_ACK_WAIT_DURATION, sizeof(zgApscAckWaitDurationPolled), &zgApscAckWaitDurationPolled
  },
  {
    ZCD_NV_APS_ACK_WAIT_MULTIPLIER, sizeof(zgApsAckWaitMultiplier), &zgApsAckWaitMultiplier
  },
  {
    ZCD_NV_BINDING_TIME, sizeof(zgApsDefaultMaxBindingTime), &zgApsDefaultMaxBindingTime
  },
  {
    ZCD_NV_APS_USE_EXT_PANID, Z_EXTADDR_LEN, zgApsUseExtendedPANID
  },
  {
    ZCD_NV_APS_USE_INSECURE_JOIN, sizeof(zgApsUseInsecureJoin), &zgApsUseInsecureJoin
  },
  {
    ZCD_NV_APSF_WINDOW_SIZE, sizeof(zgApscMaxWindowSize), &zgApscMaxWindowSize
  },
  {
    ZCD_NV_APSF_INTERFRAME_DELAY, sizeof(zgApsInterframeDelay), &zgApsInterframeDelay
  },
  {
    ZCD_NV_APS_NONMEMBER_RADIUS, sizeof(zgApsNonMemberRadius), &zgApsNonMemberRadius
  },
  {
    ZCD_NV_START_DELAY, sizeof(zgStartDelay), &zgStartDelay
  },
  {
    ZCD_NV_SAPI_ENDPOINT, sizeof(zgSapiEndpoint), &zgSapiEndpoint
  },
  {
    ZCD_NV_NWK_MGR_MODE, sizeof(zgNwkMgrMode), &zgNwkMgrMode
  },
#endif // NV_INIT
  // Last item -- DO NOT MOVE IT!
  {
    0x00, 0, NULL
  }
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static uint8 zgItemInit( uint16 id, uint16 len, void *buf, uint8 setDefault );


/*********************************************************************
 * @fn       zgItemInit()
 *
 * @brief
 *
 *   Initialize a global item. If the item doesn't exist in NV memory,
 *   write the system default (value passed in) into NV memory. But if
 *   it exists, set the item to the value stored in NV memory.
 *
 *   Also, if setDefault is TRUE and the item exists, we will write
 *   the default value to NV space.
 *
 * @param   id - item id
 * @param   len - item len
 * @param   buf - pointer to the item
 * @param   setDefault - TRUE to set default, not read
 *
 * @return  ZSUCCESS if successful, NV_ITEM_UNINIT if item did not
 *          exist in NV, NV_OPER_FAILED if failure.
 */
static uint8 zgItemInit( uint16 id, uint16 len, void *buf, uint8 setDefault )
{

  uint8 status;

  // If the item doesn't exist in NV memory, create and initialize
  // it with the value passed in.
  status = osal_nv_item_init( id, len, buf );
  if ( status == ZSUCCESS )
  {
    if ( setDefault )
    {
      // Write the default value back to NV
      status =  osal_nv_write( id, 0, len, buf );
    }
    else
    {
      // The item exists in NV memory, read it from NV memory
      status = osal_nv_read( id, 0, len, buf );
    }
  }

  return (status);
}

/*********************************************************************
 * API FUNCTIONS
 */


/*********************************************************************
 * @fn          zgInit
 *
 * @brief
 *
 *   Initialize the Z-Stack Globals. If an item doesn't exist in
 *   NV memory, write the system default into NV memory. But if
 *   it exists, set the item to the value stored in NV memory.
 *
 * NOTE: The Startup Options (ZCD_NV_STARTUP_OPTION) indicate
 *       that the Config state items (zgItemTable) need to be
 *       set to defaults (ZCD_STARTOPT_DEFAULT_CONFIG_STATE). The
 *
 *
 * @param       none
 *
 * @return      ZSUCCESS if successful, NV_ITEM_UNINIT if item did not
 *              exist in NV, NV_OPER_FAILED if failure.
 */
uint8 zgInit( void )
{
  uint8  setDefault = FALSE;

  // Do we want to default the Config state values
  if ( zgReadStartupOptions() & ZCD_STARTOPT_DEFAULT_CONFIG_STATE )
  {
    setDefault = TRUE;
  }

#if 0
  // Enable this section if you need to track the number of resets
  // This section is normally disabled to minimize "wear" on NV memory
  uint16 bootCnt = 0;

  // Update the Boot Counter
  if ( osal_nv_item_init( ZCD_NV_BOOTCOUNTER, sizeof(bootCnt), &bootCnt ) == ZSUCCESS )
  {
    // Get the old value from NV memory
    osal_nv_read( ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt );
  }

  // Increment the Boot Counter and store it into NV memory
  if ( setDefault )
    bootCnt = 0;
  else
    bootCnt++;
  osal_nv_write( ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt );
#endif

  // Initialize the Extended PAN ID as my own extended address
  ZMacGetReq( ZMacExtAddr, zgExtendedPANID );

#ifndef NONWK
  // Initialize the Pre-Configured Key to the default key
  osal_memcpy( zgPreConfigKey, defaultKey, SEC_KEY_LEN );  // Do NOT Change!!!

  // Initialize the Pre-Configured Key to the default key
  osal_memcpy( zgPreConfigTCLinkKey, defaultTCLinkKey, SEC_KEY_LEN );
#endif // NONWK

  // Initialize the items table
  zgInitItems( setDefault );

  // Clear the Config State default
  if ( setDefault )
  {
    zgWriteStartupOptions( ZG_STARTUP_CLEAR, ZCD_STARTOPT_DEFAULT_CONFIG_STATE );
  }

  return ( ZSUCCESS );
}

/*********************************************************************
 * @fn          zgInitItems
 *
 * @brief       Initializes RAM variables from NV.  If NV items don't
 *              exist, then the NV is initialize with what is in RAM
 *              variables.
 *
 * @param       none
 *
 * @return      none
 */
void zgInitItems( uint8 setDefault )
{
  uint8  i = 0;

  while ( zgItemTable[i].id != 0x00 )
  {
    // Initialize the item
    zgItemInit( zgItemTable[i].id, zgItemTable[i].len, zgItemTable[i].buf, setDefault  );

    // Move on to the next item
    i++;
  }
}

/*********************************************************************
 * @fn          zgReadStartupOptions
 *
 * @brief       Reads the ZCD_NV_STARTUP_OPTION NV Item.
 *
 * @param       none
 *
 * @return      the ZCD_NV_STARTUP_OPTION NV item
 */
uint8 zgReadStartupOptions( void )
{
  // Default to Use Config State and Use Network State
  uint8 startupOption = 0;

  // This should have been done in ZMain.c, but just in case.
  if ( osal_nv_item_init( ZCD_NV_STARTUP_OPTION,
                              sizeof(startupOption),
                              &startupOption ) == ZSUCCESS )
  {
    // Read saved startup control
    osal_nv_read( ZCD_NV_STARTUP_OPTION,
                  0,
                  sizeof( startupOption ),
                  &startupOption);
  }
  return ( startupOption );
}

/*********************************************************************
 * @fn          zgWriteStartupOptions
 *
 * @brief       Writes bits into the ZCD_NV_STARTUP_OPTION NV Item.
 *
 * @param       action - ZG_STARTUP_SET set bit, ZG_STARTUP_CLEAR to
 *               clear bit. The set bit is an OR operation, and the
 *               clear bit is an AND ~(bitOptions) operation.
 *
 * @param       bitOptions - which bits to perform action on:
 *                      ZCD_STARTOPT_DEFAULT_CONFIG_STATE
 *                      ZCD_STARTOPT_DEFAULT_NETWORK_STATE
 *
 * @return      ZSUCCESS if successful
 */
uint8 zgWriteStartupOptions( uint8 action, uint8 bitOptions )
{
  uint8 status;
  uint8 startupOptions = 0;

  status = osal_nv_read( ZCD_NV_STARTUP_OPTION,
                0,
                sizeof( startupOptions ),
                &startupOptions );

  if ( status == ZSUCCESS )
  {
    if ( action == ZG_STARTUP_SET )
    {
      // Set bits
      startupOptions |= bitOptions;
    }
    else
    {
      // Clear bits
      startupOptions &= (bitOptions ^ 0xFF);
    }

    // Changed?
    status = osal_nv_write( ZCD_NV_STARTUP_OPTION,
                 0,
                 sizeof( startupOptions ),
                 &startupOptions );
  }

  return ( status );
}

/*********************************************************************
 * @fn          zgSetItem
 *
 * @brief       Set RAM variables from set-NV, if it exist in the zgItemTable
 *
 * @param       id - NV ID
 *              len - NV item length
 *              buf - pointer to the input buffer
 *
 * @return      none
 */
void zgSetItem( uint16 id, uint16 len, void *buf )
{

  uint8  i = 0;

  // Look up the NV item table
  while ( zgItemTable[i].id != 0x00 )
  {
    if( zgItemTable[i].id == id )
    {
      if ( zgItemTable[i].len == len )
      {
        osal_memcpy( zgItemTable[i].buf, buf, len );
      }
      break;
    }
    // Move on to the next item
    i++;
  }

}


/*********************************************************************
*********************************************************************/
