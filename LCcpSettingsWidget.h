#ifndef LCCPSETTINGSWIDGET_H
#define LCCPSETTINGSWIDGET_H

#include <QWidget>
#include "LUniqueResource.h"

namespace Ui {
class LCcpSettingsWidget;
}

class LCcpSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LCcpSettingsWidget(QWidget *parent = nullptr);
    ~LCcpSettingsWidget();

    void setCanDataManager(LCanDataManager *a_pCanDataManager);

signals:
    void sendDeviceReady(bool isReady, unsigned long deviceIndex, unsigned long canIndex, unsigned long devType);

private slots:
    void on_buttonConnect_clicked();

    void on_buttonDisconnect_clicked();

private:
    void deviceConnected();

private:
    Ui::LCcpSettingsWidget *ui;
    LCanDataManager         *m_pCanDataManager;
};

#endif // LCCPSETTINGSWIDGET_H
