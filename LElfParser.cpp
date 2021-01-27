#include "LElfParser.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QThread>
#include <QDebug>

LElfParser::LElfParser()
{
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    m_pObjdumpProc = new QProcess;
    connect(m_pObjdumpProc, &QProcess::readyReadStandardOutput, this, &LElfParser::readInfo);
    connect(m_pObjdumpProc, &QProcess::readyReadStandardError, this, &LElfParser::readError);
    connect(m_pObjdumpProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &LElfParser::processFinish);

    m_bParseState = eUnknown;
    m_dParseLinePercent = 0;
}

LElfParser::~LElfParser()
{
    m_pObjdumpProc->kill();
    delete m_pObjdumpProc;
}

void LElfParser::parseDwarfOutput(const QString &a_rstrFilePath)
{
    QFileInfo fileInfo(a_rstrFilePath);
    m_iTotalBytes = fileInfo.size();
    m_iBytes = 0;
    m_dParseLinePercent = 0;
    m_strContent.clear();
    m_dwarfArray.clear();

    QStringList arguments;
    arguments << "--dwarf=info"<<a_rstrFilePath;
    m_pObjdumpProc->start("./thirdparty/objdump.exe", arguments);
    m_bParseState = eStartObjdump;
}

float LElfParser::getFileParseProgress() const
{
    double retval = 0;
    switch(m_bParseState)
    {
    case eUnknown:
        retval = 0;
        break;
    case eStartObjdump:
        retval = ((double) m_iBytes) / ((double) m_iTotalBytes*5) * 0.5;
        if(retval > 0.5) {
            retval = 0.5;
        }
        break;
    case eReadFinish:
        retval = 0.5+m_dParseLinePercent*0.5;
        break;
    case eParseFinish:
        retval = 1.0;
        break;
    default:
        break;
    }

    return (float)retval;
}

void LElfParser::findAddress(QString a_strVarName)
{
    if(a_strVarName.contains(".")) {
        // Struct
        QStringList structPath = a_strVarName.split('.');
        QJsonObject foundVar = getDwarfVar(structPath[0]);
        uint32_t address = foundVar["address"].toString().toUInt(nullptr, 16);
        QJsonObject subType = foundVar;
        int matchLevel = 1;
        for(int i=1; i<structPath.size(); i++) {
            QString strSubLevel = structPath[i];
            for(int count=0; count<subType["type"].toObject()["countElements"].toInt(); count++) {
                if(strSubLevel == subType["type"].toObject()[QString::number(count)].toObject()["name"].toString()) {
                    address += subType["type"].toObject()[QString::number(count)].toObject()["offset"].toString().toUInt(nullptr, 10);
                    matchLevel = matchLevel+1;
                    subType = subType["type"].toObject()[QString::number(count)].toObject()["type"].toObject();
                    break;
                }
            }
        }
        if(matchLevel != structPath.size()) {
            emit addressFound(a_strVarName, 0);
            return;
        }
        emit addressFound(a_strVarName, address);
        return;
    }
    else {
        // normal variable
        QJsonObject foundVar = getDwarfVar(a_strVarName);
        if(foundVar.contains("address")) {
            emit addressFound(a_strVarName, foundVar["address"].toString().toUInt(nullptr, 16));
            return;
        }
        else {
            emit addressFound(a_strVarName, 0);
            return;
        }
    }
}

void LElfParser::readInfo()
{
    m_iBytes += m_pObjdumpProc->bytesAvailable();
    m_strContent += m_pObjdumpProc->readAllStandardOutput();
}

void LElfParser::readError()
{
    qDebug() << m_pObjdumpProc->readAllStandardError();
}

void LElfParser::processFinish(int )
{
    m_bParseState = eReadFinish;
    QStringList lines = m_strContent.split("\n");
    for(int i=0; i<lines.size(); i++) {
        m_dParseLinePercent = (double)i / lines.size();
        QString strLine = lines[i];
        QString strStripLine = strLine.trimmed();
        if(strStripLine.startsWith("<")) {
            int level = 0;
            QRegularExpression rx1(".*<([0-9a-f]+)>(.*)");
            QRegularExpressionMatch match1 = rx1.match(strStripLine);
            if(match1.hasMatch()) {
                QStringList first = match1.capturedTexts();
                QString strAddr = first[1];
                QString strTuple = first[2];
                QString strDeepth = "0";
                QRegularExpression rx2(".*<([0-9a-f]+)><([0-9a-f]+)>:(.*)");
                QRegularExpressionMatch match2 = rx2.match(strStripLine);
                if(match2.hasMatch()) {
                    QStringList second = match2.capturedTexts();
                    level = 1;
                    strDeepth = second[1];
                    strAddr = second[2];
                    strTuple = second[3];
                }
                QStringList tupleArray = strTuple.split(':');
                QString strValue = strTuple.remove(0, tupleArray[0].size()+1).trimmed();
                DwarfArrayElem elem;
                elem.address = strAddr.toUInt(nullptr, 16);
                elem.deepth = strDeepth;
                elem.name = tupleArray[0].trimmed();
                elem.value = strValue;
                elem.level = level;
                m_dwarfArray.append(elem);
            }
        }
    }

    m_bParseState = eParseFinish;

}

