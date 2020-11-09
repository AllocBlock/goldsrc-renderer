#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_GoldsrcRenderer.h"

class GoldsrcRenderer : public QMainWindow
{
    Q_OBJECT

public:
    GoldsrcRenderer(QWidget *parent = Q_NULLPTR);

private:
    Ui::GoldsrcRendererClass ui;
};
