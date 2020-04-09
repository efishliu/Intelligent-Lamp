/**************************************************************************************************
  Filename:       ZDSecMgr.c
  Revised:        $Date: 2009-03-31 09:06:47 -0700 (Tue, 31 Mar 2009) $
  Revision:       $Revision: 19604 $

  Description:    The ZigBee Device Security Manager.


  Copyright 2005-2008 Texas Instruments Incorporated. All rights reserved.

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

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
 * INCLUDES
 */
#include "ZComdef.h"
#include "OSAL.h"
#include "OSAL_NV.h"
#include "ZGlobals.h"
#include "ssp.h"
#include "nwk_globals.h"
#include "nwk.h"
#include "NLMEDE.h"
#include "AddrMgr.h"
#include "AssocList.h"
#include "APSMEDE.h"
#include "AF.h"
#include "ZDConfig.h"
#include "ZDApp.h"
#include "ZDSecMgr.h"


/******************************************************************************
 * CONSTANTS
 */
// maximum number of devices managed by this Security Manager
#if !defined ( ZDSECMGR_DEVICE_MAX )
  #define ZDSECMGR_DEVICE_MAX 3
#endif

// total number of preconfigured devices (EXT address, MASTER key)
//devtag.pro.security
//#define ZDSECMGR_PRECONFIG_MAX ZDSECMGR_DEVICE_MAX
#define ZDSECMGR_PRECONFIG_MAX 0

// maximum number of MASTER keys this device may hold
#define ZDSECMGR_MASTERKEY_MAX ZDSECMGR_DEVICE_MAX

// maximum number of LINK keys this device may store
#define ZDSECMGR_ENTRY_MAX ZDSECMGR_DEVICE_MAX

// total number of devices under control - authentication, SKKE, etc.
#define ZDSECMGR_CTRL_MAX ZDSECMGR_DEVICE_MAX

// total number of stored devices
#if !defined ( ZDSECMGR_STORED_DEVICES )
  #define ZDSECMGR_STORED_DEVICES 3
#endif
  
#define ZDSECMGR_CTRL_NONE       0
#define ZDSECMGR_CTRL_INIT       1
#define ZDSECMGR_CTRL_TK_MASTER  2
#define ZDSECMGR_CTRL_SKKE_INIT  3
#define ZDSECMGR_CTRL_SKKE_WAIT  4
#define ZDSECMGR_CTRL_SKKE_DONE  5
#define ZDSECMGR_CTRL_SKKE_FAIL  6
#define ZDSECMGR_CTRL_TK_NWK     7

#define ZDSECMGR_CTRL_BASE_CNTR      1
#define ZDSECMGR_CTRL_SKKE_INIT_CNTR 1
#define ZDSECMGR_CTRL_TK_NWK_CNTR    1

// set SKA slot maximum
#define ZDSECMGR_SKA_SLOT_MAX 1

// APSME Stub Implementations
#define ZDSecMgrMasterKeyGet   APSME_MasterKeyGet
#define ZDSecMgrLinkKeySet     APSME_LinkKeySet
#define ZDSecMgrLinkKeyDataGet APSME_LinkKeyDataGet
#define ZDSecMgrKeyFwdToChild  APSME_KeyFwdToChild

#if !defined( MAX_APS_FRAMECOUNTER_CHANGES )
  // The number of times the frame counter can change before
  // saving to NV
  #define MAX_APS_FRAMECOUNTER_CHANGES    10
#endif

/******************************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint8 extAddr[Z_EXTADDR_LEN];
  uint8 key[SEC_KEY_LEN];
} ZDSecMgrPreConfigData_t;

typedef struct
{
  uint16 ami;
  uint8  key[SEC_KEY_LEN];
} ZDSecMgrMasterKeyData_t;

//should match APSME_LinkKeyData_t;
typedef struct
{
  uint8               key[SEC_KEY_LEN];
  APSME_LinkKeyData_t apsmelkd;
} ZDSecMgrLinkKeyData_t;

typedef struct
{
  uint16                ami;
  ZDSecMgrLinkKeyData_t lkd;
  ZDSecMgr_Authentication_Option authenticateOption;
} ZDSecMgrEntry_t;

typedef struct
{
  ZDSecMgrEntry_t* entry;
  uint16           parentAddr;
  uint8            secure;
  uint8            state;
  uint8            cntr;
  //uint8          next;
} ZDSecMgrCtrl_t;

typedef struct
{
  uint16          nwkAddr;
  uint8*          extAddr;
  uint16          parentAddr;
  uint8           secure;
  uint8           devStatus;
  ZDSecMgrCtrl_t* ctrl;
} ZDSecMgrDevice_t;

/******************************************************************************
 * LOCAL VARIABLES
 */
#if 0 // Taken out because the following functionality is only used for test
      // purpose. A more efficient (above) way is used. It can be put
      // back in if customers request for a white/black list feature.
uint8 ZDSecMgrStoredDeviceList[ZDSECMGR_STORED_DEVICES][Z_EXTADDR_LEN] =
{
  { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};
#endif

uint8 ZDSecMgrTCExtAddr[Z_EXTADDR_LEN]=
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

uint8 ZDSecMgrTCMasterKey[SEC_KEY_LEN] =
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x89,0x67,0x45,0x23,0x01,0xEF,0xCD,0xAB};

uint8 ZDSecMgrTCAuthenticated = FALSE;
uint8 ZDSecMgrTCDataLoaded    = FALSE;

//devtag.pro.security - remove this
#if ( ZDSECMGR_PRECONFIG_MAX != 0 )
const ZDSecMgrPreConfigData_t ZDSecMgrPreConfigData[ZDSECMGR_PRECONFIG_MAX] =
{
  //---------------------------------------------------------------------------
  // DEVICE A
  //---------------------------------------------------------------------------
  {
    // extAddr
    {0x7C,0x01,0x12,0x13,0x14,0x15,0x16,0x17},

    // key
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
  },
  //---------------------------------------------------------------------------
  // DEVICE B
  //---------------------------------------------------------------------------
  {
    // extAddr
    {0x84,0x03,0x00,0x00,0x00,0x4B,0x12,0x00},

    // key
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
  },
  //---------------------------------------------------------------------------
  // DEVICE C
  //---------------------------------------------------------------------------
  {
    // extAddr
    {0x3E,0x01,0x12,0x13,0x14,0x15,0x16,0x17},

    // key
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
  },
};
#endif // ( ZDSECMGR_PRECONFIG_MAX != 0 )

ZDSecMgrMasterKeyData_t* ZDSecMgrMasterKeyData = NULL;
ZDSecMgrEntry_t*         ZDSecMgrEntries       = NULL;
ZDSecMgrCtrl_t*          ZDSecMgrCtrlData      = NULL;
void ZDSecMgrAddrMgrUpdate( uint16 ami, uint16 nwkAddr );
void ZDSecMgrAddrMgrCB( uint8 update, AddrMgrEntry_t* newEntry, AddrMgrEntry_t* oldEntry );

uint8 ZDSecMgrPermitJoiningEnabled;
uint8 ZDSecMgrPermitJoiningTimed;

APSME_LinkKeyData_t TrustCenterLinkKey;

/******************************************************************************
 * PRIVATE FUNCTIONS
 *
 *   ZDSecMgrMasterKeyInit
 *   ZDSecMgrAddrStore
 *   ZDSecMgrExtAddrStore
 *   ZDSecMgrExtAddrLookup
 *   ZDSecMgrMasterKeyLookup
 *   ZDSecMgrMasterKeyStore
 *   ZDSecMgrEntryInit
 *   ZDSecMgrEntryLookup
 *   ZDSecMgrEntryLookupAMI
 *   ZDSecMgrEntryLookupExt
 *   ZDSecMgrEntryFree
 *   ZDSecMgrEntryNew
 *   ZDSecMgrCtrlInit
 *   ZDSecMgrCtrlRelease
 *   ZDSecMgrCtrlLookup
 *   ZDSecMgrCtrlSet
 *   ZDSecMgrCtrlAdd
 *   ZDSecMgrCtrlTerm
 *   ZDSecMgrCtrlReset
 *   ZDSecMgrMasterKeyLoad
 *   ZDSecMgrAppKeyGet
 *   ZDSecMgrAppKeyReq
 *   ZDSecMgrEstablishKey
 *   ZDSecMgrSendMasterKey
 *   ZDSecMgrSendNwkKey
 *   ZDSecMgrDeviceEntryRemove
 *   ZDSecMgrDeviceEntryAdd
 *   ZDSecMgrDeviceCtrlHandler
 *   ZDSecMgrDeviceCtrlSetup
 *   ZDSecMgrDeviceCtrlUpdate
 *   ZDSecMgrDeviceRemove
 *   ZDSecMgrDeviceValidateSKKE
 *   ZDSecMgrDeviceValidateRM
 *   ZDSecMgrDeviceValidateCM
 *   ZDSecMgrDeviceValidate
 *   ZDSecMgrDeviceJoin
 *   ZDSecMgrDeviceJoinDirect
 *   ZDSecMgrDeviceJoinFwd
 *   ZDSecMgrDeviceNew
 *   ZDSecMgrAssocDeviceAuth
 *   ZDSecMgrAuthInitiate
 *   ZDSecMgrAuthNwkKey
 */
//-----------------------------------------------------------------------------
// master key data
//-----------------------------------------------------------------------------
void ZDSecMgrMasterKeyInit( void );

//-----------------------------------------------------------------------------
// address management
//-----------------------------------------------------------------------------
ZStatus_t ZDSecMgrAddrStore( uint16 nwkAddr, uint8* extAddr, uint16* ami );
ZStatus_t ZDSecMgrExtAddrStore( uint16 nwkAddr, uint8* extAddr, uint16* ami );
ZStatus_t ZDSecMgrExtAddrLookup( uint8* extAddr, uint16* ami );

//-----------------------------------------------------------------------------
// MASTER key data
//-----------------------------------------------------------------------------
ZStatus_t ZDSecMgrMasterKeyLookup( uint16 ami, uint8** key );
ZStatus_t ZDSecMgrMasterKeyStore( uint16 ami, uint8* key );

//-----------------------------------------------------------------------------
// entry data
//-----------------------------------------------------------------------------
void ZDSecMgrEntryInit( void );
ZStatus_t ZDSecMgrEntryLookup( uint16 nwkAddr, ZDSecMgrEntry_t** entry );
ZStatus_t ZDSecMgrEntryLookupAMI( uint16 ami, ZDSecMgrEntry_t** entry );
ZStatus_t ZDSecMgrEntryLookupExt( uint8* extAddr, ZDSecMgrEntry_t** entry );
void ZDSecMgrEntryFree( ZDSecMgrEntry_t* entry );
ZStatus_t ZDSecMgrEntryNew( ZDSecMgrEntry_t** entry );
ZStatus_t ZDSecMgrAuthenticationSet( uint8* extAddr, ZDSecMgr_Authentication_Option option );

//-----------------------------------------------------------------------------
// control data
//-----------------------------------------------------------------------------
void ZDSecMgrCtrlInit( void );
void ZDSecMgrCtrlRelease( ZDSecMgrCtrl_t* ctrl );
void ZDSecMgrCtrlLookup( ZDSecMgrEntry_t* entry, ZDSecMgrCtrl_t** ctrl );
void ZDSecMgrCtrlSet( ZDSecMgrDevice_t* device,
                      ZDSecMgrEntry_t*  entry,
                      ZDSecMgrCtrl_t*   ctrl );
ZStatus_t ZDSecMgrCtrlAdd( ZDSecMgrDevice_t* device, ZDSecMgrEntry_t*  entry );
void ZDSecMgrCtrlTerm( ZDSecMgrEntry_t* entry );
ZStatus_t ZDSecMgrCtrlReset( ZDSecMgrDevice_t* device,
                             ZDSecMgrEntry_t*  entry );

//-----------------------------------------------------------------------------
// key support
//-----------------------------------------------------------------------------
ZStatus_t ZDSecMgrMasterKeyLoad( uint8* extAddr, uint8* key );
ZStatus_t ZDSecMgrAppKeyGet( uint16  initNwkAddr,
                             uint8*  initExtAddr,
                             uint16  partNwkAddr,
                             uint8*  partExtAddr,
                             uint8** key,
                             uint8*  keyType );
