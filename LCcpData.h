#ifndef LCCPDATA_H
#define LCCPDATA_H

#include <QString>
#include <QList>
#include <QMap>
#include "LAttributes.h"

class LCcpData
{
public:
    enum Sort
    {
        Unkown,
        Measurement,
        Calibration,
    };

    enum DataType
    {
        eInt8 = 0,
        eUint8,
        eInt16,
        eUint16,
        eInt32,
        eUint32,
        eFloat,
    };

public:
    LCcpData()
    {
        m_dValue = 0;
        m_dwAddress = 0;
        m_eSort = Unkown;
        m_eDataType = eFloat;
        m_bIsEnable = true;
    }

    const QString &getName() const {return m_strName;}

    QString getRawName() const
    {
        QStringList splitted = m_strName.split("$");
        if(splitted.size() != 2) return "Invalid";
        return splitted[1];
    }
    void setRawName(const QString &a_rRawName, Sort a_eSort)
    {
        // We use the name to identify the measurement and calibration
        if(a_eSort == Measurement) {
            m_strName = "M$"+a_rRawName;
        }
        else if(a_eSort == Calibration) {
            m_strName = "C$"+a_rRawName;
        }
        m_eSort = a_eSort;
    }

    Sort getSort() const {return m_eSort;}

    double getValue() const {return m_dValue;}
    void setValue(double a_dValue) {m_dValue = a_dValue;}

    void setValueBytes(QVector<quint8> a_bytes)
    {
        if(getSize() <= a_bytes.size()) {
            switch(m_eDataType)
            {
            case DataType::eInt8:
            {
                int8_t value = (int8_t)a_bytes[0];
                m_dValue = (double)value;
                break;
            }
            case DataType::eUint8:
            {
                uint8_t value = (uint8_t)a_bytes[0];
                m_dValue = (double)value;
                break;
            }
            case DataType::eInt16:
            {
                int16_t value = ((((uint32_t)a_bytes[0])<<8)&0xFF00)+((((uint32_t)a_bytes[1])<<0)&0xFF);
                m_dValue = (double)value;
                break;
            }
            case DataType::eUint16:
            {
                uint16_t value = ((((uint32_t)a_bytes[0])<<8)&0xFF00)+((((uint32_t)a_bytes[1])<<0)&0xFF);
                m_dValue = (double)value;
                break;
            }
            case DataType::eInt32:
            {
                int32_t value = ((((uint32_t)a_bytes[0])<<24)&0xFF000000)+((((uint32_t)a_bytes[1])<<16)&0xFF0000)+((((uint32_t)a_bytes[2])<<8)&0xFF00)+((((uint32_t)a_bytes[3])<<0)&0xFF);
                m_dValue = (double)value;
                break;
            }
            case DataType::eUint32:
            {
                uint32_t value = ((((uint32_t)a_bytes[0])<<24)&0xFF000000)+((((uint32_t)a_bytes[1])<<16)&0xFF0000)+((((uint32_t)a_bytes[2])<<8)&0xFF00)+((((uint32_t)a_bytes[3])<<0)&0xFF);
                m_dValue = (double)value;
                break;
            }
            case DataType::eFloat:
            {
                uint32_t value = ((((uint32_t)a_bytes[0])<<24)&0xFF000000)+((((uint32_t)a_bytes[1])<<16)&0xFF0000)+((((uint32_t)a_bytes[2])<<8)&0xFF00)+((((uint32_t)a_bytes[3])<<0)&0xFF);
                m_dValue = (double)(*((float*)&value));
                break;
            }
            default:
                break;
            }
        }
    }

    uint32_t getRawValue() const
    {
        switch(m_eDataType)
        {
        case LCcpData::DataType::eInt8:
        {
            int8_t rawValue = (int8_t)m_dValue;
            return (uint32_t)rawValue;
        }
        case LCcpData::DataType::eUint8:
        {
            uint8_t rawValue = (uint8_t)m_dValue;
            return (uint32_t)rawValue;
        }
        case LCcpData::DataType::eInt16:
        {
            int16_t rawValue = (int16_t)m_dValue;
            return (uint32_t)rawValue;
        }
        case LCcpData::DataType::eUint16:
        {
            uint16_t rawValue = (uint16_t)m_dValue;
            return (uint32_t)rawValue;
        }
        case LCcpData::DataType::eInt32:
        {
            int32_t rawValue = (int32_t)m_dValue;
            return (uint32_t)rawValue;
        }
        case LCcpData::DataType::eUint32:
        {
            uint32_t rawValue = (uint32_t)m_dValue;
            return (uint32_t)rawValue;
        }
        case LCcpData::DataType::eFloat:
        {
            float fValue = (float)m_dValue;
            uint32_t rawValue = *((uint32_t*)&fValue);
            return (uint32_t)rawValue;
        }
        default:
            return 0;
        }
    }


    uint32_t getAddress() const {return m_dwAddress;}
    void setAddress(uint32_t a_dwAddress) {m_dwAddress = a_dwAddress;}

    int getEventChannel() const {return m_iEventChannel;}
    void setEventChannel(int a_iEventChannel) {m_iEventChannel = a_iEventChannel;}

    DataType getDataType() const {return m_eDataType;}
    void setDataType(DataType a_eDataType) {m_eDataType = a_eDataType;}

    uint8_t getSize() const
    {
        if(m_eDataType <= 1) {
            return 1;
        }
        else if(m_eDataType <= 3) {
            return 2;
        }
        else {
            return 4;
        }
    }

    QString getIndex() const {return m_strIndex;}
    void setIndex(const QString &a_rIndex) {m_strIndex = a_rIndex;}

    bool isEnable() const {return m_bIsEnable;}
    void setEnable(bool a_bEnable) {m_bIsEnable = a_bEnable;}

private:
    QString     m_strName;
    double      m_dValue;
    uint32_t    m_dwAddress;
    DataType    m_eDataType;
    int         m_iEventChannel; //!< Event channel number, unused if this is calibration point
    Sort        m_eSort;
    QString     m_strIndex;      //!< Only available for measurement, to present the position in DAQ list
    bool        m_bIsEnable;     //!< Only available for measurement
};


typedef QList<LCcpData*>                    LCcpDataArray;
typedef QListIterator<LCcpData*>            LCcpDataArrayIter;
typedef QMap<QString, LCcpData*>            LCcpDataMap;
typedef QMapIterator<QString, LCcpData*>    LCcpDataMapIter;

#endif // LCCPDATA_H
