#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ChartData data;
    data.title = "一条线";
    data.color = Qt::blue;
    data.xMin = data.yMin = 0;
    data.xMax = 100;
    data.yMax = 100;
    for (int i = 0; i <= 20; i++)
    {
        data.points.append(QPoint(i * 5, qrand() % 60 + 20));
    }

    ui->widget->addData(data);
}

MainWindow::~MainWindow()
{
    delete ui;
}