void ZDSecMgrAppKeyReq( ZDO_RequestKeyInd_t* ind );
ZStatus_t ZDSecMgrEstablishKey( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrSendMasterKey( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrSendNwkKey( ZDSecMgrDevice_t* device );

//-----------------------------------------------------------------------------
// device entry
//-----------------------------------------------------------------------------
void ZDSecMgrDeviceEntryRemove( ZDSecMgrEntry_t* entry );
ZStatus_t ZDSecMgrDeviceEntryAdd( ZDSecMgrDevice_t* device, uint16 ami );

//-----------------------------------------------------------------------------
// device control
//-----------------------------------------------------------------------------
void ZDSecMgrDeviceCtrlHandler( ZDSecMgrDevice_t* device );
void ZDSecMgrDeviceCtrlSetup( ZDSecMgrDevice_t* device );
void ZDSecMgrDeviceCtrlUpdate( uint8* extAddr, uint8 state );

//-----------------------------------------------------------------------------
// device management
//-----------------------------------------------------------------------------
void ZDSecMgrDeviceRemove( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceValidateSKKE( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceValidateRM( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceValidateCM( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceValidate( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceJoin( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceJoinDirect( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceJoinFwd( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceNew( ZDSecMgrDevice_t* device );

//-----------------------------------------------------------------------------
// association management
//-----------------------------------------------------------------------------
void ZDSecMgrAssocDeviceAuth( associated_devices_t* assoc );

//-----------------------------------------------------------------------------
// authentication management
//-----------------------------------------------------------------------------
void ZDSecMgrAuthInitiate( uint8* responder );
void ZDSecMgrAuthNwkKey( void );

/******************************************************************************
 * @fn          ZDSecMgrMasterKeyInit                     ]
 *
 * @brief       Initialize master key data.
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrMasterKeyInit( void )
{
  uint16 index;
  uint16 size;

  // allocate MASTER key data
  size = (short)( sizeof(ZDSecMgrMasterKeyData_t) * ZDSECMGR_MASTERKEY_MAX );

  ZDSecMgrMasterKeyData = osal_mem_alloc( size );

  // initialize MASTER key data
  if ( ZDSecMgrMasterKeyData != NULL )
  {
    for ( index = 0; index < ZDSECMGR_MASTERKEY_MAX; index++ )
    {
      ZDSecMgrMasterKeyData[index].ami = INVALID_NODE_ADDR;
    }
  }
}
//devtag.pro.security
#if 0
void ZDSecMgrMasterKeyInit( void )
{
  uint16         index;
  uint16         size;
  AddrMgrEntry_t entry;


  // allocate MASTER key data
  size = (short)( sizeof(ZDSecMgrMasterKeyData_t) * ZDSECMGR_MASTERKEY_MAX );

  ZDSecMgrMasterKeyData = osal_mem_alloc( size );

  // initialize MASTER key data
  if ( ZDSecMgrMasterKeyData != NULL )
  {
    for ( index = 0; index < ZDSECMGR_MASTERKEY_MAX; index++ )
    {
      ZDSecMgrMasterKeyData[index].ami = INVALID_NODE_ADDR;
    }

    // check if preconfigured keys are enabled
    //-------------------------------------------------------------------------
    #if ( ZDSECMGR_PRECONFIG_MAX != 0 )
    //-------------------------------------------------------------------------
    if ( zgPreConfigKeys == TRUE )
    {
      // sync configured data
      entry.user = ADDRMGR_USER_SECURITY;

      for ( index = 0; index < ZDSECMGR_PRECONFIG_MAX; index++ )
      {
        // check for Address Manager entry
        AddrMgrExtAddrSet( entry.extAddr,
                           (uint8*)ZDSecMgrPreConfigData[index].extAddr );

        if ( AddrMgrEntryLookupExt( &entry ) != TRUE )
        {
          // update Address Manager
          AddrMgrEntryUpdate( &entry );
        }

        if ( entry.index != INVALID_NODE_ADDR )
        {
          // sync MASTER keys with Address Manager index
          ZDSecMgrMasterKeyData[index].ami = entry.index;

          osal_memcpy( ZDSecMgrMasterKeyData[index].key,
                   (void*)ZDSecMgrPreConfigData[index].key, SEC_KEY_LEN );
        }
      }
    }
    //-------------------------------------------------------------------------
    #endif // ( ZDSECMGR_PRECONFIG_MAX != 0 )
    //-------------------------------------------------------------------------
  }
}
#endif

/******************************************************************************
 * @fn          ZDSecMgrAddrStore
 *
 * @brief       Store device addresses.
 *
 * @param       nwkAddr - [in] NWK address
 * @param       extAddr - [in] EXT address
 * @param       ami     - [out] Address Manager index
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrAddrStore( uint16 nwkAddr, uint8* extAddr, uint16* ami )
{
  ZStatus_t      status;
  AddrMgrEntry_t entry;


  // add entry
  entry.user    = ADDRMGR_USER_SECURITY;
  entry.nwkAddr = nwkAddr;
  AddrMgrExtAddrSet( entry.extAddr, extAddr );

  if ( AddrMgrEntryUpdate( &entry ) == TRUE )
  {
    // return successful results
    *ami   = entry.index;
    status = ZSuccess;
  }
  else
  {
    // return failed results
    *ami   = entry.index;
    status = ZNwkUnknownDevice;
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrExtAddrStore
 *
 * @brief       Store EXT address.
 *
 * @param       extAddr - [in] EXT address
 * @param       ami     - [out] Address Manager index
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrExtAddrStore( uint16 nwkAddr, uint8* extAddr, uint16* ami )
{
  ZStatus_t      status;
  AddrMgrEntry_t entry;


  // add entry
  entry.user    = ADDRMGR_USER_SECURITY;
  entry.nwkAddr = nwkAddr;
  AddrMgrExtAddrSet( entry.extAddr, extAddr );

  if ( AddrMgrEntryUpdate( &entry ) == TRUE )
  {
    // return successful results
    *ami   = entry.index;
    status = ZSuccess;
  }
  else
  {
    // return failed results
    *ami   = entry.index;
    status = ZNwkUnknownDevice;
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrExtAddrLookup
 *
 * @brief       Lookup index for specified EXT address.
 *
 * @param       extAddr - [in] EXT address
 * @param       ami     - [out] Address Manager index
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrExtAddrLookup( uint8* extAddr, uint16* ami )
{
  ZStatus_t      status;
  AddrMgrEntry_t entry;


  // lookup entry
  entry.user = ADDRMGR_USER_SECURITY;
  AddrMgrExtAddrSet( entry.extAddr, extAddr );

  if ( AddrMgrEntryLookupExt( &entry ) == TRUE )
  {
    // return successful results
    *ami   = entry.index;
    status = ZSuccess;
  }
  else
  {
    // return failed results
    *ami   = entry.index;
    status = ZNwkUnknownDevice;
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrMasterKeyLookup
 *
 * @brief       Lookup MASTER key for specified address index.
 *
 * @param       ami - [in] Address Manager index
 * @param       key - [out] valid MASTER key
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrMasterKeyLookup( uint16 ami, uint8** key )
{
  ZStatus_t status;
  uint16    index;


  // initialize results
  *key   = NULL;
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrMasterKeyData != NULL )
  {
    for ( index = 0; index < ZDSECMGR_MASTERKEY_MAX ; index++ )
    {
      if ( ZDSecMgrMasterKeyData[index].ami == ami )
      {
        // return successful results
        *key   = ZDSecMgrMasterKeyData[index].key;
        status = ZSuccess;

        // break from loop
        index  = ZDSECMGR_MASTERKEY_MAX;
      }
    }
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrMasterKeyStore
 *
 * @brief       Store MASTER key for specified address index.
 *
 * @param       ami - [in] Address Manager index
 * @param       key - [in] valid key to store
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrMasterKeyStore( uint16 ami, uint8* key )
{
  ZStatus_t status;
  uint16    index;
  uint8*    entry;


  // initialize results
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrMasterKeyData != NULL )
  {
    for ( index = 0; index < ZDSECMGR_MASTERKEY_MAX ; index++ )
    {
      if ( ZDSecMgrMasterKeyData[index].ami == INVALID_NODE_ADDR )
      {
        // store EXT address index
        ZDSecMgrMasterKeyData[index].ami = ami;

        entry = ZDSecMgrMasterKeyData[index].key;

        if ( key != NULL )
        {
          osal_memcpy( entry, key,  SEC_KEY_LEN );
        }
        else
        {
          osal_memset( entry, 0, SEC_KEY_LEN );
        }

        // return successful results
        status = ZSuccess;

        // break from loop
        index  = ZDSECMGR_MASTERKEY_MAX;
      }
    }
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrEntryInit
 *
 * @brief       Initialize entry sub module
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrEntryInit( void )
{
  uint16 size;
  uint16 index;

  // allocate entry data
  size = (short)( sizeof(ZDSecMgrEntry_t) * ZDSECMGR_ENTRY_MAX );

  ZDSecMgrEntries = osal_mem_alloc( size );

  // initialize data
  if ( ZDSecMgrEntries != NULL )
  {
    for( index = 0; index < ZDSECMGR_ENTRY_MAX; index++ )
    {
      ZDSecMgrEntries[index].ami = INVALID_NODE_ADDR;
    }
  }
  ZDSecMgrRestoreFromNV();
}

/******************************************************************************
 * @fn          ZDSecMgrEntryLookup
 *
 * @brief       Lookup entry index using specified NWK address.
 *
 * @param       nwkAddr - [in] NWK address
 * @param       entry   - [out] valid entry
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEntryLookup( uint16 nwkAddr, ZDSecMgrEntry_t** entry )
{
  ZStatus_t      status;
  uint16         index;
  AddrMgrEntry_t addrMgrEntry;


  // initialize results
  *entry = NULL;
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrEntries != NULL )
  {
    addrMgrEntry.user    = ADDRMGR_USER_SECURITY;
    addrMgrEntry.nwkAddr = nwkAddr;

    if ( AddrMgrEntryLookupNwk( &addrMgrEntry ) == TRUE )
    {
      for ( index = 0; index < ZDSECMGR_ENTRY_MAX ; index++ )
      {
        if ( addrMgrEntry.index == ZDSecMgrEntries[index].ami )
        {
          // return successful results
          *entry = &ZDSecMgrEntries[index];
          status = ZSuccess;

          // break from loop
          index = ZDSECMGR_ENTRY_MAX;
        }
      }
    }
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrEntryLookupAMI
 *
 * @brief       Lookup entry using specified address index
 *
 * @param       ami   - [in] Address Manager index
 * @param       entry - [out] valid entry
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEntryLookupAMI( uint16 ami, ZDSecMgrEntry_t** entry )
{
  ZStatus_t status;
  uint16    index;


  // initialize results
  *entry = NULL;
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrEntries != NULL )
  {
    for ( index = 0; index < ZDSECMGR_ENTRY_MAX ; index++ )
    {
      if ( ZDSecMgrEntries[index].ami == ami )
      {
        // return successful results
        *entry = &ZDSecMgrEntries[index];
        status = ZSuccess;

        // break from loop
        index = ZDSECMGR_ENTRY_MAX;
      }
    }
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrEntryLookupExt
 *
 * @brief       Lookup entry index using specified EXT address.
 *
 * @param       extAddr - [in] EXT address
 * @param       entry   - [out] valid entry
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEntryLookupExt( uint8* extAddr, ZDSecMgrEntry_t** entry )
{
  ZStatus_t status;
  uint16    ami;


  // initialize results
  *entry = NULL;
  status = ZNwkUnknownDevice;

  // lookup address index
  if ( ZDSecMgrExtAddrLookup( extAddr, &ami ) == ZSuccess )
  {
    status = ZDSecMgrEntryLookupAMI( ami, entry );
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrEntryFree
 *
 * @brief       Free entry.
 *
 * @param       entry - [in] valid entry
 *
 * @return      ZStatus_t
 */
void ZDSecMgrEntryFree( ZDSecMgrEntry_t* entry )
{
  entry->ami = INVALID_NODE_ADDR;
}

/******************************************************************************
 * @fn          ZDSecMgrEntryNew
 *
 * @brief       Get a new entry.
 *
 * @param       entry - [out] valid entry
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEntryNew( ZDSecMgrEntry_t** entry )
{
  ZStatus_t status;
  uint16    index;


  // initialize results
  *entry = NULL;
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrEntries != NULL )
  {
    // find available entry
    for ( index = 0; index < ZDSECMGR_ENTRY_MAX ; index++ )
    {
      if ( ZDSecMgrEntries[index].ami == INVALID_NODE_ADDR )
      {
        // return successful result
        *entry = &ZDSecMgrEntries[index];
        status = ZSuccess;

        // Set the authentication option to default
        ZDSecMgrEntries[index].authenticateOption = ZDSecMgr_Not_Authenticated;

        // break from loop
        index = ZDSECMGR_ENTRY_MAX;
      }
    }
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrCtrlInit
 *
 * @brief       Initialize control sub module
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrCtrlInit( void )
{
  uint16 size;
  uint16 index;

  // allocate entry data
  size = (short)( sizeof(ZDSecMgrCtrl_t) * ZDSECMGR_CTRL_MAX );

  ZDSecMgrCtrlData = osal_mem_alloc( size );

  // initialize data
  if ( ZDSecMgrCtrlData != NULL )
  {
    for( index = 0; index < ZDSECMGR_CTRL_MAX; index++ )
    {
      ZDSecMgrCtrlData[index].state = ZDSECMGR_CTRL_NONE;
    }
  }
}

/******************************************************************************
 * @fn          ZDSecMgrCtrlRelease
 *
 * @brief       Release control data.
 *
 * @param       ctrl - [in] valid control data
 *
 * @return      none
 */
void ZDSecMgrCtrlRelease( ZDSecMgrCtrl_t* ctrl )
{
  // should always be enough entry control data
  ctrl->state = ZDSECMGR_CTRL_NONE;
}

/******************************************************************************
 * @fn          ZDSecMgrCtrlLookup
 *
 * @brief       Lookup control data.
 *
 * @param       entry - [in] valid entry data
 * @param       ctrl  - [out] control data - NULL if not found
 *
 * @return      none
 */
void ZDSecMgrCtrlLookup( ZDSecMgrEntry_t* entry, ZDSecMgrCtrl_t** ctrl )
{
  uint16 index;


  // initialize search results
  *ctrl = NULL;

  // verify data is available
  if ( ZDSecMgrCtrlData != NULL )
  {
    for ( index = 0; index < ZDSECMGR_CTRL_MAX; index++ )
    {
      // make sure control data is in use
      if ( ZDSecMgrCtrlData[index].state != ZDSECMGR_CTRL_NONE )
      {
        // check for entry match
        if ( ZDSecMgrCtrlData[index].entry == entry )
        {
          // return this control data
          *ctrl = &ZDSecMgrCtrlData[index];

          // break from loop
          index = ZDSECMGR_CTRL_MAX;
        }
      }
    }
  }
}

/******************************************************************************
 * @fn          ZDSecMgrCtrlSet
 *
 * @brief       Set control data.
 *
 * @param       device - [in] valid device data
 * @param       entry  - [in] valid entry data
 * @param       ctrl   - [in] valid control data
 *
 * @return      none
 */
void ZDSecMgrCtrlSet( ZDSecMgrDevice_t* device,
                      ZDSecMgrEntry_t*  entry,
                      ZDSecMgrCtrl_t*   ctrl )
{
  // set control date
  ctrl->parentAddr = device->parentAddr;
  ctrl->secure     = device->secure;
  ctrl->entry      = entry;
  ctrl->state      = ZDSECMGR_CTRL_INIT;
  ctrl->cntr       = 0;

  // set device pointer
  device->ctrl = ctrl;
}

/******************************************************************************
 * @fn          ZDSecMgrCtrlAdd
 *
 * @brief       Add control data.
 *
 * @param       device - [in] valid device data
 * @param       entry  - [in] valid entry data
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrCtrlAdd( ZDSecMgrDevice_t* device, ZDSecMgrEntry_t*  entry )
{
  ZStatus_t status;
  uint16    index;


  // initialize results
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrCtrlData != NULL )
  {
    // look for an empty slot
    for ( index = 0; index < ZDSECMGR_CTRL_MAX; index++ )
    {
      if ( ZDSecMgrCtrlData[index].state == ZDSECMGR_CTRL_NONE )
      {
        // return successful results
        ZDSecMgrCtrlSet( device, entry, &ZDSecMgrCtrlData[index] );

        status = ZSuccess;

        // break from loop
        index = ZDSECMGR_CTRL_MAX;
      }
    }
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrCtrlTerm
 *
 * @brief       Terminate device control.
 *
 * @param       entry - [in] valid entry data
 *
 * @return      none
 */
void ZDSecMgrCtrlTerm( ZDSecMgrEntry_t* entry )
{
  ZDSecMgrCtrl_t* ctrl;

  // remove device from control data
  ZDSecMgrCtrlLookup ( entry, &ctrl );

  if ( ctrl != NULL )
  {
    ZDSecMgrCtrlRelease ( ctrl );
  }
}

/******************************************************************************
 * @fn          ZDSecMgrCtrlReset
 *
 * @brief       Reset control data.
 *
 * @param       device - [in] valid device data
 * @param       entry  - [in] valid entry data
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrCtrlReset( ZDSecMgrDevice_t* device, ZDSecMgrEntry_t* entry )
{
  ZStatus_t       status;
  ZDSecMgrCtrl_t* ctrl;


  // initialize results
  status = ZNwkUnknownDevice;

  // look for a match for the entry
  ZDSecMgrCtrlLookup( entry, &ctrl );

  if ( ctrl != NULL )
  {
    ZDSecMgrCtrlSet( device, entry, ctrl );

    status = ZSuccess;
  }
  else
  {
    status = ZDSecMgrCtrlAdd( device, entry );
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrMasterKeyLoad
 *
 * @brief       Load the MASTER key for device with specified EXT
 *              address.
 *
 * @param       extAddr - [in] EXT address of device
 * @param       key     - [in] MASTER key shared with device
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrMasterKeyLoad( uint8* extAddr, uint8* key )
{
  ZStatus_t status;
  uint8*    loaded;
  uint16    ami;


  // set status based on policy
  status = ZDSecMgrExtAddrLookup( extAddr, &ami );

  if ( status == ZSuccess )
  {
    // get the address index
    if ( ZDSecMgrMasterKeyLookup( ami, &loaded ) == ZSuccess )
    {
      // overwrite old key
      osal_memcpy( loaded, key, SEC_KEY_LEN );
    }
    else
    {
      // store new key -- NULL will zero key
      status = ZDSecMgrMasterKeyStore( ami, key );
    }
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrAppKeyGet
 *
 * @brief       get an APP key - option APP(MASTER or LINK) key
 *
 * @param       initNwkAddr - [in] NWK address of initiator device
 * @param       initExtAddr - [in] EXT address of initiator device
 * @param       partNwkAddr - [in] NWK address of partner device
 * @param       partExtAddr - [in] EXT address of partner device
 * @param       key         - [out] APP(MASTER or LINK) key
 * @param       keyType     - [out] APP(MASTER or LINK) key type
 *
 * @return      ZStatus_t
 */
uint8 ZDSecMgrAppKeyType = KEY_TYPE_APP_LINK;    // Set the default key type
                                                 // to KEY_TYPE_APP_LINK since
                                                 // only specific requirement
                                                 // right now comes from SE profile

ZStatus_t ZDSecMgrAppKeyGet( uint16  initNwkAddr,
                             uint8*  initExtAddr,
                             uint16  partNwkAddr,
                             uint8*  partExtAddr,
                             uint8** key,
                             uint8*  keyType )
{
  // Intentionally unreferenced parameters
  (void)initNwkAddr;
  (void)initExtAddr;
  (void)partNwkAddr;
  (void)partExtAddr;
  
  //---------------------------------------------------------------------------
  // note:
  // should use a robust mechanism to generate keys, for example
  // combine EXT addresses and call a hash function
  //---------------------------------------------------------------------------
  SSP_GetTrueRand( SEC_KEY_LEN, *key );

  *keyType = ZDSecMgrAppKeyType;

  return ZSuccess;
}

/******************************************************************************
 * @fn          ZDSecMgrAppKeyReq
 *
 * @brief       Process request for APP key between two devices.
 *
 * @param       device - [in] ZDO_RequestKeyInd_t, request info
 *
 * @return      none
 */
void ZDSecMgrAppKeyReq( ZDO_RequestKeyInd_t* ind )
{
  APSME_TransportKeyReq_t req;
  uint8                   initExtAddr[Z_EXTADDR_LEN];
  uint16                  partNwkAddr;
  uint8                   key[SEC_KEY_LEN];


  // validate initiator and partner
  if ( ( APSME_LookupNwkAddr( ind->partExtAddr, &partNwkAddr ) == TRUE ) &&
       ( APSME_LookupExtAddr( ind->srcAddr, initExtAddr ) == TRUE      )   )
  {
    // point the key to some memory
    req.key = key;

    // get an APP key - option APP (MASTER or LINK) key
    if ( ZDSecMgrAppKeyGet( ind->srcAddr,
                            initExtAddr,
                            partNwkAddr,
                            ind->partExtAddr,
                            &req.key,
                            &req.keyType ) == ZSuccess )
    {
      // always secure
      req.nwkSecure = TRUE;
      req.apsSecure = TRUE;
      req.tunnel    = NULL;

      // send key to initiator device
      req.dstAddr   = ind->srcAddr;
      req.extAddr   = ind->partExtAddr;
      req.initiator = TRUE;
      APSME_TransportKeyReq( &req );

      // send key to partner device
      req.dstAddr   = partNwkAddr;
      req.extAddr   = initExtAddr;
      req.initiator = FALSE;

      APSME_TransportKeyReq( &req );
    }
  }
}

/******************************************************************************
 * @fn          ZDSecMgrEstablishKey
 *
 * @brief       Start SKKE with device joining network.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEstablishKey( ZDSecMgrDevice_t* device )
{
  ZStatus_t               status;
  APSME_EstablishKeyReq_t req;


  req.respExtAddr = device->extAddr;
  req.method      = APSME_SKKE_METHOD;

  if ( device->parentAddr == NLME_GetShortAddr() )
  {
    req.dstAddr   = device->nwkAddr;
    //devtag.0604.todo - remove obsolete
    req.apsSecure = FALSE;
    req.nwkSecure = FALSE;
  }
  else
  {
    req.dstAddr   = device->parentAddr;
    //devtag.0604.todo - remove obsolete
    req.apsSecure = TRUE;
    req.nwkSecure = TRUE;
  }

  status = APSME_EstablishKeyReq( &req );

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrSendMasterKey
 *
 * @brief       Send MASTER key to device joining network.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrSendMasterKey( ZDSecMgrDevice_t* device )
{
  ZStatus_t               status;
  APSME_TransportKeyReq_t req;


  req.keyType = KEY_TYPE_TC_MASTER;
  req.extAddr = device->extAddr;
  req.tunnel  = NULL;

  ZDSecMgrMasterKeyLookup( device->ctrl->entry->ami, &req.key );

  //check if using secure hop to to parent
  if ( device->parentAddr != NLME_GetShortAddr() )
  {
    //send to parent with security
    req.dstAddr   = device->parentAddr;
    req.nwkSecure = TRUE;
    req.apsSecure = TRUE;
  }
  else
  {
    //direct with no security
    req.dstAddr   = device->nwkAddr;
    req.nwkSecure = FALSE;
    req.apsSecure = FALSE;
  }

  status = APSME_TransportKeyReq( &req );

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrSendNwkKey
 *
 * @brief       Send NWK key to device joining network.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrSendNwkKey( ZDSecMgrDevice_t* device )
{
  ZStatus_t               status;
  APSME_TransportKeyReq_t req;
  APSDE_FrameTunnel_t     tunnel;

  req.dstAddr   = device->nwkAddr;
  req.extAddr   = device->extAddr;

  if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    req.keyType   = KEY_TYPE_NWK_HIGH;
  else
    req.keyType   = KEY_TYPE_NWK;

  if ( (ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH)
      || (ZG_CHECK_SECURITY_MODE == ZG_SECURITY_SE_STANDARD) )
  {
    // set values
    req.keySeqNum = _NIB.nwkActiveKey.keySeqNum;
    req.key       = _NIB.nwkActiveKey.key;
    //devtag.pro.security.todo - make sure that if there is no link key the NWK
    //key isn't used to secure the frame at the APS layer -- since the receiving
    //device may not have a NWK key yet
    req.apsSecure = TRUE;

    // check if using secure hop to to parent
    if ( device->parentAddr == NLME_GetShortAddr() )
    {
      req.nwkSecure = FALSE;
      req.tunnel    = NULL;
    }
    else
    {
      req.nwkSecure   = TRUE;
      req.tunnel      = &tunnel;
      req.tunnel->tna = device->parentAddr;
      req.tunnel->dea = device->extAddr;
    }
  }
  else
  {
    // default values
    //devtag.0604.verify
    req.nwkSecure = TRUE;
    req.apsSecure = FALSE;
    req.tunnel    = NULL;

    if ( device->parentAddr != NLME_GetShortAddr() )
    {
      req.dstAddr = device->parentAddr;
    }

    // special cases
    //devtag.0604.todo - modify to preconfig flag
    if ( device->secure == FALSE )
    {
      req.keySeqNum = _NIB.nwkActiveKey.keySeqNum;
      req.key       = _NIB.nwkActiveKey.key;

      // check if using secure hop to to parent
      if ( device->parentAddr == NLME_GetShortAddr() )
      {
        req.nwkSecure = FALSE;
      }
    }
    else
    {
      req.key       = NULL;
      req.keySeqNum = 0;
    }
  }

  status = APSME_TransportKeyReq( &req );

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceEntryRemove
 *
 * @brief       Remove device entry.
 *
 * @param       entry - [in] valid entry
 *
 * @return      none
 */
void ZDSecMgrDeviceEntryRemove( ZDSecMgrEntry_t* entry )
{
  // terminate device control
  if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
  {
    ZDSecMgrCtrlTerm( entry );
  }

  // remove device from entry data
  ZDSecMgrEntryFree( entry );

  // remove EXT address
  //ZDSecMgrExtAddrRelease( aiOld );
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceEntryAdd
 *
 * @brief       Add entry.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 * @param       ami    - [in] Address Manager index
 *
 * @return      ZStatus_t
 */
void ZDSecMgrAddrMgrUpdate( uint16 ami, uint16 nwkAddr )
{
  AddrMgrEntry_t entry;

  // get the ami data
  entry.user  = ADDRMGR_USER_SECURITY;
  entry.index = ami;

  AddrMgrEntryGet( &entry );

  // check if NWK address is same
  if ( entry.nwkAddr != nwkAddr )
  {
    // update NWK address
    entry.nwkAddr = nwkAddr;

    AddrMgrEntryUpdate( &entry );
  }
}

ZStatus_t ZDSecMgrDeviceEntryAdd( ZDSecMgrDevice_t* device, uint16 ami )
{
  ZStatus_t        status;
  ZDSecMgrEntry_t* entry;


  // initialize as unknown until completion
  status = ZNwkUnknownDevice;

  device->ctrl = NULL;

  // make sure not already registered
  if ( ZDSecMgrEntryLookup( device->nwkAddr, &entry ) == ZSuccess )
  {
    // verify that address index is same
    if ( entry->ami != ami )
    {
      // remove conflicting entry
      ZDSecMgrDeviceEntryRemove( entry );

      if ( ZDSecMgrEntryLookupAMI( ami, &entry ) == ZSuccess )
      {
        // update NWK address
        ZDSecMgrAddrMgrUpdate( ami, device->nwkAddr );
      }
    }
  }
  else if ( ZDSecMgrEntryLookupAMI( ami, &entry ) == ZSuccess )
  {
    // update NWK address
    ZDSecMgrAddrMgrUpdate( ami, device->nwkAddr );
  }

  // check if a new entry needs to be created
  if ( entry == NULL )
  {
    // get new entry
    if ( ZDSecMgrEntryNew( &entry ) == ZSuccess )
    {
      // reset entry lkd

      // finish setting up entry
      entry->ami = ami;

      // update NWK address
      ZDSecMgrAddrMgrUpdate( ami, device->nwkAddr );

      // enter new device into device control
      if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
      {
        status = ZDSecMgrCtrlAdd( device, entry );
      }
      else
      {
        status = ZSuccess;
      }
    }
  }
  else
  {
    // reset entry lkd

    // reset entry in entry control
    if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
    {
      status = ZDSecMgrCtrlReset( device, entry );
    }
    else
    {
      status = ZSuccess;
    }
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceCtrlHandler
 *
 * @brief       Device control handler.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      none
 */
void ZDSecMgrDeviceCtrlHandler( ZDSecMgrDevice_t* device )
{
  uint8 state;
  uint8 cntr;


  state = device->ctrl->state;
  cntr  = ZDSECMGR_CTRL_BASE_CNTR;

  switch ( state )
  {
    case ZDSECMGR_CTRL_TK_MASTER:
      if ( ZDSecMgrSendMasterKey( device ) == ZSuccess )
      {
        state = ZDSECMGR_CTRL_SKKE_INIT;
        cntr  = ZDSECMGR_CTRL_SKKE_INIT_CNTR;
      }
      break;

    case ZDSECMGR_CTRL_SKKE_INIT:
      if ( ZDSecMgrEstablishKey( device ) == ZSuccess )
      {
        state = ZDSECMGR_CTRL_SKKE_WAIT;
      }
      break;

    case ZDSECMGR_CTRL_SKKE_WAIT:
      // continue to wait for SKA control timeout
      break;

    case ZDSECMGR_CTRL_TK_NWK:
      if ( ZDSecMgrSendNwkKey( device ) == ZSuccess )
      {
        state = ZDSECMGR_CTRL_NONE;
      }
      break;

    default:
      state = ZDSECMGR_CTRL_NONE;
      break;
  }

  if ( state != ZDSECMGR_CTRL_NONE )
  {
    device->ctrl->state = state;
    device->ctrl->cntr  = cntr;

    osal_start_timerEx(ZDAppTaskID, ZDO_SECMGR_EVENT, 100 );
  }
  else
  {
    ZDSecMgrCtrlRelease( device->ctrl );
  }
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceCtrlSetup
 *
 * @brief       Setup device control.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
void ZDSecMgrDeviceCtrlSetup( ZDSecMgrDevice_t* device )
{
  if ( device->ctrl != NULL )
  {
    if ( device->secure == FALSE )
    {
      // send the master key data to the joining device
      device->ctrl->state = ZDSECMGR_CTRL_TK_MASTER;
    }
    else
    {
      // start SKKE
      device->ctrl->state = ZDSECMGR_CTRL_SKKE_INIT;
    }

    ZDSecMgrDeviceCtrlHandler( device );
  }
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceCtrlUpdate
 *
 * @brief       Update control data.
 *
 * @param       extAddr - [in] EXT address
 * @param       state   - [in] new control state
 *
 * @return      none
 */
void ZDSecMgrDeviceCtrlUpdate( uint8* extAddr, uint8 state )
{
  ZDSecMgrEntry_t* entry;
  ZDSecMgrCtrl_t*  ctrl;


  // lookup device entry data
  ZDSecMgrEntryLookupExt( extAddr, &entry );

  if ( entry != NULL )
  {
    // lookup device control data
    ZDSecMgrCtrlLookup( entry, &ctrl );

    // make sure control data is valid
    if ( ctrl != NULL )
    {
      // possible state transitions
      if ( ctrl->state == ZDSECMGR_CTRL_SKKE_WAIT )
      {
        if ( state == ZDSECMGR_CTRL_SKKE_DONE )
        {
          // send the network key
          ctrl->state = ZDSECMGR_CTRL_TK_NWK;
          ctrl->cntr  = ZDSECMGR_CTRL_TK_NWK_CNTR;
        }
        else if ( state == ZDSECMGR_CTRL_SKKE_FAIL )
        {
          // force default timeout in order to cleanup control logic
          ctrl->state = ZDSECMGR_CTRL_SKKE_FAIL;
          ctrl->cntr  = ZDSECMGR_CTRL_BASE_CNTR;
        }
      }
      // timer should be active
    }
  }
}

void APSME_SKA_TimerExpired( uint8 initiator, uint8* partExtAddr );
void APSME_SKA_TimerExpired( uint8 initiator, uint8* partExtAddr )
{
  if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
  {
    if ( initiator == TRUE )
    {
      ZDSecMgrDeviceCtrlUpdate( partExtAddr, ZDSECMGR_CTRL_SKKE_FAIL );
    }
  }
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceRemove
 *
 * @brief       Remove device from network.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      none
 */
void ZDSecMgrDeviceRemove( ZDSecMgrDevice_t* device )
{
  APSME_RemoveDeviceReq_t remDevReq;
  NLME_LeaveReq_t         leaveReq;
  associated_devices_t*   assoc;


  // check if parent, remove the device
  if ( device->parentAddr == NLME_GetShortAddr() )
  {
    // this is the parent of the device
    leaveReq.extAddr        = device->extAddr;
    leaveReq.removeChildren = FALSE;
    leaveReq.rejoin         = FALSE;

    // find child association
    assoc = AssocGetWithExt( device->extAddr );

    if ( ( assoc != NULL                            ) &&
         ( assoc->nodeRelation >= CHILD_RFD         ) &&
         ( assoc->nodeRelation <= CHILD_FFD_RX_IDLE )    )
    {
      // check if associated device is authenticated
      if ( assoc->devStatus & DEV_SEC_AUTH_STATUS )
      {
        leaveReq.silent = FALSE;
      }
      else
      {
        leaveReq.silent = TRUE;
      }

      NLME_LeaveReq( &leaveReq );
    }
  }
  else
  {
    // this is not the parent of the device
    remDevReq.parentAddr   = device->parentAddr;
    remDevReq.childExtAddr = device->extAddr;

    APSME_RemoveDeviceReq( &remDevReq );
  }
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceValidateSKKE
 *
 * @brief       Decide whether device is allowed for SKKE.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceValidateSKKE( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;
  uint16    ami;
  uint8*    key;


  // get EXT address
  status = ZDSecMgrExtAddrLookup( device->extAddr, &ami );

  if ( status == ZSuccess )
  {
    // get MASTER key
    status = ZDSecMgrMasterKeyLookup( ami, &key );

    if ( status == ZSuccess )
    {
    //  // check if initiator is Trust Center
    //  if ( device->nwkAddr == APSME_TRUSTCENTER_NWKADDR )
    //  {
    //    // verify NWK key not sent
    //    // devtag.todo
    //    // temporary - add device to internal data
    //    status = ZDSecMgrDeviceEntryAdd( device, ami );
    //  }
    //  else
    //  {
    //    // initiator not Trust Center - End to End SKKE - set policy
    //    // for accepting an SKKE initiation
    //    // temporary - add device to internal data
    //    status = ZDSecMgrDeviceEntryAdd( device, ami );
    //  }
        status = ZDSecMgrDeviceEntryAdd( device, ami );
    }
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceValidateRM (RESIDENTIAL MODE)
 *
 * @brief       Decide whether device is allowed.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceValidateRM( ZDSecMgrDevice_t* device )
{

  ZStatus_t status;
  status = ZSuccess;

  (void)device;  // Intentionally unreferenced parameter
  
  // For test purpose, turning off the zgSecurePermitJoin flag will force
  // the trust center to reject any newly joining devices by sending
  // Remove-device to the parents.
  if ( zgSecurePermitJoin == false )
  {
    status = ZNwkUnknownDevice;
  }



#if 0  // Taken out because the following functionality is only used for test
       // purpose. A more efficient (above) way is used. It can be put
       // back in if customers request for a white/black list feature.
       // ZDSecMgrStoredDeviceList[] is defined in ZDSecMgr.c

  // The following code processes the device black list (stored device list)
  // If the joining device is not part of the forbidden device list
  // Return ZSuccess. Otherwise, return ZNwkUnknownDevice. The trust center
  // will send Remove-device and ban the device from joining.

  uint8     index;
  uint8*    restricted;

  // Look through the stored device list - used for restricted devices
  for ( index = 0; index < ZDSECMGR_STORED_DEVICES; index++ )
  {
    restricted = ZDSecMgrStoredDeviceList[index];

    if ( AddrMgrExtAddrEqual( restricted, device->extAddr )  == TRUE )
    {
      // return as unknown device in regards to validation
      status = ZNwkUnknownDevice;

      // break from loop
      index = ZDSECMGR_STORED_DEVICES;
    }
  }

#endif

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceValidateCM (COMMERCIAL MODE)
 *
 * @brief       Decide whether device is allowed.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
//devtag.pro.security
ZStatus_t ZDSecMgrDeviceValidateCM( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;
  uint16    ami;
  uint8*    key;


//  // check for pre configured setting
//  if ( device->secure == TRUE )
//  {
//    // get EXT address and MASTER key
//    status = ZDSecMgrExtAddrLookup( device->extAddr, &ami );
//
//    if ( status == ZSuccess )
//    {
//      status = ZDSecMgrMasterKeyLookup( ami, &key );
//    }
//  }
//  else
//  {
    // implement EXT address and MASTER key policy here -- the total number of
    // Security Manager entries should never exceed the number of EXT addresses
    // and MASTER keys available

    // set status based on policy
    //status = ZNwkUnknownDevice;

    // set status based on policy
    status = ZSuccess; // ZNwkUnknownDevice;

    // get key based on policy
    key = ZDSecMgrTCMasterKey;

    // if policy, store new EXT address
    status = ZDSecMgrAddrStore( device->nwkAddr, device->extAddr, &ami );

    // set the key
    ZDSecMgrMasterKeyLoad( device->extAddr, key );
//  }

  // if EXT address and MASTER key available -- add device
  if ( status == ZSuccess )
  {
    // add device to internal data - with control
    status = ZDSecMgrDeviceEntryAdd( device, ami );
  }

  return status;
}
//devtag.pro.security
#if 0
ZStatus_t ZDSecMgrDeviceValidateCM( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;
  uint16    ami;
  uint8*    key;


  // check for pre configured setting
  if ( device->secure == TRUE )
  {
    // get EXT address and MASTER key
    status = ZDSecMgrExtAddrLookup( device->extAddr, &ami );

    if ( status == ZSuccess )
    {
      status = ZDSecMgrMasterKeyLookup( ami, &key );
    }
  }
  else
  {
    // implement EXT address and MASTER key policy here -- the total number of
    // Security Manager entries should never exceed the number of EXT addresses
    // and MASTER keys available

    // set status based on policy
    status = ZSuccess; // ZNwkUnknownDevice;

    // get the address index
    if ( ZDSecMgrExtAddrLookup( device->extAddr, &ami ) != ZSuccess )
    {
      // if policy, store new EXT address
      status = ZDSecMgrAddrStore( device->nwkAddr, device->extAddr, &ami );
    }

    // get the address index
    if ( ZDSecMgrMasterKeyLookup( ami, &key ) != ZSuccess )
    {
      // if policy, store new key -- NULL will zero key
      status = ZDSecMgrMasterKeyStore( ami, NULL );
    }
  }

  // if EXT address and MASTER key available -- add device
  if ( status == ZSuccess )
  {
    // add device to internal data - with control
    status = ZDSecMgrDeviceEntryAdd( device, ami );
  }

  return status;
}
#endif

/******************************************************************************
 * @fn          ZDSecMgrDeviceValidate
 *
 * @brief       Decide whether device is allowed.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceValidate( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;


  if ( ZDSecMgrPermitJoiningEnabled == TRUE )
  {
    // device may be joining with a secure flag but it is ultimately the Trust
    // Center that decides -- check if expected pre configured device --
    // override settings
    if ( zgPreConfigKeys == TRUE )
    {
      device->secure = TRUE;
    }
    else
    {
      device->secure = FALSE;
    }

    if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    {
      status = ZDSecMgrDeviceValidateCM( device );
    }
    else // ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_RESIDENTIAL )
    {
      status = ZDSecMgrDeviceValidateRM( device );
    }
  }
  else
  {
    status = ZNwkUnknownDevice;
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceJoin
 *
 * @brief       Try to join this device.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceJoin( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;

  // attempt to validate device
  status = ZDSecMgrDeviceValidate( device );

  if ( status == ZSuccess )
  {
    if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    {
      ZDSecMgrDeviceCtrlSetup( device );
    }
    else // ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_RESIDENTIAL )
    {
      //send the nwk key data to the joining device
      status = ZDSecMgrSendNwkKey( device );
    }
  }
  else
  {
    // not allowed, remove the device
    ZDSecMgrDeviceRemove( device );
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceJoinDirect
 *
 * @brief       Try to join this device as a direct child.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceJoinDirect( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;

  status = ZDSecMgrDeviceJoin( device );

  if ( status == ZSuccess )
  {
    // set association status to authenticated
    ZDSecMgrAssocDeviceAuth( AssocGetWithShort( device->nwkAddr ) );
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceJoinFwd
 *
 * @brief       Forward join to Trust Center.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceJoinFwd( ZDSecMgrDevice_t* device )
{
  ZStatus_t               status;
  APSME_UpdateDeviceReq_t req;


  // forward any joining device to the Trust Center -- the Trust Center will
  // decide if the device is allowed to join
  status = ZSuccess;

  // forward authorization to the Trust Center
  req.dstAddr    = APSME_TRUSTCENTER_NWKADDR;
  req.devAddr    = device->nwkAddr;
  req.devExtAddr = device->extAddr;

  // set security status, option for router to reject if policy set
  if ( (device->devStatus & DEV_HIGH_SEC_STATUS) )
  {
    if ( device->devStatus & DEV_REJOIN_STATUS )
    {
      if ( device->secure == TRUE )
        req.status = APSME_UD_HIGH_SECURED_REJOIN;
      else
        req.status = APSME_UD_HIGH_UNSECURED_REJOIN;
    }
    else
      req.status = APSME_UD_HIGH_UNSECURED_JOIN;
  }
  else
  {
    if ( device->devStatus & DEV_REJOIN_STATUS )
    {
      if ( device->secure == TRUE )
        req.status = APSME_UD_STANDARD_SECURED_REJOIN;
      else
        req.status = APSME_UD_STANDARD_UNSECURED_REJOIN;
    }
    else
      req.status = APSME_UD_STANDARD_UNSECURED_JOIN;
  }

  if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    req.apsSecure = TRUE;
  else
    req.apsSecure = FALSE;

  // send and APSME_UPDATE_DEVICE request to the trust center
  status = APSME_UpdateDeviceReq( &req );

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrDeviceNew
 *
 * @brief       Process a new device.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceNew( ZDSecMgrDevice_t* joiner )
{
  ZStatus_t status;

  if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
  {
    // try to join this device
    status = ZDSecMgrDeviceJoinDirect( joiner );
  }
  else
  {
    status = ZDSecMgrDeviceJoinFwd( joiner );
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrAssocDeviceAuth
 *
 * @brief       Set associated device status to authenticated
 *
 * @param       assoc - [in, out] associated_devices_t
 *
 * @return      none
 */
void ZDSecMgrAssocDeviceAuth( associated_devices_t* assoc )
{
  if ( assoc != NULL )
  {
    assoc->devStatus |= DEV_SEC_AUTH_STATUS;
  }
}

/******************************************************************************
 * @fn          ZDSecMgrAuthInitiate
 *
 * @brief       Initiate entity authentication
 *
 * @param       responder - [in] responder EXT address
 *
 * @return      none
 */
void ZDSecMgrAuthInitiate( uint8* responder )
{
  APSME_AuthenticateReq_t req;


  // make sure NWK address is available
  if ( APSME_LookupNwkAddr( responder, &req.nwkAddr ) )
  {
    // set request fields
    req.extAddr   = responder;
    req.action    = APSME_EA_INITIATE;
    req.challenge = NULL;

    // start EA processing
    APSME_AuthenticateReq( &req );
  }
}

/******************************************************************************
 * @fn          ZDSecMgrAuthNwkKey
 *
 * @brief       Handle next step in authentication process
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrAuthNwkKey()
{
  if ( devState == DEV_END_DEVICE_UNAUTH )
  {
    if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    {
      uint8 parent[Z_EXTADDR_LEN];

      // get parent's EXT address
      NLME_GetCoordExtAddr( parent );

      // begin entity authentication with parent
      ZDSecMgrAuthInitiate( parent );
    }
    else // ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_RESIDENTIAL )
    {
      // inform ZDO that device has been authenticated
      osal_set_event ( ZDAppTaskID, ZDO_DEVICE_AUTH );
    }
  }
}

/******************************************************************************
 * PUBLIC FUNCTIONS
 */
/******************************************************************************
 * @fn          ZDSecMgrInit
 *
 * @brief       Initialize ZigBee Device Security Manager.
 *
 * @param       none
 *
 * @return      none
 */
#if ( ADDRMGR_CALLBACK_ENABLED == 1 )
void ZDSecMgrAddrMgrCB( uint8 update, AddrMgrEntry_t* newEntry, AddrMgrEntry_t* oldEntry );
void ZDSecMgrAddrMgrCB( uint8           update,
                        AddrMgrEntry_t* newEntry,
                        AddrMgrEntry_t* oldEntry )
{
  (void)update;
  (void)newEntry;
  (void)oldEntry;
}
#endif // ( ADDRMGR_CALLBACK_ENABLED == 1 )

void ZDSecMgrInit( void )
{
  if ( (ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH)
      || (ZG_CHECK_SECURITY_MODE == ZG_SECURITY_SE_STANDARD) )
  {
    // initialize sub modules
    ZDSecMgrMasterKeyInit();
    ZDSecMgrEntryInit();

    if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
    {
      ZDSecMgrCtrlInit();
    }

    // register with Address Manager
    #if ( ADDRMGR_CALLBACK_ENABLED == 1 )
    AddrMgrRegister( ADDRMGR_REG_SECURITY, ZDSecMgrAddrMgrCB );
    #endif
  }

  if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
  {
    // configure SKA slot data
    APSME_SKA_SlotInit( ZDSECMGR_SKA_SLOT_MAX );
  }
  else if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_SE_STANDARD )
  {
    // Setup the preconfig Trust Center Link Key
    TrustCenterLinkKey.key = zgPreConfigTCLinkKey;
    TrustCenterLinkKey.txFrmCntr = 0;
    TrustCenterLinkKey.rxFrmCntr = 0;
#if defined ( NV_RESTORE )
    if ( osal_nv_item_init( ZCD_NV_SECURE_TCLINKKEY_TXFRAME, sizeof(uint32), &(TrustCenterLinkKey.txFrmCntr) ) == ZSUCCESS )
    {
      osal_nv_read( ZCD_NV_SECURE_TCLINKKEY_TXFRAME, 0, sizeof(uint32), &(TrustCenterLinkKey.txFrmCntr) );
    }
    if ( osal_nv_item_init( ZCD_NV_SECURE_TCLINKKEY_RXFRAME, sizeof(uint32), &(TrustCenterLinkKey.rxFrmCntr) ) == ZSUCCESS )
    {
      osal_nv_read( ZCD_NV_SECURE_TCLINKKEY_RXFRAME, 0, sizeof(uint32), &(TrustCenterLinkKey.rxFrmCntr) );
    }
#endif
    APSME_TCLinkKeySetup( 0x0000, &TrustCenterLinkKey );
  }

  if ( ZG_SECURE_ENABLED )
  {
    if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
    {
      // setup joining permissions
      ZDSecMgrPermitJoiningEnabled = TRUE;
      ZDSecMgrPermitJoiningTimed   = FALSE;
    }
  }

  // configure security based on security mode and type of device
  ZDSecMgrConfig();
}

/******************************************************************************
 * @fn          ZDSecMgrConfig
 *
 * @brief       Configure ZigBee Device Security Manager.
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrConfig( void )
{
  if ( ZG_SECURE_ENABLED )
  {
    SSP_Init();

    if ( (ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH)
        || (ZG_CHECK_SECURITY_MODE == ZG_SECURITY_SE_STANDARD) )
    {
      if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
      {
        // COMMERCIAL MODE - COORDINATOR DEVICE
        APSME_SecurityCM_CD();
      }
      else if ( ZSTACK_ROUTER_BUILD )
      {
        // COMMERCIAL MODE - ROUTER DEVICE
        APSME_SecurityCM_RD();
      }
      else
      {
        // COMMERCIAL MODE - END DEVICE
        APSME_SecurityCM_ED();
      }
    }
    else // ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_RESIDENTIAL )
    {
      if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
      {
        // RESIDENTIAL MODE - COORDINATOR DEVICE
        APSME_SecurityRM_CD();
      }
      else if ( ZSTACK_ROUTER_BUILD )
      {
        // RESIDENTIAL MODE - ROUTER DEVICE
        APSME_SecurityRM_RD();
      }
      else
      {
        // RESIDENTIAL MODE - END DEVICE
        APSME_SecurityRM_ED();
      }
    }
  }
  else
  {
    // NO SECURITY
    APSME_SecurityNM();
  }
}

/******************************************************************************
 * @fn          ZDSecMgrPermitJoining
 *
 * @brief       Process request to change joining permissions.
 *
 * @param       duration - [in] timed duration for join in seconds
 *                         - 0x00 not allowed
 *                         - 0xFF allowed without timeout
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
uint8 ZDSecMgrPermitJoining( uint8 duration )
{
  uint8 accept;


  ZDSecMgrPermitJoiningTimed = FALSE;

  if ( duration > 0 )
  {
    ZDSecMgrPermitJoiningEnabled = TRUE;

    if ( duration != 0xFF )
    {
      ZDSecMgrPermitJoiningTimed = TRUE;
    }
  }
  else
  {
    ZDSecMgrPermitJoiningEnabled = FALSE;
  }

  accept = TRUE;

  return accept;
}

/******************************************************************************
 * @fn          ZDSecMgrPermitJoiningTimeout
 *
 * @brief       Process permit joining timeout
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrPermitJoiningTimeout( void )
{
  if ( ZDSecMgrPermitJoiningTimed == TRUE )
  {
    ZDSecMgrPermitJoiningEnabled = FALSE;
    ZDSecMgrPermitJoiningTimed   = FALSE;
  }
}

/******************************************************************************
 * @fn          ZDSecMgrNewDeviceEvent
 *
 * @brief       Process a the new device event, if found reset new device
 *              event/timer.
 *
 * @param       none
 *
 * @return      uint8 - found(TRUE:FALSE)
 */
uint8 ZDSecMgrNewDeviceEvent( void )
{
  uint8                 found;
  ZDSecMgrDevice_t      device;
  AddrMgrEntry_t        addrEntry;
  associated_devices_t* assoc;
  ZStatus_t             status;

  // initialize return results
  found = FALSE;

  // look for device in the security init state
  assoc = AssocMatchDeviceStatus( DEV_SEC_INIT_STATUS );

  if ( assoc != NULL )
  {
    // device found
    found = TRUE;

    // check for preconfigured security
    if ( zgPreConfigKeys == TRUE )
    {
      // set association status to authenticated
      ZDSecMgrAssocDeviceAuth( assoc );
    }

    // set up device info
    addrEntry.user  = ADDRMGR_USER_DEFAULT;
    addrEntry.index = assoc->addrIdx;
    AddrMgrEntryGet( &addrEntry );

    device.nwkAddr    = assoc->shortAddr;
    device.extAddr    = addrEntry.extAddr;
    device.parentAddr = NLME_GetShortAddr();
    device.secure     = FALSE;
    device.devStatus  = assoc->devStatus;

    // process new device
    status = ZDSecMgrDeviceNew( &device );

    if ( status == ZSuccess )
    {
      assoc->devStatus &= ~DEV_SEC_INIT_STATUS;
    }
    else if ( status == ZNwkUnknownDevice )
    {
      AssocRemove( addrEntry.extAddr );
    }
  }

  return found;
}

/******************************************************************************
 * @fn          ZDSecMgrEvent
 *
 * @brief       Handle ZDO Security Manager event/timer(ZDO_SECMGR_EVENT).
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrEvent( void )
{
  uint8            action;
  uint8            restart;
  uint16           index;
  AddrMgrEntry_t   entry;
  ZDSecMgrDevice_t device;


  // verify data is available
  if ( ZDSecMgrCtrlData != NULL )
  {
    action  = FALSE;
    restart = FALSE;

    // update all the counters
    for ( index = 0; index < ZDSECMGR_ENTRY_MAX; index++ )
    {
      if ( ZDSecMgrCtrlData[index].state !=  ZDSECMGR_CTRL_NONE )
      {
        if ( ZDSecMgrCtrlData[index].cntr != 0 )
        {
          ZDSecMgrCtrlData[index].cntr--;
        }

        if ( ( action == FALSE ) && ( ZDSecMgrCtrlData[index].cntr == 0 ) )
        {
          action = TRUE;

          // update from control data
          device.parentAddr = ZDSecMgrCtrlData[index].parentAddr;
          device.secure     = ZDSecMgrCtrlData[index].secure;
          device.ctrl       = &ZDSecMgrCtrlData[index];

          // set the user and address index
          entry.user  = ADDRMGR_USER_SECURITY;
          entry.index = ZDSecMgrCtrlData[index].entry->ami;

          // get the address data
          AddrMgrEntryGet( &entry );

          // set device address data
          device.nwkAddr = entry.nwkAddr;
          device.extAddr = entry.extAddr;

          // update from entry data
          ZDSecMgrDeviceCtrlHandler( &device );
        }
        else
        {
          restart = TRUE;
        }
      }
    }

    // check for timer restart
    if ( restart == TRUE )
    {
      osal_start_timerEx(ZDAppTaskID, ZDO_SECMGR_EVENT, 100 );
    }
  }
}

/******************************************************************************
 * @fn          ZDSecMgrEstablishKeyCfm
 *
 * @brief       Process the ZDO_EstablishKeyCfm_t message.
 *
 * @param       cfm - [in] ZDO_EstablishKeyCfm_t confirmation
 *
 * @return      none
 */
void ZDSecMgrEstablishKeyCfm( ZDO_EstablishKeyCfm_t* cfm )
{
  // send the NWK key
  if ( ( ZG_BUILD_COORDINATOR_TYPE ) && ( ZG_DEVICE_COORDINATOR_TYPE ) )
  {
    // update control for specified EXT address
    ZDSecMgrDeviceCtrlUpdate( cfm->partExtAddr, ZDSECMGR_CTRL_SKKE_DONE );
  }
  else
  {
    // this should be done when receiving the NWK key
    // if devState ==
    //if ( devState == DEV_END_DEVICE_UNAUTH )
        //osal_set_event( ZDAppTaskID, ZDO_DEVICE_AUTH );

    // if not in joining state -- this should trigger an event for an
    // end point that requested SKKE
    // if ( devState == DEV_END_DEVICE )
   //       devState == DEV_ROUTER;

  }
}

uint8 ZDSecMgrTCExtAddrCheck( uint8* extAddr );
uint8 ZDSecMgrTCExtAddrCheck( uint8* extAddr )
{
  uint8  match;
  uint8  lookup[Z_EXTADDR_LEN];

  match = FALSE;

  if ( AddrMgrExtAddrLookup( APSME_TRUSTCENTER_NWKADDR, lookup ) )
  {
    match = AddrMgrExtAddrEqual( lookup, extAddr );
  }

  return match;
}

void ZDSecMgrTCDataLoad( uint8* extAddr );
void ZDSecMgrTCDataLoad( uint8* extAddr )
{
  uint16 ami;
  uint8* key;

  if ( !ZDSecMgrTCDataLoaded )
  {
    if ( ZDSecMgrAddrStore( APSME_TRUSTCENTER_NWKADDR, extAddr, &ami ) == ZSuccess )
    {
      // if preconfigured load key
      if ( zgPreConfigKeys == TRUE )
      {
        if ( ZDSecMgrMasterKeyLookup( ami, &key ) != ZSuccess )
        {
          ZDSecMgrMasterKeyStore( ami, ZDSecMgrTCMasterKey );
        }
      }
    }

    ZDSecMgrTCDataLoaded = TRUE;
  }
}

/******************************************************************************
 * @fn          ZDSecMgrEstablishKeyInd
 *
 * @brief       Process the ZDO_EstablishKeyInd_t message.
 *
 * @param       ind - [in] ZDO_EstablishKeyInd_t indication
 *
 * @return      none
 */
void ZDSecMgrEstablishKeyInd( ZDO_EstablishKeyInd_t* ind )
{
  ZDSecMgrDevice_t        device;
  APSME_EstablishKeyRsp_t rsp;


  // load Trust Center data if needed
  ZDSecMgrTCDataLoad( ind->initExtAddr );

  if ( ZDSecMgrTCExtAddrCheck( ind->initExtAddr ) )
  {
    //IF (ind->srcAddr == APSME_TRUSTCENTER_NWKADDR)
    //OR
    //!ZDSecMgrTCAuthenticated
    //devtag.0604.critical
        //how is the parentAddr used here

    // initial SKKE from Trust Center via parent
    device.nwkAddr    = APSME_TRUSTCENTER_NWKADDR;
    device.parentAddr = ind->srcAddr;
  }
  else
  {
    // Trust Center direct or E2E SKKE
    device.nwkAddr    = ind->srcAddr;
    device.parentAddr = INVALID_NODE_ADDR;
  }

  device.extAddr = ind->initExtAddr;
  //devtag.pro.security.0724.todo - verify usage
  device.secure  = ind->nwkSecure;

  // validate device for SKKE
  if ( ZDSecMgrDeviceValidateSKKE( &device ) == ZSuccess )
  {
    rsp.accept = TRUE;
  }
  else
  {
    rsp.accept = FALSE;
  }

  rsp.dstAddr     = ind->srcAddr;
  rsp.initExtAddr = &ind->initExtAddr[0];
  //devtag.0604.todo - remove obsolete
  rsp.apsSecure   = ind->apsSecure;
  rsp.nwkSecure   = ind->nwkSecure;

  APSME_EstablishKeyRsp( &rsp );
}
//devtag.pro.security
#if 0
void ZDSecMgrEstablishKeyInd( ZDO_EstablishKeyInd_t* ind )
{
  ZDSecMgrDevice_t        device;
  APSME_EstablishKeyRsp_t rsp;


  device.extAddr = ind->initExtAddr;
  device.secure  = ind->secure;

  if ( ind->secure == FALSE )
  {
    // SKKE from Trust Center is not secured between child and parent
    device.nwkAddr    = APSME_TRUSTCENTER_NWKADDR;
    device.parentAddr = ind->srcAddr;
  }
  else
  {
    // SKKE from initiator should be secured
    device.nwkAddr    = ind->srcAddr;
    device.parentAddr = INVALID_NODE_ADDR;
  }

  rsp.dstAddr     = ind->srcAddr;
  rsp.initExtAddr = &ind->initExtAddr[0];
  rsp.secure      = ind->secure;

  // validate device for SKKE
  if ( ZDSecMgrDeviceValidateSKKE( &device ) == ZSuccess )
  {
    rsp.accept = TRUE;
  }
  else
  {
    rsp.accept = FALSE;
  }

  APSME_EstablishKeyRsp( &rsp );
}
#endif

/******************************************************************************
 * @fn          ZDSecMgrTransportKeyInd
 *
 * @brief       Process the ZDO_TransportKeyInd_t message.
 *
 * @param       ind - [in] ZDO_TransportKeyInd_t indication
 *
 * @return      none
 */
void ZDSecMgrTransportKeyInd( ZDO_TransportKeyInd_t* ind )
{
  uint8 index;

  // load Trust Center data if needed
  ZDSecMgrTCDataLoad( ind->srcExtAddr );

  if ( ind->keyType == KEY_TYPE_TC_MASTER )
  {
    if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    //ZDSecMgrTCMasterKey( ind );
    {
      if ( zgPreConfigKeys != TRUE )
      {
        // devtag.pro.security.todo - check if Trust Center address is configured and correct
        ZDSecMgrMasterKeyLoad( ind->srcExtAddr, ind->key );
      }
      else
      {
        // error condition - reject key
      }
    }
  }
  else if ( ( ind->keyType == KEY_TYPE_NWK      ) ||
            ( ind->keyType == 6                 ) ||
            ( ind->keyType == KEY_TYPE_NWK_HIGH )    )
  {
    // check for dummy NWK key (all zeros)
    for ( index = 0;
          ( (index < SEC_KEY_LEN) && (ind->key[index] == 0) );
          index++ );

    if ( index == SEC_KEY_LEN )
    {
      // load preconfigured key - once!!
      if ( !_NIB.nwkKeyLoaded )
      {
        SSP_UpdateNwkKey( (byte*)zgPreConfigKey, 0 );
        SSP_SwitchNwkKey( 0 );
      }
    }
    else
    {
      SSP_UpdateNwkKey( ind->key, ind->keySeqNum );
      if ( !_NIB.nwkKeyLoaded )
      {
        SSP_SwitchNwkKey( ind->keySeqNum );
      }
    }

    // handle next step in authentication process
    ZDSecMgrAuthNwkKey();
  }
  else if ( ind->keyType == KEY_TYPE_TC_LINK )
  {
    if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    {
      //ZDSecMgrTCLinkKey( ind );
    }
  }
  else if ( ind->keyType == KEY_TYPE_APP_MASTER )
  {
    if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    {
      uint16           ami;
      AddrMgrEntry_t   entry;
      ZDSecMgrEntry_t* entryZD;

      ZDSecMgrExtAddrLookup( ind->srcExtAddr, &ami );

      if ( ind->initiator == TRUE )
      {
        // get the ami data
        entry.user  = ADDRMGR_USER_SECURITY;
        entry.index = ami;
        AddrMgrEntryGet( &entry );

        if ( entry.nwkAddr != INVALID_NODE_ADDR )
        {
          APSME_EstablishKeyReq_t req;
          ZDSecMgrMasterKeyLoad( ind->srcExtAddr, ind->key );

          ZDSecMgrEntryLookupAMI( ami, &entryZD );

          if ( entryZD == NULL )
          {
            // get new entry
            if ( ZDSecMgrEntryNew( &entryZD ) == ZSuccess )
            {
              // finish setting up entry
              entryZD->ami = ami;
            }
          }

          req.respExtAddr = ind->srcExtAddr;
          req.method      = APSME_SKKE_METHOD;
          req.dstAddr     = entry.nwkAddr;
          //devtag.0604.todo - remove obsolete
          req.apsSecure   = FALSE;
          req.nwkSecure   = TRUE;
          APSME_EstablishKeyReq( &req );
        }
      }
      else
      {
        if ( ami == INVALID_NODE_ADDR )
        {
          // store new EXT address
          ZDSecMgrAddrStore( INVALID_NODE_ADDR, ind->srcExtAddr, &ami );
        }

        ZDSecMgrMasterKeyLoad( ind->srcExtAddr, ind->key );
      }

      //if ( entry.nwkAddr == INVALID_NODE_ADDR )
      //{
      //  ZDP_NwkAddrReq( ind->srcExtAddr, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
      //}
    }
  }
  else if ( ind->keyType == KEY_TYPE_APP_LINK )
  {
    if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    {
      uint16           ami;
      ZDSecMgrEntry_t* entry;

      // get the address index
      if ( ZDSecMgrExtAddrLookup( ind->srcExtAddr, &ami ) != ZSuccess )
      {
        // store new EXT address
        ZDSecMgrAddrStore( INVALID_NODE_ADDR, ind->srcExtAddr, &ami );
        ZDP_NwkAddrReq( ind->srcExtAddr, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
      }

      ZDSecMgrEntryLookupAMI( ami, &entry );

      if ( entry == NULL )
      {
        // get new entry
        if ( ZDSecMgrEntryNew( &entry ) == ZSuccess )
        {
          // finish setting up entry
          entry->ami = ami;
        }
      }

      ZDSecMgrLinkKeySet( ind->srcExtAddr, ind->key );
    }
  }
}

/******************************************************************************
 * @fn          ZDSecMgrUpdateDeviceInd
 *
 * @brief       Process the ZDO_UpdateDeviceInd_t message.
 *
 * @param       ind - [in] ZDO_UpdateDeviceInd_t indication
 *
 * @return      none
 */
void ZDSecMgrUpdateDeviceInd( ZDO_UpdateDeviceInd_t* ind )
{
  ZDSecMgrDevice_t device;


  device.nwkAddr    = ind->devAddr;
  device.extAddr    = ind->devExtAddr;
  device.parentAddr = ind->srcAddr;

  //if ( ( ind->status == APSME_UD_SECURED_JOIN   ) ||
  //     ( ind->status == APSME_UD_UNSECURED_JOIN )   )
  //{
  //  if ( ind->status == APSME_UD_SECURED_JOIN )
  //  {
  //    device.secure = TRUE;
  //  }
  //  else
  //  {
  //    device.secure = FALSE;
  //  }

    // try to join this device
    ZDSecMgrDeviceJoin( &device );
  //}
}

/******************************************************************************
 * @fn          ZDSecMgrRemoveDeviceInd
 *
 * @brief       Process the ZDO_RemoveDeviceInd_t message.
 *
 * @param       ind - [in] ZDO_RemoveDeviceInd_t indication
 *
 * @return      none
 */
void ZDSecMgrRemoveDeviceInd( ZDO_RemoveDeviceInd_t* ind )
{
  ZDSecMgrDevice_t device;


  // only accept from Trust Center
  if ( ind->srcAddr == APSME_TRUSTCENTER_NWKADDR )
  {
    // look up NWK address
    if ( APSME_LookupNwkAddr( ind->childExtAddr, &device.nwkAddr ) == TRUE )
    {
      device.parentAddr = NLME_GetShortAddr();
      device.extAddr    = ind->childExtAddr;

      // remove device
      ZDSecMgrDeviceRemove( &device );
    }
  }
}

/******************************************************************************
 * @fn          ZDSecMgrRequestKeyInd
 *
 * @brief       Process the ZDO_RequestKeyInd_t message.
 *
 * @param       ind - [in] ZDO_RequestKeyInd_t indication
 *
 * @return      none
 */
void ZDSecMgrRequestKeyInd( ZDO_RequestKeyInd_t* ind )
{
  if ( ind->keyType == KEY_TYPE_NWK )
  {
  }
  else if ( ind->keyType == KEY_TYPE_APP_MASTER )
  {
    ZDSecMgrAppKeyReq( ind );
  }
  else if ( ind->keyType == KEY_TYPE_TC_LINK )
  {
  }
  //else ignore
}

/******************************************************************************
 * @fn          ZDSecMgrSwitchKeyInd
 *
 * @brief       Process the ZDO_SwitchKeyInd_t message.
 *
 * @param       ind - [in] ZDO_SwitchKeyInd_t indication
 *
 * @return      none
 */
void ZDSecMgrSwitchKeyInd( ZDO_SwitchKeyInd_t* ind )
{
  SSP_SwitchNwkKey( ind->keySeqNum );

  // Save if nv
  ZDApp_NVUpdate();
}

/******************************************************************************
 * @fn          ZDSecMgrAuthenticateInd
 *
 * @brief       Process the ZDO_AuthenticateInd_t message.
 *
 * @param       ind - [in] ZDO_AuthenticateInd_t indication
 *
 * @return      none
 */
void ZDSecMgrAuthenticateInd( ZDO_AuthenticateInd_t* ind )
{
  APSME_AuthenticateReq_t req;
  AddrMgrEntry_t          entry;


  // update the address manager
  //---------------------------------------------------------------------------
  // note:
  // required for EA processing, but ultimately EA logic could also use the
  // neighbor table to look up addresses -- also(IF using EA) the neighbor
  // table is supposed to have authentication states for neighbors
  //---------------------------------------------------------------------------
  entry.user    = ADDRMGR_USER_SECURITY;
  entry.nwkAddr = ind->aps.initNwkAddr;
  AddrMgrExtAddrSet( entry.extAddr, ind->aps.initExtAddr );

  if ( AddrMgrEntryUpdate( &entry ) == TRUE )
  {
    // set request fields
    req.nwkAddr   = ind->aps.initNwkAddr;
    req.extAddr   = ind->aps.initExtAddr;
    req.action    = APSME_EA_ACCEPT;
    req.challenge = ind->aps.challenge;

    // start EA processing
    APSME_AuthenticateReq( &req );
  }
}

/******************************************************************************
 * @fn          ZDSecMgrAuthenticateCfm
 *
 * @brief       Process the ZDO_AuthenticateCfm_t message.
 *
 * @param       cfm - [in] ZDO_AuthenticateCfm_t confirmation
 *
 * @return      none
 */
void ZDSecMgrAuthenticateCfm( ZDO_AuthenticateCfm_t* cfm )
{
  if ( cfm->aps.status == ZSuccess )
  {
    if ( ( cfm->aps.initiator == TRUE ) && ( devState == DEV_END_DEVICE_UNAUTH ) )
    {
      // inform ZDO that device has been authenticated
      osal_set_event ( ZDAppTaskID, ZDO_DEVICE_AUTH );
    }
  }
}

#if ( ZG_BUILD_COORDINATOR_TYPE )
/******************************************************************************
 * @fn          ZDSecMgrUpdateNwkKey
 *
 * @brief       Load a new NWK key and trigger a network wide update.
 *
 * @param       key       - [in] new NWK key
 * @param       keySeqNum - [in] new NWK key sequence number
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrUpdateNwkKey( uint8* key, uint8 keySeqNum, uint16 dstAddr )
{
  ZStatus_t               status;
  APSME_TransportKeyReq_t req;

  // initialize common elements of local variables
  if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
    req.keyType   = KEY_TYPE_NWK_HIGH;
  else
    req.keyType   = KEY_TYPE_NWK;

  req.dstAddr   = dstAddr;
  req.keySeqNum = keySeqNum;
  req.key       = key;
  req.extAddr   = NULL;
  req.nwkSecure = TRUE;
  req.apsSecure = TRUE;
  req.tunnel    = NULL;

  if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
  {
    ZDSecMgrEntry_t*        entry;
    uint16                  index;
    AddrMgrEntry_t          addrEntry;

    addrEntry.user = ADDRMGR_USER_SECURITY;

    status = ZFailure;

    // verify data is available
    if ( ZDSecMgrEntries != NULL )
    {
      // find available entry
      for ( index = 0; index < ZDSECMGR_ENTRY_MAX ; index++ )
      {
        if ( ZDSecMgrEntries[index].ami != INVALID_NODE_ADDR )
        {
          // return successful result
          entry = &ZDSecMgrEntries[index];

          // get NWK address
          addrEntry.index = entry->ami;
          if ( AddrMgrEntryGet( &addrEntry ) == TRUE )
          {
            req.dstAddr = addrEntry.nwkAddr;
            req.extAddr = addrEntry.extAddr;
            status = APSME_TransportKeyReq( &req );
          }
        }
      }
    }
  }
  else // ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_RESIDENTIAL )
  {
    status = APSME_TransportKeyReq( &req );
  }

  SSP_UpdateNwkKey( key, keySeqNum );

  // Save if nv
  ZDApp_NVUpdate();

  return status;
}
#endif // ( ZG_BUILD_COORDINATOR_TYPE )

#if ( ZG_BUILD_COORDINATOR_TYPE )
/******************************************************************************
 * @fn          ZDSecMgrSwitchNwkKey
 *
 * @brief       Causes the NWK key to switch via a network wide command.
 *
 * @param       keySeqNum - [in] new NWK key sequence number
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrSwitchNwkKey( uint8 keySeqNum, uint16 dstAddr )
{
  ZStatus_t            status;
  APSME_SwitchKeyReq_t req;

  // initialize common elements of local variables
  req.dstAddr = dstAddr;
  req.keySeqNum = keySeqNum;

  if ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_PRO_HIGH )
  {
    ZDSecMgrEntry_t*     entry;
    uint16               index;
    AddrMgrEntry_t       addrEntry;

    addrEntry.user = ADDRMGR_USER_SECURITY;

    status = ZFailure;

    // verify data is available
    if ( ZDSecMgrEntries != NULL )
    {
      // find available entry
      for ( index = 0; index < ZDSECMGR_ENTRY_MAX ; index++ )
      {
        if ( ZDSecMgrEntries[index].ami != INVALID_NODE_ADDR )
        {
          // return successful result
          entry = &ZDSecMgrEntries[index];

          // get NWK address
          addrEntry.index = entry->ami;

          if ( AddrMgrEntryGet( &addrEntry ) == TRUE )
          {
            req.dstAddr = addrEntry.nwkAddr;
            status = APSME_SwitchKeyReq( &req );
          }
        }
      }
    }
  }
  else // ( ZG_CHECK_SECURITY_MODE == ZG_SECURITY_RESIDENTIAL )
  {
    status = APSME_SwitchKeyReq( &req );
  }

  SSP_SwitchNwkKey( keySeqNum );

  // Save if nv
  ZDApp_NVUpdate();

  return status;
}
#endif // ( ZG_BUILD_COORDINATOR_TYPE )

#if ( ZG_BUILD_JOINING_TYPE )
/******************************************************************************
 * @fn          ZDSecMgrRequestAppKey
 *
 * @brief       Request an application key with partner.
 *
 * @param       partNwkAddr - [in] partner network address
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrRequestAppKey( uint16 partNwkAddr )
{
  ZStatus_t             status;
  APSME_RequestKeyReq_t req;
  uint8                 partExtAddr[Z_EXTADDR_LEN];


  if ( AddrMgrExtAddrLookup( partNwkAddr, partExtAddr ) )
  {
    req.dstAddr = 0;
    req.keyType = KEY_TYPE_APP_MASTER;
    req.partExtAddr = partExtAddr;
    status = APSME_RequestKeyReq( &req );
  }
  else
  {
    status = ZFailure;
  }

  return status;
}
#endif // ( ZG_BUILD_JOINING_TYPE )

#if ( ZG_BUILD_JOINING_TYPE )
/******************************************************************************
 * @fn          ZDSecMgrSetupPartner
 *
 * @brief       Setup for application key partner.
 *
 * @param       partNwkAddr - [in] partner network address
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrSetupPartner( uint16 partNwkAddr, uint8* partExtAddr )
{
  AddrMgrEntry_t entry;
  ZStatus_t      status;

  status = ZFailure;

  // update the address manager
  entry.user    = ADDRMGR_USER_SECURITY;
  entry.nwkAddr = partNwkAddr;
  AddrMgrExtAddrSet( entry.extAddr, partExtAddr );

  if ( AddrMgrEntryUpdate( &entry ) == TRUE )
  {
    status = ZSuccess;

    // check for address discovery
    if ( partNwkAddr == INVALID_NODE_ADDR )
    {
      status = ZDP_NwkAddrReq( partExtAddr, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
    }
    else if ( !AddrMgrExtAddrValid( partExtAddr ) )
    {
      status = ZDP_IEEEAddrReq( partNwkAddr, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
    }
  }

  return status;
}
#endif // ( ZG_BUILD_JOINING_TYPE )

#if ( ZG_BUILD_COORDINATOR_TYPE )
/******************************************************************************
 * @fn          ZDSecMgrAppKeyTypeSet
 *
 * @brief       Set application key type.
 *
 * @param       keyType - [in] application key type (KEY_TYPE_APP_MASTER@2 or
 *                                                   KEY_TYPE_APP_LINK@3
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrAppKeyTypeSet( uint8 keyType )
{
  if ( keyType == KEY_TYPE_APP_LINK )
  {
    ZDSecMgrAppKeyType = KEY_TYPE_APP_LINK;
  }
  else
  {
    ZDSecMgrAppKeyType = KEY_TYPE_APP_MASTER;
  }

  return ZSuccess;
}
#endif

/******************************************************************************
 * ZigBee Device Security Manager - Stub Implementations
 */
/******************************************************************************
 * @fn          ZDSecMgrMasterKeyGet (stubs APSME_MasterKeyGet)
 *
 * @brief       Get MASTER key for specified EXT address.
 *
 * @param       extAddr - [in] EXT address
 * @param       key     - [out] MASTER key
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrMasterKeyGet( uint8* extAddr, uint8** key )
{
  ZStatus_t status;
  uint16    ami;


  // lookup entry for specified EXT address
  status = ZDSecMgrExtAddrLookup( extAddr, &ami );
  //status = ZDSecMgrEntryLookupExt( extAddr, &entry );

  if ( status == ZSuccess )
  {
    ZDSecMgrMasterKeyLookup( ami, key );
  }
  else
  {
    *key = NULL;
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrLinkKeySet (stubs APSME_LinkKeySet)
 *
 * @brief       Set <APSME_LinkKeyData_t> for specified NWK address.
 *
 * @param       extAddr - [in] EXT address
 * @param       data    - [in] APSME_LinkKeyData_t
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrLinkKeySet( uint8* extAddr, uint8* key )
{
  ZStatus_t        status;
  ZDSecMgrEntry_t* entry;


  // lookup entry index for specified EXT address
  status = ZDSecMgrEntryLookupExt( extAddr, &entry );

  if ( status == ZSuccess )
  {
    // setup the link key data reference
    osal_memcpy( entry->lkd.key, key, SEC_KEY_LEN );

    entry->lkd.apsmelkd.rxFrmCntr = 0;
    entry->lkd.apsmelkd.txFrmCntr = 0;
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrAuthenticationSet
 *
 * @brief       Mark the specific device as authenticated or not
 *
 * @param       extAddr - [in] EXT address
 * @param       option  - [in] authenticated or not
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrAuthenticationSet( uint8* extAddr, ZDSecMgr_Authentication_Option option )
{
  ZStatus_t        status;
  ZDSecMgrEntry_t* entry;


  // lookup entry index for specified EXT address
  status = ZDSecMgrEntryLookupExt( extAddr, &entry );

  if ( status == ZSuccess )
  {
    entry->authenticateOption = option;
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrAuthenticationCheck
 *
 * @brief       Check if the specific device has been authenticated or not
 *              For non-trust center device, always return true
 *
 * @param       shortAddr - [in] short address
 *
 * @return      TRUE @ authenticated with CBKE
 *              FALSE @ not authenticated
 */

uint8 ZDSecMgrAuthenticationCheck( uint16 shortAddr )
{
#if defined (SE_PROFILE)

  ZDSecMgrEntry_t* entry;
  uint8 extAddr[Z_EXTADDR_LEN];

  // If the local device is not the trust center, always return TRUE
  if ( NLME_GetShortAddr() != TCshortAddr )
  {
    return TRUE;
  }
  // Otherwise, check the authentication option
  else if ( AddrMgrExtAddrLookup( shortAddr, extAddr ) )
  {
    // lookup entry index for specified EXT address
    if ( ZDSecMgrEntryLookupExt( extAddr, &entry ) == ZSuccess )
    {
      if ( entry->authenticateOption != ZDSecMgr_Not_Authenticated )
      {
        return TRUE;
      }
      else
      {
        return FALSE;
      }
    }
  }
  return FALSE;

#else
  (void)shortAddr;  // Intentionally unreferenced parameter
  
  // For non AMI/SE Profile, perform no check and always return true.
  return TRUE;

#endif // SE_PROFILE
}


/******************************************************************************
 * @fn          ZDSecMgrLinkKeyDataGet (stubs APSME_LinkKeyDataGet)
 *
 * @brief       Get <APSME_LinkKeyData_t> for specified NWK address.
 *
 * @param       extAddr - [in] EXT address
 * @param       data    - [out] APSME_LinkKeyData_t
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrLinkKeyDataGet(uint8* extAddr, APSME_LinkKeyData_t** data)
{
  ZStatus_t        status;
  ZDSecMgrEntry_t* entry;


  // lookup entry index for specified NWK address
  status = ZDSecMgrEntryLookupExt( extAddr, &entry );

  if ( status == ZSuccess )
  {
    // setup the link key data reference
    (*data) = &entry->lkd.apsmelkd;
    (*data)->key = entry->lkd.key;
  }
  else
  {
    *data = NULL;
  }

  return status;
}

/******************************************************************************
 * @fn          ZDSecMgrKeyFwdToChild (stubs APSME_KeyFwdToChild)
 *
 * @brief       Verify and process key transportation to child.
 *
 * @param       ind - [in] APSME_TransportKeyInd_t
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
uint8 ZDSecMgrKeyFwdToChild( APSME_TransportKeyInd_t* ind )
{
  uint8 success;

  success = FALSE;

  // verify from Trust Center
  if ( ind->srcAddr == APSME_TRUSTCENTER_NWKADDR )
  {
    success = TRUE;

    // check for initial NWK key
    if ( ( ind->keyType == KEY_TYPE_NWK      ) ||
         ( ind->keyType == 6                 ) ||
         ( ind->keyType == KEY_TYPE_NWK_HIGH )    )
    {
      // set association status to authenticated
      ZDSecMgrAssocDeviceAuth( AssocGetWithExt( ind->dstExtAddr ) );
    }
  }

  return success;
}

/******************************************************************************
 * @fn          ZDSecMgrAddLinkKey
 *
 * @brief       Add the application link key to ZDSecMgr. Also mark the device
 *              as authenticated in the authenticateOption. Note that this function
 *              is hardwared to CBKE right now.
 *
 * @param       shortAddr - short address of the partner device
 * @param       extAddr - extended address of the partner device
 * @param       key - link key
 *
 * @return      none
 */
void ZDSecMgrAddLinkKey( uint16 shortAddr, uint8 *extAddr, uint8 *key)
{
  uint16           ami;
  ZDSecMgrEntry_t* entry;

  ZDSecMgrAddrStore( shortAddr, extAddr, &ami );

  ZDSecMgrEntryLookupAMI( ami, &entry );

  // If no existing entry, create one
  if ( entry == NULL )
  {
    if ( ZDSecMgrEntryNew( &entry ) == ZSuccess )
    {
      entry->ami = ami;
    }
  }
  // Write the link key
  APSME_LinkKeySet( extAddr, key );

#if defined (SE_PROFILE)
  // Mark the device as authenticated.
  ZDSecMgrAuthenticationSet( extAddr, ZDSecMgr_Authenticated_CBCK );
#endif

  // Write the new established link key to NV.
  ZDSecMgrWriteNV();
}

/******************************************************************************
 * @fn          ZDSecMgrInitNV
 *
 * @brief       Initialize the SecMgr entry data in NV.
 *
 * @param       none
 *
 * @return      uint8 - <osal_nv_item_init> return codes
 */
uint8 ZDSecMgrInitNV( void )
{
  uint8  status;
  uint16 size;

  size = (uint16)( sizeof(ZDSecMgrEntry_t) * ZDSECMGR_ENTRY_MAX );

  status = osal_nv_item_init( ZCD_NV_APS_LINK_KEY_TABLE, size, NULL );

  // The item does not already exist
  if ( status != ZSUCCESS )
  {
    ZDSecMgrSetDefaultNV();
  }

  return status;
}


/******************************************************************************
 * @fn          ZDSecMgrSetDefaultNV
 *
 * @brief       Set default SecMgr entry data in NV.
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrSetDefaultNV( void )
{
  nvDeviceListHdr_t hdr;

  // Initialize the header
  hdr.numRecs = 0;

  // Save off the header
  osal_nv_write( ZCD_NV_APS_LINK_KEY_TABLE, 0, sizeof( nvDeviceListHdr_t ), &hdr );
}



/*********************************************************************
 * @fn      ZDSecMgrWriteNV()
 *
 * @brief   Save off the link key list to NV
 *
 * @param   none
 *
 * @return  none
 */
void ZDSecMgrWriteNV( void )
{
  uint16 i;
  nvDeviceListHdr_t hdr;

  hdr.numRecs = 0;

  for ( i = 0; i < ZDSECMGR_ENTRY_MAX; i++ )
  {
    if ( ZDSecMgrEntries[i].ami != INVALID_NODE_ADDR )
    {
      // Save off the record
      osal_nv_write( ZCD_NV_APS_LINK_KEY_TABLE,
              (uint16)((sizeof(nvDeviceListHdr_t)) + (hdr.numRecs * sizeof(ZDSecMgrEntry_t))),
                      sizeof(ZDSecMgrEntry_t), &ZDSecMgrEntries[i] );
      hdr.numRecs++;
    }
  }

  // Save off the header
  osal_nv_write( ZCD_NV_APS_LINK_KEY_TABLE, 0, sizeof( nvDeviceListHdr_t ), &hdr );
}

/******************************************************************************
 * @fn          ZDSecMgrRestoreFromNV
 *
 * @brief       Restore the SecMgr entry data from NV.
 *
 * @param       none
 *
 * @return      ZStatus_t ZSuccess or ZFailure
 */
ZStatus_t ZDSecMgrRestoreFromNV( void )
{
  uint8 x = 0;
  nvDeviceListHdr_t hdr;

  // Initialize the device list
  if ( osal_nv_read( ZCD_NV_APS_LINK_KEY_TABLE, 0, sizeof(nvDeviceListHdr_t), &hdr ) == ZSUCCESS )
  {
    // Read in the device list
    for ( ; x < hdr.numRecs; x++ )
    {
      if ( osal_nv_read( ZCD_NV_APS_LINK_KEY_TABLE,
                (uint16)(sizeof(nvDeviceListHdr_t) + (x * sizeof(ZDSecMgrEntry_t))),
                      sizeof(ZDSecMgrEntry_t), &ZDSecMgrEntries[x] ) == ZSUCCESS )
      {
        ZDSecMgrEntries[x].lkd.apsmelkd.txFrmCntr += ( MAX_APS_FRAMECOUNTER_CHANGES + 1 );
      }
    }

    // Write the updated entry back to NV.
    ZDSecMgrWriteNV();

    return ZSuccess;
  }
  return ZFailure;
}

/******************************************************************************
 * @fn          ZDSecMgrAPSRemove
 *
 * @brief       Remove device from network.
 *
 * @param       nwkAddr - device's NWK address
 * @param       extAddr - device's Extended address
 * @param       parentAddr - parent's NWK address
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrAPSRemove( uint16 nwkAddr, uint8 *extAddr, uint16 parentAddr )
{
  ZDSecMgrDevice_t device;

  if ( ( nwkAddr == INVALID_NODE_ADDR ) ||
       ( extAddr == NULL )              ||
       ( parentAddr == INVALID_NODE_ADDR ) )
  {
    return ( ZFailure );
  }

  device.nwkAddr = nwkAddr;
  device.extAddr = extAddr;
  device.parentAddr = parentAddr;

  // remove device
  ZDSecMgrDeviceRemove( &device );

  return ( ZSuccess );
}

/******************************************************************************
******************************************************************************/

