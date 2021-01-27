#include "LElfVarEditorWidget.h"
#include "ui_LElfVarEditorWidget.h"

#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QCheckBox>

#define COLUMN_NAME             0
#define COLUMN_TYPE             1
#define COLUMN_ADDRESS          2
#define COLUMN_TOOL             3
#define MEAS_COLUMN_EVENT       4
#define MEAS_COLUMN_ENABLE      5

// Must be the same with the embedded code
#define EVENT_1MS           1
#define EVENT_5MS           2
#define EVENT_10MS          3
#define EVENT_100MS         4


LElfVarEditorWidget::LElfVarEditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LElfVarEditorWidget)
{
    ui->setupUi(this);
    ui->tableMeas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableCalib->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_pMapAllData = nullptr;

    m_pTimer = new QTimer;
    connect(m_pTimer, &QTimer::timeout, this, &LElfVarEditorWidget::handleFileParseProgress);

    m_pThread = new QThread;

    m_pElfParser = new LElfParser;
    connect(this, &LElfVarEditorWidget::findAddress, m_pElfParser, &LElfParser::findAddress);
    connect(m_pElfParser, &LElfParser::addressFound, this, &LElfVarEditorWidget::receiveAddressFound);
    m_pElfParser->moveToThread(m_pThread);

    m_pTimer->setInterval(100);
    m_pThread->start();
}

LElfVarEditorWidget::~LElfVarEditorWidget()
{
    m_pTimer->stop();
    delete m_pTimer;

    // Make sure the thread quits
    m_pThread->quit();
    m_pThread->wait();
    delete m_pThread;

    delete ui;
}

void LElfVarEditorWidget::refreshAllData()
{
    if(m_pMapAllData == nullptr) return;

    // Clear the previous map
    qDeleteAll(*m_pMapAllData);
    m_pMapAllData->clear();

    // Build the data map
    for(int i=0; i<ui->tableMeas->rowCount(); i++) {
        QString strName = ui->tableMeas->item(i, COLUMN_NAME)->text();
        uint32_t dwAddress = ui->tableMeas->item(i, COLUMN_ADDRESS)->text().toULong(nullptr, 16);
        QComboBox* pType = qobject_cast<QComboBox*>(ui->tableMeas->cellWidget(i, COLUMN_TYPE));
        QComboBox* pEvent = qobject_cast<QComboBox*>(ui->tableMeas->cellWidget(i, MEAS_COLUMN_EVENT));
        QCheckBox* pCheckBox = qobject_cast<QCheckBox*>(ui->tableMeas->cellWidget(i, MEAS_COLUMN_ENABLE));
        if(!strName.isEmpty() && dwAddress != 0 && pType && pEvent && pCheckBox) {
            LCcpData *pMeasPoint = new LCcpData;
            pMeasPoint->setRawName(strName, LCcpData::Sort::Measurement);
            pMeasPoint->setAddress(dwAddress);
            pMeasPoint->setDataType((LCcpData::DataType)pType->currentData().toInt());
            pMeasPoint->setEventChannel(pEvent->currentData().toInt());
            pMeasPoint->setEnable(pCheckBox->checkState()==Qt::Checked);
            m_pMapAllData->insert(pMeasPoint->getName(), pMeasPoint);
        }
    }
    for(int i=0; i<ui->tableCalib->rowCount(); i++) {
        QString strName = ui->tableCalib->item(i, COLUMN_NAME)->text();
        uint32_t dwAddress = ui->tableCalib->item(i, COLUMN_ADDRESS)->text().toULong(nullptr, 16);
        QComboBox* pType = qobject_cast<QComboBox*>(ui->tableCalib->cellWidget(i, COLUMN_TYPE));
        if(!strName.isEmpty() && dwAddress != 0 && pType) {
            LCcpData *pCalibPoint = new LCcpData;
            pCalibPoint->setRawName(strName, LCcpData::Sort::Calibration);
            pCalibPoint->setAddress(dwAddress);
            pCalibPoint->setDataType((LCcpData::DataType)pType->currentData().toInt());
            pCalibPoint->setEnable(true);
            m_pMapAllData->insert(pCalibPoint->getName(), pCalibPoint);
        }
    }
}

