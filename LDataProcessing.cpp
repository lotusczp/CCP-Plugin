#include "LDataProcessing.h"
#include <QMessageBox>
#include <QEventLoop>
#include "xcpmaster/DAQ.h"

#define CAN_REPLY_TIMEOUT_ms    (2000)

void CanRxHandler::receiveMsgCallback(const VCI_CAN_OBJ &a_rMsg)
{
    emit sendRxMsg(a_rMsg);
}


LDataProcessing::LDataProcessing(LCcpDataMap &a_rAllDataMap, QMutex &a_rMutex, QObject *parent)
    : QObject (parent),
      m_rAllDataMap(a_rAllDataMap),
      m_rMutex(a_rMutex)
{
    qRegisterMetaType<CanObj>("CanObj");

    m_pCanDataManager = nullptr;
    m_pCanRxHandler = nullptr;
    m_bIsConnected = false;
    m_pPullingData = nullptr;
    m_bStartProcessing = false;

    m_pXcpMaster = new XcpMaster(XcpMaster::TransportLayer::CAN);
    connect(m_pXcpMaster, &XcpMaster::sendUploadByteArray, this, &LDataProcessing::handleUploadReply);
    connect(m_pXcpMaster, &XcpMaster::sendOdtEntryValue, this, &LDataProcessing::receiveOdtEntryValue);
}

LDataProcessing::~LDataProcessing()
{
    sendXcpMessage(m_pXcpMaster->createDisconnectMessage());
    waitForReply();

    if(m_pCanRxHandler) delete m_pCanRxHandler;
    delete m_pXcpMaster;
}

void LDataProcessing::setCanMsgId(uint32_t a_dwSlaveRxId, uint32_t a_dwSlaveTxId)
{
    m_dwSlaveRxId = a_dwSlaveRxId;
    m_dwSlaveTxId = a_dwSlaveTxId;

    m_pCanRxHandler = new CanRxHandler(a_dwSlaveTxId);
    connect(m_pCanRxHandler, &CanRxHandler::sendRxMsg, this, &LDataProcessing::receiveRxMsg);
}

void LDataProcessing::setCanDataManager(LCanDataManager *a_pCanDataManager)
{
    m_pCanDataManager = a_pCanDataManager;
}

void LDataProcessing::startProcessing(bool a_bStart)
{
    if(m_pCanDataManager && m_bIsConnected) {
        if(a_bStart) {
            sendXcpMessage(m_pXcpMaster->createStartStopSynchMessage(StartStopSynchPacket::Mode::START_SELECTED));
            waitForReply();
        }
        else {
            sendXcpMessage(m_pXcpMaster->createStartStopSynchMessage(StartStopSynchPacket::Mode::STOP_ALL));
            waitForReply();
        }
        m_bStartProcessing = a_bStart;
    }
}

void LDataProcessing::pullValueCmd(const QString &a_rName)
{
    if(m_pPullingData == nullptr) {
        if(m_rAllDataMap.contains(a_rName)) {
            m_pPullingData = m_rAllDataMap[a_rName];
            sendXcpMessage(m_pXcpMaster->createShortUploadMessage(m_pPullingData->getSize(), m_pPullingData->getAddress(), 0));
            waitForReply();
        }
    }
    else {
        // Last pull command not finished, ignore the new command
    }
}

void LDataProcessing::setValueCmd(const QString &a_rName, double a_dValue)
{
    if(m_rAllDataMap.contains(a_rName)) {
        m_rAllDataMap[a_rName]->setValue(a_dValue);
        // Set MTA
        sendXcpMessage(m_pXcpMaster->createSetMtaMessage(m_rAllDataMap[a_rName]->getAddress(), 0));
        waitForReply();
        // Set value
        uint32_t dwRawValue = m_rAllDataMap[a_rName]->getRawValue();
        sendXcpMessage(m_pXcpMaster->createDownloadMessage(m_rAllDataMap[a_rName]->getSize(), dwRawValue));
        waitForReply();

        // Pull value after set
        pullValueCmd(a_rName);
    }
}

