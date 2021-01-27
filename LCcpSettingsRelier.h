#ifndef LCCPSETTINGSRELIER_H
#define LCCPSETTINGSRELIER_H

#include <QObject>
#include "LSettingsRelier.h"
#include "LElfVarEditorWidget.h"

class LCcpSettingsRelier : public QObject, public LSettingsRelier
{
    Q_OBJECT
public:
    LCcpSettingsRelier(LCcpDataMap &a_rAllDataMap, LElfVarEditorWidget* a_pElfVarEditorWidget);
    virtual ~LCcpSettingsRelier() {}

    void parseFromSetting(LObixTreeIter a_obixIter) Q_DECL_OVERRIDE ;

    void convertToSetting() Q_DECL_OVERRIDE ;

public slots:
    void saveSettings();

private:
    LElfVarEditorWidget *m_pElfVarEditorWidget;
    LCcpDataMap         &m_rAllDataMap;
};

#endif // LCCPSETTINGSRELIER_H
