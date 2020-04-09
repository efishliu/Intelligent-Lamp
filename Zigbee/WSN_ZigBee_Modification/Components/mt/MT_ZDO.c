/**************************************************************************************************
  Filename:       MT_ZDO.c
  Revised:        $Date: 2009-01-05 16:58:00 -0800 (Mon, 05 Jan 2009) $
  Revision:       $Revision: 18682 $

  Description:    MonitorTest functions for the ZDO layer.


  Copyright 2004-2007 Texas Instruments Incorporated. All rights reserved.

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

#ifdef MT_ZDO_FUNC

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "ZComDef.h"
#include "OSAL.h"
#include "MT.h"
#include "MT_ZDO.h"
#include "APSMEDE.h"
#include "ZDConfig.h"
#include "ZDProfile.h"
#include "ZDObject.h"
#include "ZDApp.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

#include "nwk_util.h"

/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/
#define MT_ZDO_END_DEVICE_ANNCE_IND_LEN   0x0D
#define MT_ZDO_ADDR_RSP_LEN               0x0D
#define MT_ZDO_BIND_UNBIND_RSP_LEN        0x03

#define MTZDO_RESPONSE_BUFFER_LEN   100

#define MTZDO_MAX_MATCH_CLUSTERS    16
#define MTZDO_MAX_ED_BIND_CLUSTERS  15

/**************************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************************/
uint32 _zdoCallbackSub;

/**************************************************************************************************
 * LOCAL VARIABLES
 **************************************************************************************************/
uint8 mtzdoResponseBuffer[MTZDO_RESPONSE_BUFFER_LEN];

/**************************************************************************************************
 * LOCAL FUNCTIONS
 **************************************************************************************************/
#if defined (MT_ZDO_FUNC)
void MT_ZdoNWKAddressRequest(uint8 *pBuf);
void MT_ZdoIEEEAddrRequest(uint8 *pBuf);
void MT_ZdoNodeDescRequest(uint8 *pBuf);
void MT_ZdoPowerDescRequest(uint8 *pBuf);
void MT_ZdoSimpleDescRequest(uint8 *pBuf);
void MT_ZdoActiveEpRequest(uint8 *pBuf);
void MT_ZdoMatchDescRequest(uint8 *pBuf);
void MT_ZdoComplexDescRequest(uint8 *pBuf);
void MT_ZdoUserDescRequest(uint8 *pBuf);
void MT_ZdoEndDevAnnce(uint8 *pBuf);
void MT_ZdoUserDescSet(uint8 *pBuf);
void MT_ZdoServiceDiscRequest(uint8 *pBuf);
void MT_ZdoEndDevBindRequest(uint8 *pBuf);
void MT_ZdoBindRequest(uint8 *pBuf);
void MT_ZdoUnbindRequest(uint8 *pBuf);
void MT_ZdoMgmtNwkDiscRequest(uint8 *pBuf);
void MT_ZdoStartupFromApp(uint8 *pBuf);
#if defined (MT_ZDO_MGMT)
void MT_ZdoMgmtLqiRequest(uint8 *pBuf);
void MT_ZdoMgmtRtgRequest(uint8 *pBuf);
void MT_ZdoMgmtBindRequest(uint8 *pBuf);
void MT_ZdoMgmtLeaveRequest(uint8 *pBuf);
void MT_ZdoMgmtDirectJoinRequest(uint8 *pBuf);
void MT_ZdoMgmtPermitJoinRequest(uint8 *pBuf);
void MT_ZdoMgmtNwkUpdateRequest(uint8 *pBuf);
#endif /* MT_ZDO_MGMT */
#endif /* MT_ZDO_FUNC */

#if defined (MT_ZDO_CB_FUNC)
uint8 MT_ZdoHandleExceptions( afIncomingMSGPacket_t *pData, zdoIncomingMsg_t *inMsg );
void MT_ZdoAddrRspCB( ZDO_NwkIEEEAddrResp_t *pMsg, uint16 clusterID );
void MT_ZdoEndDevAnnceCB( ZDO_DeviceAnnce_t *pMsg, uint16 srcAddr );
void MT_ZdoBindUnbindRspCB( uint16 clusterID, uint16 srcAddr, uint8 status );

