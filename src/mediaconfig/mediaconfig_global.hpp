#ifndef MEDIACONFIG_GLOBAL_H
#define MEDIACONFIG_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MEDIACONFIG_LIBRARY)
#define MEDIACONFIG_EXPORT Q_DECL_EXPORT
#else
#define MEDIACONFIG_EXPORT Q_DECL_IMPORT
#endif

#endif // MEDIACONFIG_GLOBAL_H