void LDataProcessing::applyAllData()
{
    // Must connect first
    if(m_bIsConnected == false) {
        QMessageBox::warning(nullptr, QObject::tr("Warning"), "CCP Plugin:\nPlease connect the CAN device first.");
        return;
    }

    if(m_bStartProcessing) {
        // Already started. stop first
        sendXcpMessage(m_pXcpMaster->createStartStopSynchMessage(StartStopSynchPacket::Mode::STOP_ALL));
        waitForReply();
    }

    // Build the DAQ layout
    DaqLayout daqlayout;
    QMap<int, DAQ*> daqMap; // One daq for one channel
    LCcpDataMapIter it(m_rAllDataMap);
    while(it.hasNext()) {
        it.next();
        if(it.value()->getSort() == LCcpData::Sort::Measurement
                && it.value()->isEnable()) {
            it.value()->setIndex(""); // Clear first
            uint16_t wEventChannel = it.value()->getEventChannel();
            if(!daqMap.contains(wEventChannel)) {
                // New event channel
                daqMap.insert(wEventChannel, new DAQ);
                daqMap[wEventChannel]->setMode(0);
                daqMap[wEventChannel]->setEventChannel(wEventChannel);
                daqMap[wEventChannel]->setPrescaler(1);
                daqMap[wEventChannel]->setPriority(0);
            }
            //! \note We match 1 point for just 1 ODT, this is simple but waste the bandwidth
            ODT odt;
            OdtEntry odtEntry(it.value()->getAddress(), 0, it.value()->getSize());
            //! \note LCcpData::DataType and MeasurementDataTypes must be compatible
            odtEntry.setDataType((MeasurementDataTypes)it.value()->getDataType());
            odt.addEntry(odtEntry);
            daqMap[wEventChannel]->addOdt(odt);
            // Mark the index in DAQ list
            int iDaqIndex = daqMap.keys().indexOf(wEventChannel);
            int iOdtIndex = daqMap[wEventChannel]->getNumberOfOdts()-1;
            it.value()->setIndex(QString::number(iDaqIndex)+","+QString::number(iOdtIndex)+",0"); // ODT entry index is always 0
        }
    }

    // Add to master's DAQ list
    QMapIterator<int, DAQ*> daqMapIt(daqMap);
    while(daqMapIt.hasNext()) {
        daqMapIt.next();
        daqlayout.addDaq(*daqMapIt.value());
    }

    daqlayout.setInitialized(true);
    m_pXcpMaster->setDaqLayout(daqlayout);

    // Configure the DAQ settings to slave
    configDaqForSlave();

    if(m_bStartProcessing) {
        // Already started. re-start
        sendXcpMessage(m_pXcpMaster->createStartStopSynchMessage(StartStopSynchPacket::Mode::START_SELECTED));
        waitForReply();
    }
}

void LDataProcessing::receiveDeviceReady(bool isConnected, unsigned long deviceIndex, unsigned long canIndex, unsigned long devType)
{
    m_bIsConnected = isConnected;

    m_info.devIndex = deviceIndex;
    m_info.chanelIndex = canIndex;
    m_info.devType = devType;

    if(m_bIsConnected && m_pCanDataManager && m_pCanRxHandler) {
        m_pCanDataManager->registerReceiver(m_pCanRxHandler, m_info.devIndex, m_info.chanelIndex, m_info.devType);

        sendXcpMessage(m_pXcpMaster->createConnectMessage(CmdPacketConnect::ConnectMode::NORMAL));
        waitForReply();
    }
}

void LDataProcessing::sendXcpMessage(XcpMessage *a_pMsg)
{
    if(m_pCanDataManager && m_bIsConnected) {
        CanObj txMsg;
        txMsg.ID = 0x100;
        txMsg.SendType = 0;
        txMsg.ExternFlag = 0;
        txMsg.RemoteFlag = 0;
        txMsg.DataLen = 8;
        for(int i=0; i<a_pMsg->getByteArray().size(); i++) {
            txMsg.Data[i]=(a_pMsg->getByteArray()[i]);
        }
        for(int i=a_pMsg->getByteArray().size(); i<8; i++) {
            txMsg.Data[i]=0; // Fullfill the bytes for CAN
        }
        m_pCanDataManager->transmit(&txMsg, 1, m_info.devIndex, m_info.chanelIndex, m_info.devType);
        m_pXcpMaster->handleSentMessage(a_pMsg);
    }
}

void LDataProcessing::waitForReply()
{
    if(m_pCanDataManager && m_pCanRxHandler && m_bIsConnected) {
        QEventLoop *pEventloop = new QEventLoop;
        QTimer *pTimer = new QTimer;
        pTimer->setSingleShot(true);
        connect(m_pCanRxHandler, &CanRxHandler::sendRxMsg, pEventloop, &QEventLoop::quit);
        connect(pTimer, &QTimer::timeout, pEventloop, &QEventLoop::quit);
        pTimer->start(CAN_REPLY_TIMEOUT_ms);
        pEventloop->exec();
        if(pTimer->isActive()) {
            // Reply within timeout
            // Normal quit
        }
        else{
            QMessageBox::warning(nullptr, QObject::tr("Warning"), "CCP Plugin:\nCAN reply from slave timeout.\nPlease check the CAN connection.\nIs the slave working?");
        }
        delete pTimer;
        delete pEventloop;
    }
}

