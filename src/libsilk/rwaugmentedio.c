/*
** Copyright (C) 2005-2017 by Carnegie Mellon University.
**
** @OPENSOURCE_LICENSE_START@
** See license information in ../../LICENSE.txt
** @OPENSOURCE_LICENSE_END@
*/

/*
** rwaugmentedio.c
**
** Suresh L Konda
**      routines to do io stuff with augmented records.
*/

#include <silk/silk.h>

RCSIDENT("$SiLK: rwaugmentedio.c 275df62a2e41 2017-01-05 17:30:40Z mthomas $");

/* #define RWPACK_BYTES_PACKETS          1 */
#define RWPACK_FLAGS_TIMES_VOLUMES    1
#define RWPACK_PROTO_FLAGS            1
/* #define RWPACK_SBB_PEF                1 */
#define RWPACK_TIME_BYTES_PKTS_FLAGS  1
#define RWPACK_TIMES_FLAGS_PROTO      1
#include "skstream_priv.h"
#include "rwpack.c"


/* Version to use when SK_RECORD_VERSION_ANY is specified */
#define DEFAULT_RECORD_VERSION 4


/* ********************************************************************* */

/*
**  RWAUGMENTED VERSION 5
**
**  in the following: EXPANDED == ((tcp_state & SK_TCPSTATE_EXPANDED) ? 1 : 0)
**
**    uint32_t      rflag_stime;     //  0- 3
**    // uint32_t     rest_flags: 8; //        is_tcp==0: Empty; else
**                                   //          EXPANDED==0:Empty
**                                   //          EXPANDED==1:TCPflags/!1st pkt
**    // uint32_t     is_tcp    : 1; //        1 if FLOW is TCP; 0 otherwise
**    // uint32_t     unused    : 1; //        Reserved
**    // uint32_t     stime     :22; //        Start time:msec offset from hour
**
**    uint8_t       proto_iflags;    //  4     is_tcp==0: Protocol; else:
**                                   //          EXPANDED==0:TCPflags/ALL pkts
**                                   //          EXPANDED==1:TCPflags/1st pkt
**    uint8_t       tcp_state;       //  5     TCP state machine info
**    uint16_t      application;     //  6- 7  Indication of type of traffic
**
**    uint16_t      sPort;           //  8- 9  Source port
**    uint16_t      dPort;           // 10-11  Destination port
**
**    uint32_t      elapsed;         // 12-15  Duration of the flow
**
**    uint32_t      pkts;            // 16-19  Count of packets
**    uint32_t      bytes;           // 20-23  Count of bytes
**
**    uint32_t      sIP;             // 24-27  Source IP
**    uint32_t      dIP;             // 28-31  Destination IP
**
**
**  32 bytes on disk.
*/

#define RECLEN_RWAUGMENTED_V5 32


/*
 *    Byte swap the RWAUGMENTED v5 record 'ar' in place.
 */
#define augmentedioRecordSwap_V5(ar)                            \
    {                                                           \
        SWAP_DATA32((ar) +  0);   /* rflag_stime */             \
        /* two single bytes (4)proto_iflags, (5)tcp_state */    \
        SWAP_DATA16((ar) +  6);   /* application */             \
        SWAP_DATA16((ar) +  8);   /* sPort */                   \
        SWAP_DATA16((ar) + 10);   /* dPort */                   \
        SWAP_DATA32((ar) + 12);   /* elapsed */                 \
        SWAP_DATA32((ar) + 16);   /* pkts */                    \
        SWAP_DATA32((ar) + 20);   /* bytes */                   \
        SWAP_DATA32((ar) + 24);   /* sIP */                     \
        SWAP_DATA32((ar) + 28);   /* dIP */                     \
    }


/*
 *  Unpack the array of bytes 'ar' into a record 'rwrec'
 */
