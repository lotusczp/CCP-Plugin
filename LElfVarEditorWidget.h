#ifndef LELFVAREDITORWIDGET_H
#define LELFVAREDITORWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QThread>
#include <QMap>
#include <QTableWidget>
#include "LElfParser.h"
#include "LCcpData.h"

namespace Ui {
class LElfVarEditorWidget;
}

class LElfVarEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LElfVarEditorWidget(QWidget *parent = nullptr);
    ~LElfVarEditorWidget();

    void setDataMap(LCcpDataMap *a_pMapAllData) {m_pMapAllData = a_pMapAllData;}

    void refreshAllData();

    void clearAll();

    void addMeasurement(QString a_strRawName, uint32_t a_dwAddress, int a_iType, int a_iEvent, bool a_bIsEnable);

    void addCalibration(QString a_strRawName, uint32_t a_dwAddress, int a_iType);

    QString getElfFilePath() const;

    void setElfFilePath(const QString &a_rPath);

signals:
    void sendVarToParse(QString name);

    void findAddress(QString a_strVarName);

    void applyAllData();

    void saveSettings();

public slots:
    void receiveAddressFound(QString varName, quint32 address);

private slots:
    void handleFileParseProgress();

    void handleMeasSingleVarParse();

    void handleCalibSingleVarParse();

    void on_btnParseFile_clicked();

    void on_btnParseVars_clicked();

    void on_btnDeleteRowMeas_clicked();

    void on_btnAddRowMeas_clicked();

    void on_btnAddRowCalib_clicked();

    void on_btnDeleteRowCalib_clicked();

    void on_btnApply_clicked();

    void on_btnBrowse_clicked();

    void on_btnSave_clicked();

private:
    void addTypeColumn(QTableWidget* pTableWidget);

    void addEventColumn(QTableWidget* pTableWidget);

    void addEnableColumn(QTableWidget* pTableWidget);

private:
    Ui::LElfVarEditorWidget *ui;

    LElfParser      *m_pElfParser;
    QTimer          *m_pTimer;
    QThread         *m_pThread;
    LCcpDataMap     *m_pMapAllData;
    QString         m_strFilePath;
};

#endif // LELFVAREDITORWIDGET_H