void LDataProcessing::configDaqForSlave()
{
    sendXcpMessage(m_pXcpMaster->createGetDaqProcessorInfoMessage());
    waitForReply();

    DaqLayout daqlayout = m_pXcpMaster->getDaqLayout();

    // Dynamic config, free and re-alloc
    sendXcpMessage(m_pXcpMaster->createFreeDaqMessage());
    waitForReply();
    sendXcpMessage(m_pXcpMaster->createAllocDaqMessage(daqlayout.getNumberOfDaqLists()));
    waitForReply();
    for(int i=0; i < daqlayout.getNumberOfDaqLists(); i++) {
        DAQ daq = daqlayout.getDaq(i);
        sendXcpMessage(m_pXcpMaster->createAllocOdtMessage(i, daq.getNumberOfOdts()));
        waitForReply();
        for (int j = 0; j < daqlayout.getDaq(i).getNumberOfOdts(); j++) {
            ODT odt = daq.getOdt(j);
            sendXcpMessage(m_pXcpMaster->createAllocOdtEntryMessage(i, j, odt.getNumberOfEntries()));
            waitForReply();
        }
    }

    // Must finish the alloc process before set DAQ ptr and write DAQ
    for(int i=0; i<daqlayout.getNumberOfDaqLists(); i++) {
        DAQ daq =daqlayout.getDaq(i);
        for (int j = 0; j < daqlayout.getDaq(i).getNumberOfOdts(); j++) {
            ODT odt = daq.getOdt(j);
            for (int k = 0; k < daqlayout.getDaq(i).getOdt(j).getNumberOfEntries(); k++) {
                OdtEntry entry = odt.getEntry(k);
                sendXcpMessage(m_pXcpMaster->createSetDaqPtrMessage(i, j, k));
                waitForReply();
                sendXcpMessage(m_pXcpMaster->createWriteDaqMessage(0xFF, entry.getLength(), entry.getAddressExtension(), entry.getAddress()));
                waitForReply();
            }
        }
    }

    // Set DAQ list mode and select all
    for(int id=0; id<daqlayout.getNumberOfDaqLists(); id++) {
        sendXcpMessage(m_pXcpMaster->createSetDaqListModeMessage(daqlayout.getDaq(id).getMode(),
                                                                 id,
                                                                 daqlayout.getDaq(id).getEventChannel(),
                                                                 daqlayout.getDaq(id).getPrescaler(),
                                                                 daqlayout.getDaq(id).getPriority()));
        waitForReply();

        sendXcpMessage(m_pXcpMaster->createStartStopDaqListMessage(StartStopDaqListPacket::Mode::SELECT, id));
        waitForReply();
    }
}

void LDataProcessing::receiveRxMsg(CanObj a_canObj)
{
    QVector<quint8> ba;
    for(int i=0; i<8; i++) {
        ba.append(a_canObj.Data[i]);
    }
    m_pXcpMaster->deserializeMessage(ba);
}

void LDataProcessing::handleUploadReply(QVector<uint8_t> bytes)
{
    if(m_pPullingData) {
        m_pPullingData->setValueBytes(bytes);
        LDataValue dataValue;
        dataValue.set(m_pPullingData->getValue());
        emit sendSingleVar(m_pPullingData->getName(), dataValue);
        m_pPullingData = nullptr;
    }
}

void LDataProcessing::receiveOdtEntryValue(uint16_t a_wDaqIndex, uint8_t a_byOdtIndex, uint32_t a_dwOdtEntryIndex, qint64 a_timestamp, double value)
{
    QString strIndex = QString::number(a_wDaqIndex)+","+QString::number(a_byOdtIndex)+","+QString::number(a_dwOdtEntryIndex);
    LCcpDataMapIter it(m_rAllDataMap);
    while(it.hasNext()) {
        it.next();
        if(it.value()->getIndex() == strIndex) {
            // Found the value
            it.value()->setValue(value);
            LDataValue dataValue;
            dataValue.set(it.value()->getValue());
            emit sendSingleVar(it.value()->getName(), dataValue);
            break;
        }
    }
}