/* ZDO cluster ID to MT response command ID lookup */
static const uint8 CODE mtZdoCluster2Rsp[4][7] =
{
  {
    MT_ZDO_NWK_ADDR_RSP,         /* NWK_addr_req */
    MT_ZDO_IEEE_ADDR_RSP,        /* IEEE_addr_req */
    MT_ZDO_NODE_DESC_RSP,        /* Node_Desc_req */
    MT_ZDO_POWER_DESC_RSP,       /* Power_Desc_req */
    MT_ZDO_SIMPLE_DESC_RSP,      /* Simple_Desc_req */
    MT_ZDO_ACTIVE_EP_RSP,        /* Active_EP_req */
    MT_ZDO_MATCH_DESC_RSP        /* Match_Desc_req */
  },
  {
    MT_ZDO_COMPLEX_DESC_RSP,     /* Complex_Desc_req */
    MT_ZDO_USER_DESC_RSP,        /* User_Desc_req */
    0,                           /* Discovery_Cache_req */
    0,                           /* End_Device_annce */
    MT_ZDO_USER_DESC_CONF,       /* User_Desc_set */
    MT_ZDO_SERVER_DISC_RSP,      /* Server_Discovery_req */
    0
  },
  {
    MT_ZDO_END_DEVICE_BIND_RSP,  /* End_Device_Bind_req */
    MT_ZDO_BIND_RSP,             /* Bind_req */
    MT_ZDO_UNBIND_RSP,           /* Unbind_req */
    0,
    0,
    0,
    MT_ZDO_STATUS_ERROR_RSP      /* default error status msg */
  },
  {
    MT_ZDO_MGMT_NWK_DISC_RSP,    /* Mgmt_NWK_Disc_req */
    MT_ZDO_MGMT_LQI_RSP,         /* Mgmt_Lqi_req */
    MT_ZDO_MGMT_RTG_RSP,         /* Mgmt_Rtg_req */
    MT_ZDO_MGMT_BIND_RSP,        /* Mgmt_Bind_req */
    MT_ZDO_MGMT_LEAVE_RSP,       /* Mgmt_Leave_req */
    MT_ZDO_MGMT_DIRECT_JOIN_RSP, /* Mgmt_Direct_Join_req */
    MT_ZDO_MGMT_PERMIT_JOIN_RSP  /* Mgmt_Permit_Join_req */
  }
};
#endif /* MT_ZDO_CB_FUNC */

#if defined (MT_ZDO_FUNC)
/***************************************************************************************************
 * @fn      MT_ZdoCommandProcessing
 *
 * @brief
 *
 *   Process all the ZDO commands that are issued by test tool
 *
 * @param   pBuf - pointer to the msg buffer
 *
 *          | LEN  | CMD0  | CMD1  |  DATA  |
 *          |  1   |   1   |   1   |  0-255 |
 *
 * @return  status
 ***************************************************************************************************/
