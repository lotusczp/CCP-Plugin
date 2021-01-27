#include "LCcpToolWidget.h"
#include "ui_LCcpToolWidget.h"

LCcpToolWidget::LCcpToolWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCcpToolWidget)
{
    ui->setupUi(this);
}

LCcpToolWidget::~LCcpToolWidget()
{
    delete ui;
}

LElfVarEditorWidget *LCcpToolWidget::getElfVarEditorWidget()
{
    return ui->widget;
}