static int
augmentedioRecordUnpack_V5(
    skstream_t         *stream,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    /* swap if required */
    if (stream->swapFlag) {
        augmentedioRecordSwap_V5(ar);
    }

    /* Start time, TCP flags, Protocol, TCP State */
    rwpackUnpackTimesFlagsProto(rwrec, ar, stream->hdr_starttime);

    /* application */
    rwRecMemSetApplication(rwrec, &ar[ 6]);

    /* sPort, dPort */
    rwRecMemSetSPort(rwrec, &ar[ 8]);
    rwRecMemSetDPort(rwrec, &ar[10]);

    /* Elapsed */
    rwRecMemSetElapsed(rwrec, &ar[12]);

    /* packets, bytes */
    rwRecMemSetPkts(rwrec,  &ar[16]);
    rwRecMemSetBytes(rwrec, &ar[20]);

    /* sIP, dIP */
    rwRecMemSetSIPv4(rwrec, &ar[24]);
    rwRecMemSetDIPv4(rwrec, &ar[28]);

    /* sensor, flow_type from file name/header */
    rwRecSetSensor(rwrec, stream->hdr_sensor);
    rwRecSetFlowType(rwrec, stream->hdr_flowtype);

    return SKSTREAM_OK;
}


/*
 *  Pack the record 'rwrec' into an array of bytes 'ar'
 */
static int
augmentedioRecordPack_V5(
    skstream_t             *stream,
    const rwGenericRec_V5  *rwrec,
    uint8_t                *ar)
{
    int rv;

    /* Start time, TCP Flags, Protocol, TCP State */
    rv = rwpackPackTimesFlagsProto(rwrec, ar, stream->hdr_starttime);
    if (rv) {
        return rv;
    }

    /* application */
    rwRecMemGetApplication(rwrec, &ar[6]);

    /* sPort, dPort */
    rwRecMemGetSPort(rwrec, &ar[ 8]);
    rwRecMemGetDPort(rwrec, &ar[10]);

    /* Elapsed */
    rwRecMemGetElapsed(rwrec, &ar[12]);

    /* packets, bytes */
    rwRecMemGetPkts(rwrec,  &ar[16]);
    rwRecMemGetBytes(rwrec, &ar[20]);

    /* sIP, dIP */
    rwRecMemGetSIPv4(rwrec, &ar[24]);
    rwRecMemGetDIPv4(rwrec, &ar[28]);

    /* swap if required */
    if (stream->swapFlag) {
        augmentedioRecordSwap_V5(ar);
    }

    return SKSTREAM_OK;
}


/* ********************************************************************* */

/*
**  RWAUGMENTED VERSION 4
**
**  in the following: EXPANDED == ((tcp_state & SK_TCPSTATE_EXPANDED) ? 1 : 0)
**
**    uint32_t      stime_bb1;       //  0- 3
**    // uint32_t     stime     :22  //        Start time:msec offset from hour
**    // uint32_t     bPPkt1    :10; //        Whole bytes-per-packet (hi 10)
**
**    uint32_t      bb2_elapsed;     //  4- 7
**    // uint32_t     bPPkt2    : 4; //        Whole bytes-per-packet (low 4)
**    // uint32_t     bPPFrac   : 6; //        Fractional bytes-per-packet
**    // uint32_t     elapsed   :22; //        Duration of flow in msec
**
**    uint32_t      pro_flg_pkts;    //  8-11
**    // uint32_t     prot_flags: 8; //        is_tcp==0: IP protocol
**                                   //        is_tcp==1 &&
**                                   //          EXPANDED==0:TCPflags/All pkts
**                                   //          EXPANDED==1:TCPflags/1st pkt
**    // uint32_t     pflag     : 1; //        'pkts' requires multiplier?
**    // uint32_t     is_tcp    : 1; //        1 if flow is TCP; 0 otherwise
**    // uint32_t     padding   : 2; //
**    // uint32_t     pkts      :20; //        Count of packets
**
**    uint8_t       tcp_state;       // 12     TCP state machine info
**    uint8_t       rest_flags;      // 13     is_tcp==0: Flow's reported flags
**                                   //        is_tcp==1 &&
**                                   //          EXPANDED==0:Empty
**                                   //          EXPANDED==1:TCPflags/!1st pkt
**    uint16_t      application;     // 14-15  Type of traffic
**
**    uint16_t      sPort;           // 16-17  Source port
**    uint16_t      dPort;           // 18-19  Destination port
**
**    uint32_t      sIP;             // 20-23  Source IP
**    uint32_t      dIP;             // 24-27  Destination IP
**
**
**  28 bytes on disk.
*/

#define RECLEN_RWAUGMENTED_V4 28


/*
 *    Byte swap the RWAUGMENTED v4 record 'ar' in place.
 */
