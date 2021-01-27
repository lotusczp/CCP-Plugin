#ifndef CCPPLUGIN_GLOBAL_H
#define CCPPLUGIN_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CCPPLUGIN_LIBRARY)
#  define CCPPLUGINSHARED_EXPORT Q_DECL_EXPORT
#else
#  define CCPPLUGINSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // CCPPLUGIN_GLOBAL_H
