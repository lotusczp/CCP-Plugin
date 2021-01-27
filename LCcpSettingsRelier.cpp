#include "LCcpSettingsRelier.h"

LCcpSettingsRelier::LCcpSettingsRelier(LCcpDataMap &a_rAllDataMap, LElfVarEditorWidget *a_pElfVarEditorWidget)
    : m_rAllDataMap(a_rAllDataMap)
{
    m_pElfVarEditorWidget = a_pElfVarEditorWidget;
}

void LCcpSettingsRelier::parseFromSetting(LObixTreeIter a_obixIter)
{
    m_pElfVarEditorWidget->clearAll();

    a_obixIter.moveToRoot();
    if(a_obixIter.hasChild()) {
        a_obixIter.moveToFirstChild();
        if( (a_obixIter.getValue().getType() == eObj) && (a_obixIter.getValue().getProperty("is") == "Measurements") ) {
            if(a_obixIter.hasChild()) {
                a_obixIter.moveToFirstChild();
                bool bFinish = false;
                while(!bFinish) {
                    if(a_obixIter.getValue().getType() == eObj) {
                        QString strRawName = a_obixIter.getValue().getProperty("name");
                        uint32_t dwAddress = a_obixIter.getValue().getProperty("address").toULong();
                        int iType = a_obixIter.getValue().getProperty("type").toInt();
                        int iEvent = a_obixIter.getValue().getProperty("event").toInt();
                        bool bIsEnable = a_obixIter.getValue().getProperty("enable") == "true";
                        m_pElfVarEditorWidget->addMeasurement(strRawName, dwAddress, iType, iEvent, bIsEnable);
                    }
                    if(a_obixIter.hasSibling()) {
                        a_obixIter.moveToNextSibling();
                    }
                    else {
                        bFinish = true;
                    }
                }
                a_obixIter.moveToParent();
            }
        }

        a_obixIter.moveToNextSibling();
        if( (a_obixIter.getValue().getType() == eObj) && (a_obixIter.getValue().getProperty("is") == "Calibrations") ) {
            if(a_obixIter.hasChild()) {
                a_obixIter.moveToFirstChild();
                bool bFinish = false;
                while(!bFinish) {
                    if(a_obixIter.getValue().getType() == eObj) {
                        QString strRawName = a_obixIter.getValue().getProperty("name");
                        uint32_t dwAddress = a_obixIter.getValue().getProperty("address").toULong();
                        int iType = a_obixIter.getValue().getProperty("type").toInt();
                        m_pElfVarEditorWidget->addCalibration(strRawName, dwAddress, iType);
                    }
                    if(a_obixIter.hasSibling()) {
                        a_obixIter.moveToNextSibling();
                    }
                    else {
                        bFinish = true;
                    }
                }
                a_obixIter.moveToParent();
            }
        }

        a_obixIter.moveToNextSibling();

        if( (a_obixIter.getValue().getType() == eStr) && (a_obixIter.getValue().getProperty("is") == "file") ) {
            QString strFilePath = a_obixIter.getValue().getProperty("path");
            m_pElfVarEditorWidget->setElfFilePath(strFilePath);
        }
    }

    m_pElfVarEditorWidget->refreshAllData();
}

void LCcpSettingsRelier::convertToSetting()
{
    m_obixMutableIter.moveToRoot();
    // build a new subtree
    if (m_obixMutableIter.hasChild()) {
        m_obixMutableIter.removeChildren();
    }

    LObixObj obixObjMeas("obj", "is", "Measurements");
    m_obixMutableIter.appendChild(obixObjMeas);

    LObixObj obixObjCalib("obj", "is", "Calibrations");
    m_obixMutableIter.appendChild(obixObjCalib);

    m_obixMutableIter.moveToRoot();

    m_pElfVarEditorWidget->refreshAllData();

    LCcpDataMapIter it(m_rAllDataMap);
    while(it.hasNext()) {
        it.next();
        if(it.value()->getSort() == LCcpData::Sort::Measurement) {
            // First child is Measurements
            m_obixMutableIter.moveToFirstChild();

            LObixObj obixObjPoint("obj", "name", it.value()->getRawName());
            obixObjPoint.addProperty("address", QString::number(it.value()->getAddress()));
            obixObjPoint.addProperty("type", QString::number(it.value()->getDataType()));
            obixObjPoint.addProperty("event", QString::number(it.value()->getEventChannel()));
            obixObjPoint.addProperty("enable", it.value()->isEnable() ? "true" : "false");
            m_obixMutableIter.appendChild(obixObjPoint);

            m_obixMutableIter.moveToRoot();
        }
        else if(it.value()->getSort() == LCcpData::Sort::Calibration) {
            // Second child is Calibrations
            m_obixMutableIter.moveToFirstChild();
            m_obixMutableIter.moveToNextSibling();

            LObixObj obixObjPoint("obj", "name", it.value()->getRawName());
            obixObjPoint.addProperty("address", QString::number(it.value()->getAddress()));
            obixObjPoint.addProperty("type", QString::number(it.value()->getDataType()));
            m_obixMutableIter.appendChild(obixObjPoint);

            m_obixMutableIter.moveToRoot();
        }
    }

    LObixObj obixObjFile("str", "is", "file");
    obixObjFile.addProperty("path", m_pElfVarEditorWidget->getElfFilePath());
    m_obixMutableIter.appendChild(obixObjFile);
}

void LCcpSettingsRelier::saveSettings()
{
    convertToSetting();
}