#define augmentedioRecordSwap_V4(ar)                            \
    {                                                           \
        SWAP_DATA32((ar) +  0);   /* stime_bb1 */               \
        SWAP_DATA32((ar) +  4);   /* bb2_elapsed */             \
        SWAP_DATA32((ar) +  8);   /* pro_flg_pkts */            \
        /* two single bytes (12)tcp_state, (13)rest_flags */    \
        SWAP_DATA16((ar) + 14);   /* application */             \
        SWAP_DATA16((ar) + 16);   /* sPort */                   \
        SWAP_DATA16((ar) + 18);   /* dPort */                   \
        SWAP_DATA32((ar) + 20);   /* sIP */                     \
        SWAP_DATA32((ar) + 24);   /* dIP */                     \
    }


/*
 *  Unpack the array of bytes 'ar' into a record 'rwrec'
 */
static int
augmentedioRecordUnpack_V4(
    skstream_t         *stream,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    /* swap if required */
    if (stream->swapFlag) {
        augmentedioRecordSwap_V4(ar);
    }

    /* sTime, elapsed, pkts, bytes, proto, tcp-flags, state, application */
    rwpackUnpackFlagsTimesVolumes(rwrec, ar, stream->hdr_starttime, 16, 0);

    /* sPort, dPort */
    rwRecMemSetSPort(rwrec, &ar[16]);
    rwRecMemSetDPort(rwrec, &ar[18]);

    /* sIP, dIP */
    rwRecMemSetSIPv4(rwrec, &ar[20]);
    rwRecMemSetDIPv4(rwrec, &ar[24]);

    /* sensor, flow_type from file name/header */
    rwRecSetSensor(rwrec, stream->hdr_sensor);
    rwRecSetFlowType(rwrec, stream->hdr_flowtype);

    return SKSTREAM_OK;
}


/*
 *  Pack the record 'rwrec' into an array of bytes 'ar'
 */
static int
augmentedioRecordPack_V4(
    skstream_t             *stream,
    const rwGenericRec_V5  *rwrec,
    uint8_t                *ar)
{
    int rv = SKSTREAM_OK; /* return value */

    /* sTime, elapsed, pkts, bytes, proto, tcp-flags, state, application */
    rv = rwpackPackFlagsTimesVolumes(ar, rwrec, stream->hdr_starttime, 16);
    if (rv) {
        return rv;
    }

    /* sPort, dPort */
    rwRecMemGetSPort(rwrec, &ar[16]);
    rwRecMemGetDPort(rwrec, &ar[18]);

    /* sIP, dIP */
    rwRecMemGetSIPv4(rwrec, &ar[20]);
    rwRecMemGetDIPv4(rwrec, &ar[24]);

    /* swap if required */
    if (stream->swapFlag) {
        augmentedioRecordSwap_V4(ar);
    }

    return SKSTREAM_OK;
}


/* ********************************************************************* */

/*
**  RWAUGMENTED VERSION 1
**  RWAUGMENTED VERSION 2
**  RWAUGMENTED VERSION 3
**
**    uint32_t      sIP;             //  0- 3  Source IP
**    uint32_t      dIP;             //  4- 7  Destination IP
**
**    uint16_t      sPort;           //  8- 9  Source port
**    uint16_t      dPort;           // 10-11  Destination port
**
**    uint32_t      pkts_stime;      // 12-15
**    // uint32_t     pkts      :20; //        Count of packets
**    // uint32_t     sTime     :12; //        Start time--offset from hour
**
**    uint32_t      bbe;             // 16-19
**    // uint32_t     bPPkt     :14; //        Whole bytes-per-packet
**    // uint32_t     bPPFrac   : 6; //        Fractional bytes-per-packet
**    // uint32_t     elapsed   :12; //        Duration of flow
**
**    uint32_t      msec_flags       // 20-23
**    // uint32_t     sTime_msec:10; //        Fractional sTime (millisec)
**    // uint32_t     elaps_msec:10; //        Fractional elapsed (millisec)
**    // uint32_t     pflag     : 1; //        'pkts' requires multiplier?
**    // uint32_t     is_tcp    : 1; //        1 if flow is TCP; 0 otherwise
**    // uint32_t     padding   : 2; //        padding/reserved
**    // uint32_t     prot_flags: 8; //        is_tcp==0: IP protocol
**                                   //        is_tcp==1 &&
**                                   //          EXPANDED==0:TCPflags/All pkts
**                                   //          EXPANDED==1:TCPflags/1st pkt
**
**    uint16_t      application;     // 24-25  Type of traffic
**
**    uint8_t       tcp_state;       // 26     TCP state machine info
**    uint8_t       rest_flags;      // 27     is_tcp==0: Flow's reported flags
**                                   //        is_tcp==1 &&
**                                   //          EXPANDED==0:Empty
**                                   //          EXPANDED==1:TCPflags/!1st pkt
**
**
**  28 bytes on disk.
*/

