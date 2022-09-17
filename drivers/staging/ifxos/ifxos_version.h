/****************************************************************************

         Copyright (c) 2020 MaxLinear, Inc.
         Copyright (c) 2016 - 2019 Intel Corporation
         Copyright (c) 2011 - 2016 Lantiq Beteiligungs-GmbH & Co. KG

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*****************************************************************************/
#ifndef _IFXOS_VERSION_H
#define _IFXOS_VERSION_H

/** \file
   This file contains the version and the corresponding check definitions for the IFX OS.

*/


/**
   The version of the IFXOS is separated into 3 numbers:
   "<interface revision> . <feature step> . <build step>"


   - Interface Revision:
      The interface revision defines a IFXOS API version. The revision number
      will only change if the existing IFXOS API is changed and the functions are
      not anymore backward compatible.

      Example:
      An already existing function gets an new parameter.

      \attention
         If a new API function is added this requires no change of the revision
         number. Therefore the feature step will be incremented.

   - Feature Step:
      The number defines the current IFXOS API features. The number will change if
      new features have been added and the existing API is not changed.

      Example:
      Add additional socket functions - the available functions are unchanged.

   - Build Step:
      The build step is incremented for internal bugfixes and minor changes.
      Fixes and minor changes will have no influence to the IFXOS header files.
      No recompilation of the user applications required.

\ingroup IFXOS_COMMON

*/

/* ==========================================================================
   Includes
   ========================================================================== */
#include "ifx_types.h"

/* ==========================================================================
   IFXOS Version defs
   ========================================================================== */

#ifndef _MKSTR_1
#define _MKSTR_1(x)    #x
#define _MKSTR(x)      _MKSTR_1(x)
#endif

/** IFXOS version, base name */
#define IFXOS_BASE_NAME                "IFXOS"

/** IFXOS version - Interface revision */
#define IFXOS_VERSION_REVISION                1
/** IFXOS version - feature */
#define IFXOS_VERSION_FEATURE                 7
/** IFXOS version, build number - step */
#define IFXOS_VERSION_STEP                    1
/** IFXOS version, build number - build (test, debug, maintenance) */
#define IFXOS_VERSION_BUILD                   0

/** IFXOS version as number */
#define IFXOS_VER_NUMBER \
            ((IFXOS_VERSION_REVISION << 16) || (IFXOS_VERSION_FEATURE << 8) || (IFXOS_VERSION_STEP))

/** IFXOS version as string */
#if (IFXOS_VERSION_BUILD == 0)
#define IFXOS_VER_STR \
            _MKSTR(IFXOS_VERSION_REVISION) "." _MKSTR(IFXOS_VERSION_FEATURE) "." _MKSTR(IFXOS_VERSION_STEP)
#else
#define IFXOS_VER_STR \
            _MKSTR(IFXOS_VERSION_REVISION) "." _MKSTR(IFXOS_VERSION_FEATURE) "." \
            _MKSTR(IFXOS_VERSION_STEP) "." _MKSTR(IFXOS_VERSION_BUILD)
#endif

/** IFXOS version, what string */
#define IFXOS_WHAT_STR "@(#)" IFXOS_BASE_NAME ", Version " IFXOS_VER_STR


/* ==========================================================================
   IFXOS Version checks
   ========================================================================== */
/**
   Version Check - equal with the given version
*/
#define IFXOS_VERSION_CHECK_EQ(revision, feature, step) \
               ((IFXOS_VERSION_REVISION == revision) && (IFXOS_VERSION_FEATURE == feature) && (IFXOS_VERSION_STEP == step) )

/**
   Version Check - less than given version
   - revision number must match and
     --> feature number must be less or
     --> step number must be less if feature is the same.
*/
#define IFXOS_VERSION_CHECK_L_THAN(revision, feature, step) \
               (     (IFXOS_VERSION_REVISION <= revision) \
                 &&  (   (IFXOS_VERSION_FEATURE < feature) \
                      || ((IFXOS_VERSION_FEATURE == feature) && (IFXOS_VERSION_STEP < step)) ))

/**
   Version Check - equal or greater than given version
   - revision number must match and
     --> feature number must be greater or
     --> step number must be greater if feature is the same.
*/
#define IFXOS_VERSION_CHECK_EG_THAN(revision, feature, step) \
               (     (IFXOS_VERSION_REVISION == revision) \
                 &&  (   (IFXOS_VERSION_FEATURE > feature) \
                      || ((IFXOS_VERSION_FEATURE == feature) && (IFXOS_VERSION_STEP >= step)) ))


/* ==========================================================================
   IFXOS Version - functions
   ========================================================================== */
IFX_void_t IFXOS_versionGet(
                     IFX_uint8_t *pRevision,
                     IFX_uint8_t *pFeature,
                     IFX_uint8_t *pStep);

IFX_boolean_t  IFXOS_versionCheck_equal(
                     IFX_uint8_t revisionNum,
                     IFX_uint8_t featureNum,
                     IFX_uint8_t stepNum);

IFX_boolean_t  IFXOS_versionCheck_egThan(
                     IFX_uint8_t revisionNum,
                     IFX_uint8_t featureNum,
                     IFX_uint8_t stepNum);

IFX_boolean_t  IFXOS_versionCheck_lessThan(
                     IFX_uint8_t revisionNum,
                     IFX_uint8_t featureNum,
                     IFX_uint8_t stepNum);


/** @} */

#endif      /* #ifndef _IFXOS_VERSION_H */

