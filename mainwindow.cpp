#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->widget, &LineChart::signalSelectRangeChanged, this, [=](int start, int end) {
        if (start != end)
            ui->label->setText("选中：" + QString::number(start) + " ~ " + QString::number(end));
        else
            ui->label->setText("位置：" + QString::number(start));
    });


    {
        ChartData data;
        data.title = "一条线";
        data.color = QColor("#6495ED");
        data.xMin = data.yMin = 0;
        data.xMax = 100;
        data.yMax = 100;
        for (int i = 0; i <= 20; i++)
        {
            data.points.append(QPoint(i * 5, qrand() % 80 + 20));
            // data.xLabels.append(QString::number(i * 5));
        }

        ui->widget->addLine(data);
    }

    {
        ChartData data;
        data.title = "一条线";
        data.color = QColor("#0DBF8C");
        data.xMin = data.yMin = 0;
        data.xMax = 100;
        data.yMax = 100;
        for (int i = 0; i <= 20; i++)
        {
            data.points.append(QPoint(i * 5, qrand() % 30));
        }
        ui->widget->addLine(data);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    x += 5;
    ui->widget->addPoint(0, x, qrand() % 60 + 20);
    ui->widget->addPoint(1, x, qrand() % 30);
}

void MainWindow::on_pushButton_2_clicked()
{
    for (int i = 0; i < ui->widget->lineCount(); i++)
        ui->widget->removeFirst(i);
}

void MainWindow::on_pushButton_3_clicked()
{
    x += 5;
    ui->widget->addPoint(0, x, qrand() % 60 + 20);
    ui->widget->addPoint(1, x, qrand() % 30);

    for (int i = 0; i < ui->widget->lineCount(); i++)
        ui->widget->removeFirst(i);
}
