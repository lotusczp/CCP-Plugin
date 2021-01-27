#ifndef LDATAPROCESSING_H
#define LDATAPROCESSING_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include "LCommon.h"
#include "LCcpData.h"
#include "LUniqueResource.h"
#include "xcpmaster/XcpMaster.h"


class CanRxHandler : public QObject, public LCanDataReceiver
{
    Q_OBJECT
public:
    CanRxHandler(unsigned int a_uiCanId) : LCanDataReceiver(a_uiCanId){}
    void receiveMsgCallback(const VCI_CAN_OBJ& a_rMsg);
signals:
    void sendRxMsg(CanObj a_canObj);
};


class LDataProcessing : public QObject
{
    Q_OBJECT

public:
    explicit LDataProcessing(LCcpDataMap& a_rAllDataMap,
                             QMutex& a_rMutex,
                             QObject *parent = nullptr);
    virtual ~LDataProcessing();

    void setCanMsgId(uint32_t a_dwSlaveRxId, uint32_t a_dwSlaveTxId);

    void setCanDataManager(LCanDataManager *a_pCanDataManager);

    void startProcessing(bool a_bStart);

    void pullValueCmd(const QString &a_rName);

    void setValueCmd(const QString &a_rName, double a_dValue);

signals:
    void sendSingleVar(QString a_strDataName, LDataValue a_dataValue);

public slots:
    void applyAllData();

    //! Receive device connected
    void receiveDeviceReady(bool isConnected, unsigned long deviceIndex, unsigned long canIndex, unsigned long devType);

private:
    struct Info{
        unsigned devIndex;
        unsigned chanelIndex;
        unsigned devType;
    };

private:
    void sendXcpMessage(XcpMessage *a_pMsg);

    void waitForReply();

    void configDaqForSlave();

    void receiveRxMsg(CanObj a_canObj);

    void handleUploadReply(QVector<uint8_t> bytes);

    void receiveOdtEntryValue(uint16_t a_wDaqIndex, uint8_t a_byOdtIndex, uint32_t a_dwOdtEntryIndex, qint64 a_timestamp, double value);

private:
    LCcpDataMap     &m_rAllDataMap;
    QMutex          &m_rMutex;
    LCanDataManager *m_pCanDataManager;
    Info            m_info;
    uint32_t        m_dwSlaveRxId;
    uint32_t        m_dwSlaveTxId;
    CanRxHandler    *m_pCanRxHandler;
    bool            m_bIsConnected;
    XcpMaster       *m_pXcpMaster;
    LCcpData        *m_pPullingData;
    bool            m_bStartProcessing;
};

#endif // LDATAPROCESSING_H