#define RECLEN_RWAUGMENTED_V1 28
#define RECLEN_RWAUGMENTED_V2 28
#define RECLEN_RWAUGMENTED_V3 28


/*
 *    Byte swap the RWAUGMENTED v1 record 'ar' in place.
 */
#define augmentedioRecordSwap_V1(ar)                            \
    {                                                           \
        SWAP_DATA32((ar) +  0);   /* sIP */                     \
        SWAP_DATA32((ar) +  4);   /* dIP */                     \
        SWAP_DATA16((ar) +  8);   /* sPort */                   \
        SWAP_DATA16((ar) + 10);   /* dPort */                   \
        SWAP_DATA32((ar) + 12);   /* pkts_stime */              \
        SWAP_DATA32((ar) + 16);   /* bbe */                     \
        SWAP_DATA32((ar) + 20);   /* msec_flags */              \
        SWAP_DATA16((ar) + 24);   /* application */             \
        /* Two single bytes: (26)tcp_state, (27)rest_flags */   \
    }


/*
 *  Unpack the array of bytes 'ar' into a record 'rwrec'
 */
static int
augmentedioRecordUnpack_V1(
    skstream_t         *stream,
    rwGenericRec_V5    *rwrec,
    uint8_t            *ar)
{
    uint32_t msec_flags;
    uint8_t is_tcp, prot_flags;

    /* swap if required */
    if (stream->swapFlag) {
        augmentedioRecordSwap_V1(ar);
    }

    /* sIP, dIP, sPort, dPort */
    rwRecMemSetSIPv4(rwrec, &ar[0]);
    rwRecMemSetDIPv4(rwrec, &ar[4]);
    rwRecMemSetSPort(rwrec, &ar[8]);
    rwRecMemSetDPort(rwrec, &ar[10]);

    /* msec times, proto or flags */
    memcpy(&msec_flags, &ar[20], 4);

    /* application */
    rwRecMemSetApplication(rwrec, &ar[24]);

    /* sTime, pkts, bytes, elapsed, proto, tcp-flags, bpp */
    rwpackUnpackTimeBytesPktsFlags(rwrec, stream->hdr_starttime,
                                   (uint32_t*)&ar[12], (uint32_t*)&ar[16],
                                   &msec_flags);

    /* extra TCP information */
    is_tcp = (uint8_t)GET_MASKED_BITS(msec_flags, 10, 1);
    prot_flags = (uint8_t)GET_MASKED_BITS(msec_flags, 0, 8);
    rwpackUnpackProtoFlags(rwrec, is_tcp, prot_flags, ar[26], ar[27]);

    /* sensor, flow_type from file name/header */
    rwRecSetSensor(rwrec, stream->hdr_sensor);
    rwRecSetFlowType(rwrec, stream->hdr_flowtype);

    return SKSTREAM_OK;
}


/*
 *  Pack the record 'rwrec' into an array of bytes 'ar'
 */
static int
augmentedioRecordPack_V1(
    skstream_t             *stream,
    const rwGenericRec_V5  *rwrec,
    uint8_t                *ar)
{
    int rv = SKSTREAM_OK; /* return value */
    uint32_t msec_flags;
    uint8_t is_tcp, prot_flags;

    /* sTime, pkts, bytes, elapsed, proto, tcp-flags, bpp */
    rv = rwpackPackTimeBytesPktsFlags((uint32_t*)&ar[12], (uint32_t*)&ar[16],
                                      &msec_flags,
                                      rwrec, stream->hdr_starttime);
    if (rv) {
        return rv;
    }

    rwpackPackProtoFlags(&is_tcp, &prot_flags, &ar[26], &ar[27], rwrec);

    /* msec_flags: sTime_msec:10; elaps_msec:10; pflag:1;
     *             is_tcp:1; pad:2; prot_flags:8; */
    /* overwrite the least significant 11 bits */
    msec_flags = ((msec_flags & (MASKARRAY_21 << 11))
                  | (is_tcp ? (1 << 10) : 0)
                  | prot_flags);

    /* sIP, dIP, sPort, dPort */
    rwRecMemGetSIPv4(rwrec, &ar[0]);
    rwRecMemGetDIPv4(rwrec, &ar[4]);
    rwRecMemGetSPort(rwrec, &ar[8]);
    rwRecMemGetDPort(rwrec, &ar[10]);

    /* msec_flags */
    memcpy(&ar[20], &msec_flags, 4);

    /* application */
    rwRecMemGetApplication(rwrec, &ar[24]);

    /* swap if required */
    if (stream->swapFlag) {
        augmentedioRecordSwap_V1(ar);
    }

    return SKSTREAM_OK;
}


