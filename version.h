// version.h
#ifndef LIB_VERSION_H
#define LIB_VERSION_H

#define LIB_VERSION_MAJOR 1
#define LIB_VERSION_MINOR 4
#define LIB_VERSION_PATCH 2

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define LIB_VERSION_STRING TOSTRING(LIB_VERSION_MAJOR) "." \
                           TOSTRING(LIB_VERSION_MINOR) "." \
                           TOSTRING(LIB_VERSION_PATCH)
#endif