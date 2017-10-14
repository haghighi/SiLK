/*
** Copyright (C) 2007-2017 by Carnegie Mellon University.
**
** @OPENSOURCE_LICENSE_START@
** See license information in ../../LICENSE.txt
** @OPENSOURCE_LICENSE_END@
*/

/*
**  skipfix.h
**
**    Converts between IPFIX and SiLK Flow Recs
**
*/
#ifndef _SKIPFIX_H
#define _SKIPFIX_H
#ifdef __cplusplus
extern "C" {
#endif

#include <silk/silk.h>

RCSIDENTVAR(rcsID_SKIPFIX_H, "$SiLK: skipfix.h 275df62a2e41 2017-01-05 17:30:40Z mthomas $");

#include <silk/silk_types.h>
#include <silk/libflowsource.h>

/* quiet warnings generated by -Wundef */
#if !SK_HAVE_DECL_FB_ENABLE_SCTP
#define FB_ENABLE_SCTP 0
#endif
#if !SK_HAVE_DECL_HAVE_OPENSSL
#define HAVE_OPENSSL 0
#endif
/* fixbuf not consistent in use of "#if" vs "#ifdef"
 *|#if !SK_HAVE_DECL_HAVE_SPREAD
 *|#define HAVE_SPREAD 0
 *|#endif
 */

SK_DIAGNOSTIC_IGNORE_PUSH("-Wundef")

#include <fixbuf/public.h>

SK_DIAGNOSTIC_IGNORE_POP("-Wundef")


/**
 * @file
 *
 * SiLK Flow record (rwRec) interface to fixbuf. Supports the reading
 * and writing of IPFIX Files by SiLK applications, and the creation
 * of IPFIX Collecting and Exporting Processes using the SiLK flow
 * format.
 *
 * This file is part of libflowsource.
 *
 * This library supports the creation of fbListener_t and fBuf_t
 * instances configured to read any IPFIX record containing the
 * appropriate information elements as a SiLK rwRec, and the creation
 * of fBuf_t instances configured to write SiLK rwRecs as IPFIX
 * records.
 *
 * To read SiLK Flow records from an IPFIX file, use fopen() to open
 * the file, create a buffer using skiCreateReadBufferForFP(), and
 * iterate over records with skiRwNextRecord().  When done, use
 * fBufFree() to free the resulting buffer and fclose() the file.
 *
 * To write SiLK Flow records to an IPFIX file, fopen() the file,
 * create a buffer using skiCreateWriteBufferForFP(), and write each
 * record with skiRwAppendRecord().  Use fBufFree() to free the
 * resulting buffer and fclose() the file.
 *
 * This library uses the GError facility from glib for reporting
 * errors.  Pass a pointer to a NULL GError * on any call taking an
 * err parameter; if an error occurs, the function will return NULL or
 * FALSE as appropriate, and allocate a GError structure describing
 * the error. If an error occurs, you can use the g_error_matches()
 * macro to match it against FB_ERROR_* constants defined by
 * libfixbuf, and the err->message field to get a human-readable error
 * message. After handling an error, use the g_clear_error() macro to
 * free the error description. See the glib documentation for more
 * details on this facility.
 *
 * See the documentation for libfixbuf for details on the fBuf_t,
 * fbListener_t, fbCollector_t, fbExporter_t, and fbConnSpec_t types,
 * and the fbListenerAppInit_fn and fbListenerAppFree_fn callbacks.
 */


/**
 *    The IPFIX Private Enterprise Number for CERT.
 */
#define IPFIX_CERT_PEN  6871



#ifdef __cplusplus
}
#endif
#endif /* _SKIPFIX_H */

/*
** Local Variables:
** mode:c
** indent-tabs-mode:nil
** c-basic-offset:4
** End:
*/