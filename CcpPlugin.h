#ifndef CCPPLUGIN_H
#define CCPPLUGIN_H

#include "ccpplugin_global.h"

#include "LTransPluginFactory.h"

class CCPPLUGINSHARED_EXPORT CcpPlugin : public LTransPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Lobster.LTransPluginFactory" FILE "CcpPlugin.json")
    Q_INTERFACES(LTransPluginFactory)
public:
    CcpPlugin() {}
    ~CcpPlugin() Q_DECL_OVERRIDE {}

    //! create the transmission instance
    virtual LTransmission* createTransInstance() Q_DECL_OVERRIDE;
};

#endif // CCPPLUGIN_H