void LElfVarEditorWidget::clearAll()
{
    if(m_pMapAllData == nullptr) return;

    ui->tableMeas->clearContents();
    ui->tableMeas->setRowCount(0);
    ui->tableCalib->clearContents();
    ui->tableCalib->setRowCount(0);

    qDeleteAll(*m_pMapAllData);
    m_pMapAllData->clear();
}

void LElfVarEditorWidget::addMeasurement(QString a_strRawName, uint32_t a_dwAddress, int a_iType, int a_iEvent, bool a_bIsEnable)
{
    on_btnAddRowMeas_clicked();

    ui->tableMeas->item(ui->tableMeas->rowCount()-1, COLUMN_NAME)->setText(a_strRawName);
    ui->tableMeas->item(ui->tableMeas->rowCount()-1, COLUMN_ADDRESS)->setText("0x"+QString::number(a_dwAddress, 16));
    QComboBox* pType = qobject_cast<QComboBox*>(ui->tableMeas->cellWidget(ui->tableMeas->rowCount()-1, COLUMN_TYPE));
    if(pType) {
        pType->setCurrentIndex(pType->findData(a_iType));
    }
    QComboBox* pEvent = qobject_cast<QComboBox*>(ui->tableMeas->cellWidget(ui->tableMeas->rowCount()-1, MEAS_COLUMN_EVENT));
    if(pEvent) {
        pEvent->setCurrentIndex(pEvent->findData(a_iEvent));
    }
    QCheckBox* pCheckBox = qobject_cast<QCheckBox*>(ui->tableMeas->cellWidget(ui->tableMeas->rowCount()-1, MEAS_COLUMN_ENABLE));
    if(pCheckBox) {
        pCheckBox->setChecked(a_bIsEnable);
    }
}

void LElfVarEditorWidget::addCalibration(QString a_strRawName, uint32_t a_dwAddress, int a_iType)
{
    on_btnAddRowCalib_clicked();

    ui->tableCalib->item(ui->tableCalib->rowCount()-1, COLUMN_NAME)->setText(a_strRawName);
    ui->tableCalib->item(ui->tableCalib->rowCount()-1, COLUMN_ADDRESS)->setText("0x"+QString::number(a_dwAddress, 16));
    QComboBox* pType = qobject_cast<QComboBox*>(ui->tableCalib->cellWidget(ui->tableCalib->rowCount()-1, COLUMN_TYPE));
    if(pType) {
        pType->setCurrentIndex(pType->findData(a_iType));
    }
}

QString LElfVarEditorWidget::getElfFilePath() const
{
    return ui->lineEdit->text();
}

void LElfVarEditorWidget::setElfFilePath(const QString &a_rPath)
{
    ui->lineEdit->setText(a_rPath);
}

void LElfVarEditorWidget::receiveAddressFound(QString varName, quint32 address)
{
    // Find in measurements
    QList<QTableWidgetItem *> listMeasItems = ui->tableMeas->findItems(varName, Qt::MatchCaseSensitive);
    foreach(QTableWidgetItem *pMeasItem, listMeasItems) {
        ui->tableMeas->item(pMeasItem->row(), COLUMN_ADDRESS)->setText("0x"+QString::number(address, 16));
    }

    // Find in calibration
    QList<QTableWidgetItem *> listCalibItems = ui->tableCalib->findItems(varName, Qt::MatchCaseSensitive);
    foreach(QTableWidgetItem *pCalibItem, listCalibItems) {
        ui->tableCalib->item(pCalibItem->row(), COLUMN_ADDRESS)->setText("0x"+QString::number(address, 16));
    }
}

void LElfVarEditorWidget::handleFileParseProgress()
{
    ui->progressBarFile->setValue(100*m_pElfParser->getFileParseProgress());

    // Disable for re-parse
    if(ui->progressBarFile->value() > 99) {
        ui->btnParseFile->setEnabled(true);
        m_pTimer->stop();
    }
}