uint8 MT_ZdoCommandProcessing(uint8* pBuf)
{
  uint8 status = MT_RPC_SUCCESS;

  switch (pBuf[MT_RPC_POS_CMD1])
  {
#if defined ( ZDO_NWKADDR_REQUEST )
    case MT_ZDO_NWK_ADDR_REQ:
      MT_ZdoNWKAddressRequest(pBuf);
      break;
#endif

#if defined ( ZDO_IEEEADDR_REQUEST )
    case MT_ZDO_IEEE_ADDR_REQ:
      MT_ZdoIEEEAddrRequest(pBuf);
      break;
#endif

#if defined ( ZDO_NODEDESC_REQUEST )
    case MT_ZDO_NODE_DESC_REQ:
      MT_ZdoNodeDescRequest(pBuf);
      break;
#endif

#if defined ( ZDO_POWERDESC_REQUEST )
    case MT_ZDO_POWER_DESC_REQ:
      MT_ZdoPowerDescRequest(pBuf);
      break;
#endif

#if defined ( ZDO_SIMPLEDESC_REQUEST )
    case MT_ZDO_SIMPLE_DESC_REQ:
      MT_ZdoSimpleDescRequest(pBuf);
      break;
#endif

#if defined ( ZDO_ACTIVEEP_REQUEST )
    case MT_ZDO_ACTIVE_EP_REQ:
      MT_ZdoActiveEpRequest(pBuf);
      break;
#endif

#if defined ( ZDO_MATCH_REQUEST )
    case MT_ZDO_MATCH_DESC_REQ:
      MT_ZdoMatchDescRequest(pBuf);
      break;
#endif

#if defined ( ZDO_COMPLEXDESC_REQUEST )
    case MT_ZDO_COMPLEX_DESC_REQ:
      MT_ZdoComplexDescRequest(pBuf);
      break;
#endif

#if defined ( ZDO_USERDESC_REQUEST )
    case MT_ZDO_USER_DESC_REQ:
      MT_ZdoUserDescRequest(pBuf);
      break;
#endif

#if defined ( ZDO_ENDDEVICE_ANNCE )
    case MT_ZDO_END_DEV_ANNCE:
      MT_ZdoEndDevAnnce(pBuf);
      break;
#endif      

#if defined ( ZDO_USERDESCSET_REQUEST )
    case MT_ZDO_USER_DESC_SET:
      MT_ZdoUserDescSet(pBuf);
      break;
#endif

#if defined ( ZDO_SERVERDISC_REQUEST )
    case MT_ZDO_SERVICE_DISC_REQ:
      MT_ZdoServiceDiscRequest(pBuf);
      break;
#endif

#if defined ( ZDO_ENDDEVICEBIND_REQUEST )
    case MT_ZDO_END_DEV_BIND_REQ:
      MT_ZdoEndDevBindRequest(pBuf);
      break;
#endif

#if defined ( ZDO_BIND_UNBIND_REQUEST )
    case MT_ZDO_BIND_REQ:
      MT_ZdoBindRequest(pBuf);
      break;
#endif

#if defined ( ZDO_BIND_UNBIND_REQUEST )
    case MT_ZDO_UNBIND_REQ:
      MT_ZdoUnbindRequest(pBuf);
      break;
#endif

#if defined ( ZDO_NETWORKSTART_REQUEST )
    case MT_ZDO_STARTUP_FROM_APP:
      MT_ZdoStartupFromApp(pBuf);
      break;
#endif

#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
    case MT_ZDO_MGMT_NWKDISC_REQ:
      MT_ZdoMgmtNwkDiscRequest(pBuf);
      break;
#endif

#if defined ( ZDO_MGMT_LQI_REQUEST )
    case MT_ZDO_MGMT_LQI_REQ:
      MT_ZdoMgmtLqiRequest(pBuf);
      break;
#endif

#if defined ( ZDO_MGMT_RTG_REQUEST )
    case MT_ZDO_MGMT_RTG_REQ:
      MT_ZdoMgmtRtgRequest(pBuf);
      break;
#endif

#if defined ( ZDO_MGMT_BIND_REQUEST )
    case MT_ZDO_MGMT_BIND_REQ:
      MT_ZdoMgmtBindRequest(pBuf);
      break;
#endif

#if defined ( ZDO_MGMT_LEAVE_REQUEST )
    case MT_ZDO_MGMT_LEAVE_REQ:
      MT_ZdoMgmtLeaveRequest(pBuf);
      break;
#endif

#if defined ( ZDO_MGMT_JOINDIRECT_REQUEST )
    case MT_ZDO_MGMT_DIRECT_JOIN_REQ:
      MT_ZdoMgmtDirectJoinRequest(pBuf);
      break;
#endif

#if defined ( ZDO_MGMT_PERMIT_JOIN_REQUEST )
    case MT_ZDO_MGMT_PERMIT_JOIN_REQ:
      MT_ZdoMgmtPermitJoinRequest(pBuf);
      break;
#endif

#if defined ( ZDO_MGMT_NWKUPDATE_REQUEST )
    case MT_ZDO_MGMT_NWK_UPDATE_REQ:
      MT_ZdoMgmtNwkUpdateRequest(pBuf);
      break;
#endif 

    default:
      status = MT_RPC_ERR_COMMAND_ID;
      break;
  }

  return status;
}