/* ********************************************************************* */

/*
 *  Return length of record of specified version, or 0 if no such
 *  version exists.  See skstream_priv.h for details.
 */
uint16_t
augmentedioGetRecLen(
    sk_file_version_t   vers)
{
    switch (vers) {
      case 1:
        return RECLEN_RWAUGMENTED_V1;
      case 2:
        return RECLEN_RWAUGMENTED_V2;
      case 3:
        return RECLEN_RWAUGMENTED_V3;
      case 4:
        return RECLEN_RWAUGMENTED_V4;
      case 5:
        return RECLEN_RWAUGMENTED_V5;
      default:
        return 0;
    }
}


/*
 *  status = augmentedioPrepare(&stream);
 *
 *    Sets the record version to the default if it is unspecified,
 *    checks that the record format supports the requested record
 *    version, sets the record length, and sets the pack and unpack
 *    functions for this record format and version.
 */
int
augmentedioPrepare(
    skstream_t         *stream)
{
#define FILE_FORMAT "FT_RWAUGMENTED"
    sk_file_header_t *hdr = stream->silk_hdr;
    int rv = SKSTREAM_OK; /* return value */

    assert(skHeaderGetFileFormat(hdr) == FT_RWAUGMENTED);

    /* Set version if none was selected by caller */
    if ((stream->io_mode == SK_IO_WRITE)
        && (skHeaderGetRecordVersion(hdr) == SK_RECORD_VERSION_ANY))
    {
        skHeaderSetRecordVersion(hdr, DEFAULT_RECORD_VERSION);
    }

    /* version check; set values based on version */
    switch (skHeaderGetRecordVersion(hdr)) {
      case 5:
        stream->rwUnpackFn = &augmentedioRecordUnpack_V5;
        stream->rwPackFn   = &augmentedioRecordPack_V5;
        break;
      case 4:
        stream->rwUnpackFn = &augmentedioRecordUnpack_V4;
        stream->rwPackFn   = &augmentedioRecordPack_V4;
        break;
      case 3:
      case 2:
      case 1:
        /* V1 and V2 differ only in the padding of the header */
        /* V2 and V3 differ only in that V3 supports compression on
         * read and write; V2 supports compression only on read */
        stream->rwUnpackFn = &augmentedioRecordUnpack_V1;
        stream->rwPackFn   = &augmentedioRecordPack_V1;
        break;
      case 0:
      default:
        rv = SKSTREAM_ERR_UNSUPPORT_VERSION;
        goto END;
    }

    stream->recLen = augmentedioGetRecLen(skHeaderGetRecordVersion(hdr));

    /* verify lengths */
    if (stream->recLen == 0) {
        skAppPrintErr("Record length not set for %s version %u",
                      FILE_FORMAT, (unsigned)skHeaderGetRecordVersion(hdr));
        skAbort();
    }
    if (stream->recLen != skHeaderGetRecordLength(hdr)) {
        if (0 == skHeaderGetRecordLength(hdr)) {
            skHeaderSetRecordLength(hdr, stream->recLen);
        } else {
            skAppPrintErr(("Record length mismatch for %s version %u\n"
                           "\tcode = %" PRIu16 " bytes;  header = %lu bytes"),
                          FILE_FORMAT, (unsigned)skHeaderGetRecordVersion(hdr),
                          stream->recLen,
                          (unsigned long)skHeaderGetRecordLength(hdr));
            skAbort();
        }
    }

  END:
    return rv;
}


/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/