void LElfVarEditorWidget::handleMeasSingleVarParse()
{
    QString strName = ui->tableMeas->item(ui->tableMeas->currentRow(), COLUMN_NAME)->text();
    QPushButton *pBtn = qobject_cast<QPushButton *>(ui->tableMeas->cellWidget(ui->tableMeas->currentRow(), COLUMN_TOOL));

    if(pBtn) pBtn->setEnabled(false);
    emit findAddress(strName);
    // Wait until address found
    QEventLoop loop;
    connect(m_pElfParser, SIGNAL(addressFound(QString, quint32)), &loop, SLOT(quit()));
    loop.exec();
    if(pBtn) pBtn->setEnabled(true);

}

void LElfVarEditorWidget::handleCalibSingleVarParse()
{
    QString strName = ui->tableCalib->item(ui->tableCalib->currentRow(), COLUMN_NAME)->text();

    QPushButton *pBtn = qobject_cast<QPushButton *>(ui->tableCalib->cellWidget(ui->tableCalib->currentRow(), COLUMN_TOOL));

    if(pBtn) pBtn->setEnabled(false);
    emit findAddress(strName);
    // Wait until address found
    QEventLoop loop;
    connect(m_pElfParser, SIGNAL(addressFound(QString, quint32)), &loop, SLOT(quit()));
    loop.exec();
    if(pBtn) pBtn->setEnabled(true);
}

void LElfVarEditorWidget::on_btnParseFile_clicked()
{
    ui->progressBarFile->setValue(0);
    m_pElfParser->parseDwarfOutput(ui->lineEdit->text());
    ui->btnParseFile->setEnabled(false);
    m_pTimer->start();
}

void LElfVarEditorWidget::on_btnParseVars_clicked()
{
    ui->progressBarVars->setValue(0);
    // Disable for re-parse
    ui->btnParseVars->setEnabled(false);

    int varMeasNum = ui->tableMeas->rowCount();
    int varMeasCount = 0;
    int varCalibNum = ui->tableCalib->rowCount();
    int varCalibCount = 0;

    // Parse measurements
    for(varMeasCount=0; varMeasCount<varMeasNum; varMeasCount++) {
        QString strName = ui->tableMeas->item(varMeasCount, COLUMN_NAME)->text();
        emit findAddress(strName);
        // Wait until address found
        QEventLoop loop;
        connect(m_pElfParser, SIGNAL(addressFound(QString, quint32)), &loop, SLOT(quit()));
        loop.exec();
        // Set the progress bar
        ui->progressBarVars->setValue(100.0*(varMeasCount+1)/(varMeasNum+varCalibNum));
    }

    // Parse calibration
    for(varCalibCount=0; varCalibCount<varCalibNum; varCalibCount++) {
        QString strName = ui->tableCalib->item(varCalibCount, COLUMN_NAME)->text();
        emit findAddress(strName);
        // Wait until address found
        QEventLoop loop;
        connect(m_pElfParser, SIGNAL(addressFound(QString, quint32)), &loop, SLOT(quit()));
        loop.exec();
        // Set the progress bar
        ui->progressBarVars->setValue(100.0*(varMeasCount+varCalibCount+1)/(varMeasNum+varCalibNum));
    }

    // Re-enable after finish
    ui->btnParseVars->setEnabled(true);
}

void LElfVarEditorWidget::on_btnDeleteRowMeas_clicked()
{
    ui->tableMeas->removeRow(ui->tableMeas->currentRow());
}

