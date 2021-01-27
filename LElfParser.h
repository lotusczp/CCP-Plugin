#ifndef LELFPARSER_H
#define LELFPARSER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QProcess>

class LElfParser : public QObject
{
    Q_OBJECT

public:
    LElfParser();
    ~LElfParser();

    void parseDwarfOutput(const QString& a_rstrFilePath);

    bool parseFinish() const {return m_bParseState==eParseFinish;}

    float getFileParseProgress() const;

signals:
    void addressFound(QString varName, quint32 address);

public slots:
    void findAddress(QString a_strVarName);

private:
    struct DwarfArrayElem
    {
        uint32_t    address;
        QString     deepth;
        QString     name;
        QString     value;
        int         level;
    };

    struct TypeInfo
    {
        int index;
        QJsonObject info;
    };

    enum ParseState
    {
        eUnknown,
        eStartObjdump,
        eReadFinish,
        eParseFinish
    };

private slots:
    void readInfo();

    void readError();

    void processFinish(int);

private:
    QJsonObject getDwarfVar(const QString& a_rstrName);

    TypeInfo getDwarfType(uint32_t typeAddress);

private:
    QProcess    *m_pObjdumpProc;
    qint64      m_iBytes;
    qint64      m_iTotalBytes;
    QString     m_strContent;
    QVector<DwarfArrayElem> m_dwarfArray;
    ParseState        m_bParseState;
    double      m_dParseLinePercent;
};

#endif // LELFPARSER_H
