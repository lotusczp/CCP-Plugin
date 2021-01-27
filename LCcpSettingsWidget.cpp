#include "LCcpSettingsWidget.h"
#include "ui_LCcpSettingsWidget.h"
#include <QMessageBox>

LCcpSettingsWidget::LCcpSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCcpSettingsWidget)
{
    ui->setupUi(this);

    ui->devTypeBox->addItem("USBCAN2", VCI_USBCAN2);
    ui->devTypeBox->addItem("USBCAN2A", VCI_USBCAN2A);
    ui->devTypeBox->addItem("USBCAN_2E_U", VCI_USBCAN_2E_U);

    ui->devTypeBox->setCurrentIndex(0);

    m_pCanDataManager = nullptr;

    ui->buttonConnect->setEnabled(true);
    ui->buttonDisconnect->setEnabled(false);
}

LCcpSettingsWidget::~LCcpSettingsWidget()
{
    delete ui;
}

void LCcpSettingsWidget::setCanDataManager(LCanDataManager *a_pCanDataManager)
{
    m_pCanDataManager = a_pCanDataManager;
}

void LCcpSettingsWidget::on_buttonConnect_clicked()
{
    if(m_pCanDataManager == nullptr) return;

    unsigned long deviceType = ui->devTypeBox->currentData().toInt();
    unsigned long deviceIndex = ui->devIndexBox->currentText().toInt();
    unsigned long canIndex = ui->channelIndexBox->currentText().toInt();
    // open CAN device
    unsigned long dwIsOpen = m_pCanDataManager->openDevice(deviceIndex, deviceType);
    if(dwIsOpen == STATUS_OK) {
        // init device
        if(deviceType == VCI_USBCAN_2E_U
                || deviceType == VCI_USBCAN_E_U) {
            // Should use setReference to set the baudrate and filters
            DWORD baudrate = 0x060007; // 500Kbps
            if(m_pCanDataManager->setReference(0, (void*)&baudrate, deviceIndex, canIndex, deviceType) != STATUS_OK) {
                QMessageBox::warning(0, QObject::tr("Warning"), "Cgd Plugin failed to setReference to USBCAN device");
                return;
            }
        }
        if(m_pCanDataManager->initCAN(eCanBaudRate_500Kbps, deviceIndex, canIndex, deviceType) == 1) {
            unsigned long dwStarted = m_pCanDataManager->startCAN(deviceIndex, canIndex, deviceType);
            if (dwStarted == STATUS_OK) {
                m_pCanDataManager->startReceive(true);
                deviceConnected();
            }
            else {
                m_pCanDataManager->closeDevice(deviceIndex);
                QMessageBox::warning(0, QObject::tr("Warning"), "Cgd Plugin failed to start USBCAN device");
            }
        }
        else {
            m_pCanDataManager->closeDevice();
            QMessageBox::warning(0, QObject::tr("Warning"), "Cgd Plugin failed to initialize USBCAN device");
        }
    }
    else {
        unsigned int errorCode;
        m_pCanDataManager->readErrInfo(&errorCode, deviceIndex, -1, deviceType);
        if(errorCode == ERR_DEVICEOPENED) {
            // Already opened by other plugin, we have to trust it
            deviceConnected();
        }
        else {
            QMessageBox::warning(0, QObject::tr("Warning"), "Cgd Plugin can not open USBCAN device with device index " + QString::number(deviceIndex) + " and Channel index " + QString::number(canIndex));
        }
    }
}

void LCcpSettingsWidget::on_buttonDisconnect_clicked()
{
    if(m_pCanDataManager == nullptr) return;

    unsigned long deviceIndex = ui->devIndexBox->currentText().toInt();
    unsigned long canIndex = ui->channelIndexBox->currentText().toInt();
    unsigned long deviceType = ui->devTypeBox->currentData().toInt();
    emit sendDeviceReady(false, deviceIndex, canIndex, deviceType);
    m_pCanDataManager->startReceive(false);
    m_pCanDataManager->closeDevice(deviceIndex);
    ui->buttonConnect->setEnabled(true);
    ui->buttonDisconnect->setEnabled(false);
}

void LCcpSettingsWidget::deviceConnected()
{
    unsigned long deviceIndex = ui->devIndexBox->currentText().toInt();
    unsigned long canIndex = ui->channelIndexBox->currentText().toInt();
    unsigned long deviceType = ui->devTypeBox->currentData().toInt();
    emit sendDeviceReady(true, deviceIndex, canIndex, deviceType);
    ui->buttonConnect->setEnabled(false);
    ui->buttonDisconnect->setEnabled(true);
}