void LElfVarEditorWidget::on_btnAddRowMeas_clicked()
{
    ui->tableMeas->setRowCount(ui->tableMeas->rowCount()+1);

    QTableWidgetItem *pNameItem = new QTableWidgetItem;
    ui->tableMeas->setItem(ui->tableMeas->rowCount()-1, COLUMN_NAME, pNameItem);

    QTableWidgetItem *pAddrItem = new QTableWidgetItem;
    pAddrItem->setFlags(pAddrItem->flags() ^ Qt::ItemIsEditable);
    ui->tableMeas->setItem(ui->tableMeas->rowCount()-1, COLUMN_ADDRESS, pAddrItem);

    addTypeColumn(ui->tableMeas);
    addEventColumn(ui->tableMeas);

    QPushButton *pBtn = new QPushButton;
    pBtn->setText("Parse address");
    connect(pBtn, &QPushButton::clicked, this, &LElfVarEditorWidget::handleMeasSingleVarParse);
    ui->tableMeas->setCellWidget(ui->tableMeas->rowCount()-1, COLUMN_TOOL, pBtn);

    addEnableColumn(ui->tableMeas);
}

void LElfVarEditorWidget::on_btnAddRowCalib_clicked()
{
    ui->tableCalib->setRowCount(ui->tableCalib->rowCount()+1);

    QTableWidgetItem *pNameItem = new QTableWidgetItem;
    ui->tableCalib->setItem(ui->tableCalib->rowCount()-1, COLUMN_NAME, pNameItem);

    QTableWidgetItem *pAddrItem = new QTableWidgetItem;
    pAddrItem->setFlags(pAddrItem->flags() ^ Qt::ItemIsEditable);
    ui->tableCalib->setItem(ui->tableCalib->rowCount()-1, COLUMN_ADDRESS, pAddrItem);

    addTypeColumn(ui->tableCalib);

    QPushButton *pBtn = new QPushButton;
    pBtn->setText("Parse address");
    connect(pBtn, &QPushButton::clicked, this, &LElfVarEditorWidget::handleCalibSingleVarParse);
    ui->tableCalib->setCellWidget(ui->tableCalib->rowCount()-1, COLUMN_TOOL, pBtn);
}

void LElfVarEditorWidget::on_btnDeleteRowCalib_clicked()
{
    ui->tableCalib->removeRow(ui->tableCalib->currentRow());
}

void LElfVarEditorWidget::on_btnApply_clicked()
{
    refreshAllData();

    emit applyAllData();
}

void LElfVarEditorWidget::addTypeColumn(QTableWidget *pTableWidget)
{
    QComboBox* pType = new QComboBox;
    pType->addItem("float", LCcpData::DataType::eFloat);
    pType->addItem("int32_t", LCcpData::DataType::eInt32);
    pType->addItem("uint32_t", LCcpData::DataType::eUint32);
    pType->addItem("int16_t", LCcpData::DataType::eInt16);
    pType->addItem("uint16_t", LCcpData::DataType::eUint16);
    pType->addItem("int8_t", LCcpData::DataType::eInt8);
    pType->addItem("uint8_t", LCcpData::DataType::eUint8);
    pTableWidget->setCellWidget(pTableWidget->rowCount()-1, COLUMN_TYPE, pType);
}

void LElfVarEditorWidget::addEventColumn(QTableWidget *pTableWidget)
{
    QComboBox* pEvent = new QComboBox;
    pEvent->addItem("10ms", EVENT_10MS);
    pEvent->addItem("1ms", EVENT_1MS);
    pEvent->addItem("5ms", EVENT_5MS);
    pEvent->addItem("100ms", EVENT_100MS);
    pTableWidget->setCellWidget(pTableWidget->rowCount()-1, MEAS_COLUMN_EVENT, pEvent);
}

void LElfVarEditorWidget::addEnableColumn(QTableWidget *pTableWidget)
{
    QCheckBox *pCheckBox = new QCheckBox;
    pTableWidget->setCellWidget(pTableWidget->rowCount()-1, MEAS_COLUMN_ENABLE, pCheckBox);
    pCheckBox->setChecked(true); // Default to be true
}

void LElfVarEditorWidget::on_btnBrowse_clicked()
{
    m_strFilePath = QFileDialog::getOpenFileName(this,
                                                 tr("Open File"),
                                                 m_strFilePath,
                                                 tr("ELF file (*.elf *.out)"));
    ui->lineEdit->setText(m_strFilePath);
}

void LElfVarEditorWidget::on_btnSave_clicked()
{
    refreshAllData();

    emit saveSettings();
}
