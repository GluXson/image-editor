#include "histogrampage.h"
#include "ui_histogrampage.h"

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QAbstractAxis>
#include <QAbstractSeries>
#include <QLegend>
#include <QPainter>
#include <QMessageBox>
#include <algorithm>

HistogramPage::HistogramPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HistogramPage)
    , chart(new QChart())
    , chartView(new QChartView(chart, this))
{
    ui->setupUi(this);

    chartView->setRenderHint(QPainter::Antialiasing);
    ui->chartLayout->addWidget(chartView);

    chart->setTitle("Histogram obrazu");
    chart->legend()->setVisible(true);

    if (ui->checkR) {
        ui->checkR->setChecked(true);
    }

    if (ui->checkG) {
        ui->checkG->setChecked(true);
    }

    if (ui->checkB) {
        ui->checkB->setChecked(true);
    }

    if (ui->checkGray) {
        ui->checkGray->setChecked(true);
    }
}

HistogramPage::~HistogramPage()
{
    delete ui;
}

void HistogramPage::updateHistogram(const std::array<int, 256> &histR,
                                    const std::array<int, 256> &histG,
                                    const std::array<int, 256> &histB,
                                    const std::array<int, 256> &histGray)
{
    chart->removeAllSeries();

    const auto oldAxes = chart->axes();
    for (QAbstractAxis *axis : oldAxes) {
        chart->removeAxis(axis);
        delete axis;
    }

    chart->legend()->setVisible(true);
    chart->setTitle("Histogram obrazu");

    int maxValue = 1;
    for (int i = 0; i < 256; ++i) {
        maxValue = std::max(maxValue, histR[i]);
        maxValue = std::max(maxValue, histG[i]);
        maxValue = std::max(maxValue, histB[i]);
        maxValue = std::max(maxValue, histGray[i]);
    }

    if (ui->checkR && ui->checkR->isChecked()) {
        QLineSeries *series = new QLineSeries();
        series->setName("Czerwony");
        series->setColor(Qt::red);
        for (int i = 0; i < 256; ++i) {
            series->append(i, histR[i]);
        }
        chart->addSeries(series);
    }

    if (ui->checkG && ui->checkG->isChecked()) {
        QLineSeries *series = new QLineSeries();
        series->setName("Zielony");
        series->setColor(Qt::green);
        for (int i = 0; i < 256; ++i) {
            series->append(i, histG[i]);
        }
        chart->addSeries(series);
    }

    if (ui->checkB && ui->checkB->isChecked()) {
        QLineSeries *series = new QLineSeries();
        series->setName("Niebieski");
        series->setColor(Qt::blue);
        for (int i = 0; i < 256; ++i) {
            series->append(i, histB[i]);
        }
        chart->addSeries(series);
    }

    if (ui->checkGray && ui->checkGray->isChecked()) {
        QLineSeries *series = new QLineSeries();
        series->setName("Szary");
        series->setColor(QColor(90, 90, 90));
        for (int i = 0; i < 256; ++i) {
            series->append(i, histGray[i]);
        }
        chart->addSeries(series);
    }

    QValueAxis *axisX = new QValueAxis();
    axisX->setRange(0, 255);
    axisX->setTitleText("Poziom jasności");
    axisX->setLabelFormat("%.0f");
    axisX->setTickCount(6);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, maxValue);
    axisY->setTitleText("Liczba pikseli");
    axisY->setLabelFormat("%.0f");
    axisY->setTickCount(6);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    const auto seriesList = chart->series();
    for (QAbstractSeries *series : seriesList) {
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }
}

void HistogramPage::on_checkR_toggled(bool checked)
{
    Q_UNUSED(checked);
    emit histogramDisplayChanged();
}

void HistogramPage::on_checkG_toggled(bool checked)
{
    Q_UNUSED(checked);
    emit histogramDisplayChanged();
}

void HistogramPage::on_checkB_toggled(bool checked)
{
    Q_UNUSED(checked);
    emit histogramDisplayChanged();
}

void HistogramPage::on_checkGray_toggled(bool checked)
{
    Q_UNUSED(checked);
    emit histogramDisplayChanged();
}

void HistogramPage::on_btnStretchHistogram_clicked()
{
    emit stretchHistogramRequested();
}

void HistogramPage::on_btnEqualizeHistogram_clicked()
{
    emit equalizeHistogramRequested();
}

void HistogramPage::on_btnHistogramInfo_clicked()
{
    QMessageBox::information(
        this,
        "Info o panelu",
        "Pliki obsługujące panel:\n"
        "- histogramhandler.h - deklaracje operacji histogramowych\n"
        "- histogramhandler.cpp - logika operacji histogramowych\n\n"
        "Działania:\n"
        "- Odświeżenie histogramu - ponowne przeliczenie i wyświetlenie kanałów obrazu - funkcja: refreshHistogramIfNeeded | plik: histogramhandler.cpp\n"
        "- Rozciągnięcie histogramu - rozszerzenie zakresu jasności obrazu - funkcja: applyHistogramStretch | plik: histogramhandler.cpp\n"
        "- Wyrównanie histogramu - wyrównanie rozkładu jasności obrazu - funkcja: applyHistogramEqualization | plik: histogramhandler.cpp\n\n"
        "Kanały wyświetlania:\n"
        "- Czerwony - histogram składowej czerwonej obrazu\n"
        "- Zielony - histogram składowej zielonej obrazu\n"
        "- Niebieski - histogram składowej niebieskiej obrazu\n"
        "- Szary - histogram jasności obrazu w skali szarości"
        );
}