/***************************************************************************************************
 * @fn      MT_ZdoNwkAddrReq
 *
 * @brief   Handle a nwk address request.
 *
 * @param   pData  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoNWKAddressRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  uint8 reqType;
  uint8 startIndex;
  uint8 *pExtAddr;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* parse parameters */
  pExtAddr = pBuf;
  pBuf += Z_EXTADDR_LEN;

  /* Request type */
  reqType = *pBuf++;

  /* Start index */
  startIndex = *pBuf;

  retValue = (uint8)ZDP_NwkAddrReq(pExtAddr, reqType, startIndex, 0);

  /* Build and send back the response */
  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoIEEEAddrRequest
 *
 * @brief   Handle a IEEE address request.
 *
 * @param   pData  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoIEEEAddrRequest (uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  uint16 shortAddr;
  uint8 reqType;
  uint8 startIndex;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  shortAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += 2;

  /* request type */
  reqType = *pBuf++;

  /* start index */
  startIndex = *pBuf;

  retValue = (uint8)ZDP_IEEEAddrReq(shortAddr, reqType, startIndex, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoNodeDescRequest
 *
 * @brief   Handle a Node Descriptor request.
 *
 * @param   pData  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoNodeDescRequest (uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint16 shortAddr;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Destination address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Network address of interest */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  retValue = (uint8)ZDP_NodeDescReq( &destAddr, shortAddr, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoPowerDescRequest
 *
 * @brief   Handle a Power Descriptor request.
 *
 * @param   pData  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoPowerDescRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint16 shortAddr;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Network address of interest */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  retValue = (uint8)ZDP_PowerDescReq( &destAddr, shortAddr, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoSimpleDescRequest
 *
 * @brief   Handle a Simple Descriptor request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoSimpleDescRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  uint8 epInt;
  zAddrType_t destAddr;
  uint16 shortAddr;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Network address of interest */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* endpoint/interface */
  epInt = *pBuf++;

  retValue = (uint8)ZDP_SimpleDescReq( &destAddr, shortAddr, epInt, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoSimpleDescRequest
 *
 * @brief   Handle a Active EP request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoActiveEpRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint16 shortAddr;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Network address of interest */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  retValue = (uint8)ZDP_ActiveEPReq( &destAddr, shortAddr, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoMatchDescRequest
 *
 * @brief   Handle a Match Descriptor request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoMatchDescRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue = 0;
  uint8 i, numInClusters, numOutClusters;
  uint16 profileId;
  zAddrType_t destAddr;
  uint16 shortAddr;
  uint16 inClusters[MTZDO_MAX_MATCH_CLUSTERS], outClusters[MTZDO_MAX_MATCH_CLUSTERS];

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Network address of interest */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Profile ID */
  profileId = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* NumInClusters */
  numInClusters = *pBuf++;
  if ( numInClusters <= MTZDO_MAX_MATCH_CLUSTERS )
  {
    /* IN clusters */
    for ( i = 0; i < numInClusters; i++ )
    {
      inClusters[i] = BUILD_UINT16( pBuf[0], pBuf[1]);
      pBuf += 2;
    }
  }
  else
  {
    retValue = ZDP_INVALID_REQTYPE;
  }

  /* NumOutClusters */
  numOutClusters = *pBuf++;
  if ( numOutClusters <= MTZDO_MAX_MATCH_CLUSTERS )
  {
    /* OUT Clusters */
    for ( i = 0; i < numOutClusters; i++ )
    {
      outClusters[i] = BUILD_UINT16( pBuf[0], pBuf[1]);
      pBuf += 2;
    }
  }
  else
  {
    retValue = ZDP_INVALID_REQTYPE;
  }

  if ( retValue == 0 )
  {
    retValue = (uint8)ZDP_MatchDescReq( &destAddr, shortAddr, profileId, numInClusters,
                                       inClusters, numOutClusters, outClusters, 0);
  }

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoComplexDescRequest
 *
 * @brief   Handle a Complex Descriptor request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoComplexDescRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint16 shortAddr;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Network address of interest */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  retValue = (uint8)ZDP_ComplexDescReq( &destAddr, shortAddr, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoUserDescRequest
 *
 * @brief   Handle a User Descriptor request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoUserDescRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint16 shortAddr;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1]);
  pBuf += 2;

  /* Network address of interest */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1]);
  pBuf += 2;

  retValue = (uint8)ZDP_UserDescReq( &destAddr, shortAddr, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoEndDevAnnce
 *
 * @brief   Handle a End Device Announce Descriptor request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoEndDevAnnce(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  uint16 shortAddr;
  uint8 *pIEEEAddr;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* network address */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* extended address */
  pIEEEAddr = pBuf;
  pBuf += Z_EXTADDR_LEN;

  retValue = (uint8)ZDP_DeviceAnnce( shortAddr, pIEEEAddr, *pBuf, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoUserDescSet
 *
 * @brief   Handle a User Descriptor Set.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoUserDescSet(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint16 shortAddr;
  UserDescriptorFormat_t userDesc;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Network address of interest */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* User descriptor */
  userDesc.len = *pBuf++;
  osal_memcpy( userDesc.desc, pBuf, userDesc.len );
  pBuf += 16;

  retValue = (uint8)ZDP_UserDescSet( &destAddr, shortAddr, &userDesc, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoServiceDiscRequest
 *
 * @brief   Handle a Server Discovery request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoServiceDiscRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  uint16 serviceMask;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Service Mask */
  serviceMask = BUILD_UINT16( pBuf[0], pBuf[1]);
  pBuf += 2;

  retValue = (uint8)ZDP_ServerDiscReq( serviceMask, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoEndDevBindRequest
 *
 * @brief   Handle a End Device Bind request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoEndDevBindRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue = 0;
  uint8 i, epInt, numInClusters, numOutClusters;
  zAddrType_t destAddr;
  uint16 shortAddr;
  uint16 profileID, inClusters[MTZDO_MAX_ED_BIND_CLUSTERS], outClusters[MTZDO_MAX_ED_BIND_CLUSTERS];

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Local coordinator of the binding */
  shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;
  
  /* For now, skip past the extended address */
  pBuf += Z_EXTADDR_LEN;

  /* Endpoint */
  epInt = *pBuf++;

  /* Profile ID */
  profileID = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* NumInClusters */
  numInClusters = *pBuf++;
  if ( numInClusters <= MTZDO_MAX_ED_BIND_CLUSTERS )
  {
    for ( i = 0; i < numInClusters; i++ )
    {
      inClusters[i] = BUILD_UINT16(pBuf[0], pBuf[1]);
      pBuf += 2;
    }
  }
  else
    retValue = ZDP_INVALID_REQTYPE;

  /* NumOutClusters */
  numOutClusters = *pBuf++;
  if ( numOutClusters <= MTZDO_MAX_ED_BIND_CLUSTERS )
  {
    for ( i = 0; i < numOutClusters; i++ )
    {
      outClusters[i] = BUILD_UINT16(pBuf[0], pBuf[1]);
      pBuf += 2;
    }
  }
  else
    retValue = ZDP_INVALID_REQTYPE;
  
  if ( retValue == 0 )
  {
    retValue = (uint8)ZDP_EndDeviceBindReq( &destAddr, shortAddr, epInt, profileID,
                                          numInClusters, inClusters, numOutClusters, outClusters, 0);
  }

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoBindRequest
 *
 * @brief   Handle a Bind request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoBindRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr, devAddr;
  uint8 *pSrcAddr, *ptr;
  uint8 srcEPInt, dstEPInt;
  uint16 clusterID;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* SrcAddress */
  pSrcAddr = pBuf;
  pBuf += Z_EXTADDR_LEN;

  /* SrcEPInt */
  srcEPInt = *pBuf++;

  /* ClusterID */
  clusterID = BUILD_UINT16( pBuf[0], pBuf[1]);
  pBuf += 2;

  /* Destination Address mode */
  devAddr.addrMode = *pBuf++;

  /* Destination Address */
  if ( devAddr.addrMode == Addr64Bit )
  {
    ptr = pBuf;
    osal_cpyExtAddr( devAddr.addr.extAddr, ptr );
  }
  else
  {
    devAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  }
  /* The short address occupies LSB two bytes */
  pBuf += Z_EXTADDR_LEN;

  /* DstEPInt */
  dstEPInt = *pBuf;

  retValue = (uint8)ZDP_BindReq( &destAddr, pSrcAddr, srcEPInt, clusterID, &devAddr, dstEPInt, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoUnbindRequest
 *
 * @brief   Handle a Unbind request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoUnbindRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr, devAddr;
  uint8 *pSrcAddr, *ptr;
  uint8 srcEPInt, dstEPInt;
  uint16 clusterID;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* SrcAddress */
  pSrcAddr = pBuf;
  pBuf += Z_EXTADDR_LEN;

  /* SrcEPInt */
  srcEPInt = *pBuf++;

  /* ClusterID */
  clusterID = BUILD_UINT16( pBuf[0], pBuf[1]);
  pBuf += 2;

  /* Destination Address mode */
  devAddr.addrMode = *pBuf++;

  /* Destination Address */
  if ( devAddr.addrMode == Addr64Bit )
  {
    ptr = pBuf;
    osal_cpyExtAddr( devAddr.addr.extAddr, ptr );
  }
  else
  {
    devAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  }
  /* The short address occupies LSB two bytes */
  pBuf += Z_EXTADDR_LEN;

  /* dstEPInt */
  dstEPInt = *pBuf;

  retValue = (uint8)ZDP_UnbindReq( &destAddr, pSrcAddr, srcEPInt, clusterID, &devAddr, dstEPInt, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoStartupFromApp
 *
 * @brief   Handle a Startup from App request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoStartupFromApp(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  retValue = ZDOInitDevice(100);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

#if defined (MT_ZDO_MGMT)
/***************************************************************************************************
 * @fn      MT_ZdoMgmtNwkDiscRequest
 *
 * @brief   Handle a Mgmt Nwk Discovery request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoMgmtNwkDiscRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint32 scanChannels;
  uint8 scanDuration, startIndex;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Scan Channels */
  scanChannels = BUILD_UINT32( pBuf[0], pBuf[1], pBuf[2], pBuf[3] );
  pBuf += 4;

  /* Scan Duration */
  scanDuration = *pBuf++;

  /* Start Index */
  startIndex = *pBuf;

  retValue = (uint8)ZDP_MgmtNwkDiscReq( &destAddr, scanChannels, scanDuration, startIndex, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoMgmtLqiRequest
 *
 * @brief   Handle a Mgmt Lqi request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoMgmtLqiRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint8 startIndex;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Start Index */
  startIndex = *pBuf;

  retValue = (uint8)ZDP_MgmtLqiReq( &destAddr, startIndex, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoMgmtRtgRequest
 *
 * @brief   Handle a Mgmt Rtg request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoMgmtRtgRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint8 startIndex;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev Address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1]);
  pBuf += 2;

  /* Start Index */
  startIndex = *pBuf;

  retValue = (byte)ZDP_MgmtRtgReq( &destAddr, startIndex, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoMgmtBindRequest
 *
 * @brief   Handle a Mgmt Bind request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoMgmtBindRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint8 startIndex;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Dev Address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Start Index */
  startIndex = *pBuf;

  retValue = (uint8)ZDP_MgmtBindReq( &destAddr, startIndex, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoMgmtLeaveRequest
 *
 * @brief   Handle a Mgmt Leave request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoMgmtLeaveRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint8 *pIEEEAddr;
  uint8 removeChildren, rejoin;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Destination Address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* IEEE address */
  pIEEEAddr = pBuf;
  pBuf += Z_EXTADDR_LEN;

  /* Remove Children */
  removeChildren = *pBuf++;

  /* Rejoin */
  rejoin = *pBuf;

  retValue = (byte)ZDP_MgmtLeaveReq( &destAddr, pIEEEAddr, removeChildren, rejoin, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}


/***************************************************************************************************
 * @fn      MT_ZdoMgmtDirectJoinRequest
 *
 * @brief   Handle a Mgmt Direct Join request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoMgmtDirectJoinRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint8 *deviceAddr;
  uint8 capInfo;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Destination Address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Device Address */
  deviceAddr = pBuf;
  pBuf += Z_EXTADDR_LEN;

  /* Capability information */
  capInfo = *pBuf;

  retValue = (uint8)ZDP_MgmtDirectJoinReq( &destAddr, deviceAddr, capInfo, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoMgmtPermitJoinRequest
 *
 * @brief   Handle a Mgmt Permit Join request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoMgmtPermitJoinRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint8 duration, tcSignificance;

  /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Destination Address */
  destAddr.addrMode = Addr16Bit;
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Duration */
  duration = *pBuf++;

  /* Trust center significance */
  tcSignificance = *pBuf;

  retValue = (byte)ZDP_MgmtPermitJoinReq( &destAddr, duration, tcSignificance, 0);

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}

/***************************************************************************************************
 * @fn      MT_ZdoMgmtNwkUpdateRequest
 *
 * @brief   Handle a Mgmt Nwk Update request.
 *
 * @param   pBuf  - MT message data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ZdoMgmtNwkUpdateRequest(uint8 *pBuf)
{
  uint8 cmdId;
  uint8 retValue;
  zAddrType_t destAddr;
  uint32 channelMask;
  uint8 scanDuration, scanCount;
  uint16 nwkManagerAddr;

    /* parse header */
  cmdId = pBuf[MT_RPC_POS_CMD1];
  pBuf += MT_RPC_FRAME_HDR_SZ;

  /* Destination address */
  destAddr.addr.shortAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;

  /* Destination address mode */
  destAddr.addrMode = *pBuf++;

  channelMask = BUILD_UINT32( pBuf[0], pBuf[1], pBuf[2], pBuf[3]);
  pBuf += 4;

  /* Scan duration */
  scanDuration = *pBuf++;

  /* Scan count */
  scanCount = *pBuf++;

  /* NWK manager address */
  nwkManagerAddr = BUILD_UINT16( pBuf[0], pBuf[1] );

  /* Send the Management Network Update request */
  retValue = (uint8)ZDP_MgmtNwkUpdateReq( &destAddr, channelMask, scanDuration,
                                          scanCount, _NIB.nwkUpdateId+1, nwkManagerAddr );

  /*
    Since we don't recevied our own broadcast messages, we should
    send a unicast copy of the message to ourself.
  */
  if ( destAddr.addrMode == AddrBroadcast )
  {
    destAddr.addrMode = Addr16Bit;
    destAddr.addr.shortAddr = _NIB.nwkDevAddress;
    retValue = (uint8) ZDP_MgmtNwkUpdateReq( &destAddr, channelMask, scanDuration,
                                             scanCount, _NIB.nwkUpdateId+1, nwkManagerAddr );
  }

  MT_BuildAndSendZToolResponse(((uint8)MT_RPC_CMD_SRSP | (uint8)MT_RPC_SYS_ZDO), cmdId, 1, &retValue);
}
#endif /* MT_ZDO_MGMT */

#endif /* MT_ZDO_FUNC */


/***************************************************************************************************
 * Callback handling function
 ***************************************************************************************************/

#if defined (MT_ZDO_CB_FUNC)
/***************************************************************************************************
 * @fn     MT_ZdoDirectCB()
 *
 * @brief  ZDO direct callback.  Build an MT message directly from the
 *         over-the-air ZDO message.
 *
 * @param  pData - Incoming AF frame.
 *
 * @return  none
 ***************************************************************************************************/
void MT_ZdoDirectCB( afIncomingMSGPacket_t *pData,  zdoIncomingMsg_t *inMsg )
{
  uint8 dataLen;
  uint8 msgLen;
  uint8 *pBuf;
  uint8 *p;
  uint8 id;
  
  // Is the message an exception or not a response?
  if ( ((pData->clusterId & ZDO_RESPONSE_BIT) == 0) 
                      || MT_ZdoHandleExceptions( pData, inMsg ) )
  {
    // Handled somewhere else or not needed
    return;
  }

  /* map cluster ID to MT message */
  id = (uint8) pData->clusterId;
  id = mtZdoCluster2Rsp[id >> 4][id & 0x0F];

  /* ZDO data starts after one-byte sequence number */
  dataLen = pData->cmd.DataLength - 1;

  /* msg buffer length includes two bytes for srcAddr */
  msgLen = dataLen + sizeof(uint16);

  /* get buffer */
  if ((p = pBuf = MT_TransportAlloc(((uint8)MT_RPC_CMD_AREQ |(uint8)MT_RPC_SYS_ZDO), msgLen)) != NULL)
  {
    /* build header */
    *p++ = msgLen;
    *p++ = (uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_ZDO;
    *p++ = id;

    /* build srcAddr */
    *p++ = LO_UINT16(pData->srcAddr.addr.shortAddr);
    *p++ = HI_UINT16(pData->srcAddr.addr.shortAddr);

    /* copy ZDO data, skipping one-byte sequence number */
    osal_memcpy(p, (pData->cmd.Data + 1), dataLen);

    /* send it */
    MT_TransportSend(pBuf);
  }
}

/***************************************************************************************************
 * @fn     MT_ZdoHandleExceptions()
 *
 * @brief  Handles all messages that are an expection to the generic MT ZDO Response.
 *
 * @param  pData - Incoming AF frame.
 *
 * @return  TRUE if handled by this function, FALSE if not
 ***************************************************************************************************/
uint8 MT_ZdoHandleExceptions( afIncomingMSGPacket_t *pData, zdoIncomingMsg_t *inMsg )
{
  uint8 ret = TRUE;
  ZDO_NwkIEEEAddrResp_t *nwkRsp = NULL;
  ZDO_DeviceAnnce_t devAnnce;
  uint8 doDefault = FALSE;
  
  switch ( inMsg->clusterID )
  {
    case NWK_addr_rsp:
    case IEEE_addr_rsp:
      nwkRsp = ZDO_ParseAddrRsp( inMsg );
      MT_ZdoAddrRspCB( nwkRsp, inMsg->clusterID );
      if ( nwkRsp )
        osal_mem_free( nwkRsp );
      break;
      
    case Device_annce:
      ZDO_ParseDeviceAnnce( inMsg, &devAnnce );
      MT_ZdoEndDevAnnceCB( &devAnnce, inMsg->srcAddr.addr.shortAddr );
      break;
      
    case Simple_Desc_rsp:
      if ( pData->cmd.DataLength > 5 )
        ret = FALSE;
      else
        doDefault = TRUE;        
      break;  
      
    default:
      ret = FALSE;
      break;
  }
  
  if ( doDefault )
  {
    ret = FALSE;
    pData->clusterId = 0x26;
    pData->cmd.DataLength = 2;
  }
  
  return ( ret );
}

/***************************************************************************************************
 * @fn      MT_ZdoAddrRspCB
 *
 * @brief   Handle IEEE or nwk address response OSAL message from ZDO.
 *
 * @param   pMsg  - Message data
 *
 * @return  void
 */
void MT_ZdoAddrRspCB( ZDO_NwkIEEEAddrResp_t *pMsg, uint16 clusterID )
{
  uint8   listLen;
  uint8   msgLen;
  uint8   *pBuf;
  uint8   *p;

  /* both ZDO_NwkAddrResp_t and ZDO_IEEEAddrResp_t must be the same */

  /* get length, sanity check length */
  listLen = pMsg->numAssocDevs;
  
  /* calculate msg length */
  msgLen = MT_ZDO_ADDR_RSP_LEN + (listLen * sizeof(uint16));

  /* get buffer */
  if ((p = pBuf = MT_TransportAlloc(((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_ZDO), msgLen)) != NULL)
  {
    /* build header */
    *p++ = msgLen;
    *p++ = (uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_ZDO;
    *p++ = (clusterID == IEEE_addr_rsp) ?
           MT_ZDO_IEEE_ADDR_RSP : MT_ZDO_NWK_ADDR_RSP;

    /* build msg parameters */

    *p++ = pMsg->status;

    osal_cpyExtAddr(p, pMsg->extAddr);
    p += Z_EXTADDR_LEN;

    *p++ = LO_UINT16(pMsg->nwkAddr);
    *p++ = HI_UINT16(pMsg->nwkAddr);

    *p++ = pMsg->startIndex;

    *p++ = listLen;

    MT_Word2Buf(p, pMsg->devList, listLen);

    /* send it */
    MT_TransportSend(pBuf);
  }
}

/***************************************************************************************************
 * @fn      MT_ZdoEndDevAnnceCB
 *
 * @brief   Handle end device announce OSAL message from ZDO.
 *
 * @param   pMsg  - Message data
 *
 * @return  void
 */
void MT_ZdoEndDevAnnceCB( ZDO_DeviceAnnce_t *pMsg, uint16 srcAddr )
{
  uint8 *pBuf;
  uint8 *p;

  if ((p = pBuf = MT_TransportAlloc(((uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_ZDO),
                    MT_ZDO_END_DEVICE_ANNCE_IND_LEN)) != NULL)
  {
    *p++ = MT_ZDO_END_DEVICE_ANNCE_IND_LEN;
    *p++ = (uint8)MT_RPC_CMD_AREQ | (uint8)MT_RPC_SYS_ZDO;
    *p++ = MT_ZDO_END_DEVICE_ANNCE_IND;

    *p++ = LO_UINT16(srcAddr);
    *p++ = HI_UINT16(srcAddr);

    *p++ = LO_UINT16(pMsg->nwkAddr);
    *p++ = HI_UINT16(pMsg->nwkAddr);

    osal_cpyExtAddr(p, pMsg->extAddr);
    p += Z_EXTADDR_LEN;

    *p = pMsg->capabilities;

    MT_TransportSend(pBuf);
  }
}
#endif // MT_ZDO_CB_FUNC
/***************************************************************************************************
***************************************************************************************************/

#endif   /*ZDO Command Processing in MT*/
