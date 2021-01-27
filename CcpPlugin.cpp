#include "CcpPlugin.h"


#include "LCcpTrans.h"

LTransmission *CcpPlugin::createTransInstance()
{
    return new LCcpTrans;
}