QJsonObject LElfParser::getDwarfVar(const QString &a_rstrName)
{
    int iFoundArray = -1;
    QString strAddress = "";
    int iStruct = 0;
    for(int i=0; i<m_dwarfArray.size(); i++) {
        // Array or variable name
        if(m_dwarfArray[i].name == "DW_AT_name" && m_dwarfArray[i].value == a_rstrName) {
            iFoundArray = i;
            // goto top of this DIE
            while(i > 0 && m_dwarfArray[i].level != 1) {
                i -= 1;
            }
            i += 1;
            // walk through the whole DIE
            while(i < m_dwarfArray.size()
                  && m_dwarfArray[i].level != 1){
                if(m_dwarfArray[i].name == "DW_AT_location") {
                    iStruct = 1;
                    QRegularExpression rx(".*\\(DW_OP_addr\\:\\ *([0-9a-f]+)\\).*");
                    QRegularExpressionMatch match = rx.match(m_dwarfArray[i].value);
                    if(match.hasMatch()) {
                        strAddress = match.capturedTexts()[1];
                    }
                    break;
                }

                if(m_dwarfArray[i].name == "DW_AT_declaration" && m_dwarfArray[i].value == "1") {
                    // skip declaration
                    iFoundArray = -1;
                    break;
                }

                i += 1;
            }

            if(iFoundArray != -1){
                break;
            }
        }
    }

    if(iFoundArray != -1){
        QString strCurrentDepth = m_dwarfArray[iFoundArray].deepth;
        int i = iFoundArray;
        // goto top of this DIE
        while(i > 0 && m_dwarfArray[i].level != 1) {
            i -= 1;
        }
        i += 1;

        while(m_dwarfArray[i].deepth == strCurrentDepth
              && m_dwarfArray[i].level != 1
              && m_dwarfArray[i].name != "DW_AT_type") {
            i+=1;
        }

        // get type DIE-address
        uint32_t uiTypeAddress = m_dwarfArray[i].value.mid(1, m_dwarfArray[i].value.size()-2).toUInt(nullptr, 0);

        // get type at DIE-address
        TypeInfo type = getDwarfType(uiTypeAddress);
        if(iStruct == 1) {
            type.info["struct"] = 1;
        }
        type.info["address"] = strAddress;
        return type.info;
    }

    return QJsonObject(); // Can not reach here
}

LElfParser::TypeInfo LElfParser::getDwarfType(uint32_t typeAddress)
{
    // Find type at DIE-Address
    int iTypeFound = 0;
    TypeInfo retVal;
    int countElements = 0;
    for(int i=0; i<m_dwarfArray.size(); i++) {
        // Array or variable name
        if(m_dwarfArray[i].address == typeAddress) {
            iTypeFound = i;
            while(i < m_dwarfArray.size()) {
                if(m_dwarfArray[i].value.contains("DW_TAG_array_type")) {
                    // array type found
                    retVal.info["array"] = "1";
                }
                else if(m_dwarfArray[i].name == "DW_AT_name") {
                    // name of type
                    if(m_dwarfArray[i].value.contains("): ")) {
                        //! \note   .out file format special
                        QString strTmp = m_dwarfArray[i].value;
                        retVal.info["name"] = strTmp.remove(0, strTmp.indexOf("): ")+3);
                    }
                    else {
                        retVal.info["name"] = m_dwarfArray[i].value;
                    }
                }
                else if(m_dwarfArray[i].name == "DW_AT_type") {
                    // type of type, proceed until base type is found
                    QString strTmp = m_dwarfArray[i].value;
                    TypeInfo dummy = getDwarfType(strTmp.remove(0,1).remove(strTmp.size()-2,1).toUInt(nullptr, 0));
                    retVal.info["type"] = dummy.info;
                }
                else if(m_dwarfArray[i].name == "DW_AT_byte_size") {
                    // size of type
                    retVal.info["size"] = m_dwarfArray[i].value;
                }
                else if(m_dwarfArray[i].name == "DW_AT_data_member_location") {
                    // structre element found at location:
                    QRegularExpression rx(".*DW_OP_plus_uconst:(.*)\\)");
                    QRegularExpressionMatch match = rx.match(m_dwarfArray[i].value);
                    if(match.hasMatch()) {
                        retVal.info["offset"] = match.capturedTexts()[1];
                    }
                }

                i++;

                // Search for sub-DIEs (structure elements)
                while(i < m_dwarfArray.size()
                      && m_dwarfArray[i].level == 1
                      && m_dwarfArray[i].deepth.toInt() > m_dwarfArray[iTypeFound].deepth.toInt()) {
                    TypeInfo dummy = getDwarfType(m_dwarfArray[i].address);
                    i = dummy.index;
                    if(dummy.info.contains("name")) {
                        // name of structure element
                        retVal.info[QString::number(countElements)] = dummy.info;
                        countElements = countElements+1;
                    }
                }

                if(i < m_dwarfArray.size()
                      && m_dwarfArray[i].level == 1
                      && m_dwarfArray[i].deepth.toInt() <= m_dwarfArray[iTypeFound].deepth.toInt()) {
                    // probably all subelements found, so save the count
                    retVal.info["countElements"] = countElements;
                    break;
                }
            }
            retVal.index = i;
            break;
        }
    }

    return retVal;
}
