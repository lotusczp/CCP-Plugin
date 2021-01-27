#ifndef LCCPTOOLWIDGET_H
#define LCCPTOOLWIDGET_H

#include <QWidget>
#include "LElfVarEditorWidget.h"

namespace Ui {
class LCcpToolWidget;
}

class LCcpToolWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LCcpToolWidget(QWidget *parent = nullptr);
    ~LCcpToolWidget();

    LElfVarEditorWidget* getElfVarEditorWidget();

private:
    Ui::LCcpToolWidget *ui;
};

#endif // LCCPTOOLWIDGET_H
