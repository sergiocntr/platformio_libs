// version_check.h
#ifndef VERSION_CHECK_H
#define VERSION_CHECK_H

#include "version.h"

#if defined(EXPECTED_LIB_VERSION_MAJOR) && defined(EXPECTED_LIB_VERSION_MINOR)
    #if (EXPECTED_LIB_VERSION_MAJOR != LIB_VERSION_MAJOR)
        #warning "⚠️ ATTENZIONE: Major version mismatch! Libreria v" LIB_VERSION_STRING ", progetto si aspetta v" TOSTRING(EXPECTED_LIB_VERSION_MAJOR) "." TOSTRING(EXPECTED_LIB_VERSION_MINOR) ".x"
    #elif (EXPECTED_LIB_VERSION_MINOR > LIB_VERSION_MINOR)
        #warning "⚠️ ATTENZIONE: Minor version mismatch! Libreria v" LIB_VERSION_STRING ", progetto si aspetta v" TOSTRING(EXPECTED_LIB_VERSION_MAJOR) "." TOSTRING(EXPECTED_LIB_VERSION_MINOR) ".x"
    #endif
#endif

#endif