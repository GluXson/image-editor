#include "binarizationpage.h"
#include "ui_binarizationpage.h"

#include <QtCharts/QAbstractAxis>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QFrame>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

#include <algorithm>

BinarizationPage::BinarizationPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::BinarizationPage)
    , chart(new QChart)
    , chartView(new QChartView(chart, this))
{
    ui->setupUi(this);

    chartView->setRenderHint(QPainter::Antialiasing, false);
    chartView->setFrameShape(QFrame::NoFrame);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (ui->chartLayout) {
        ui->chartLayout->setContentsMargins(0, 0, 0, 0);
        ui->chartLayout->setSpacing(0);
        ui->chartLayout->addWidget(chartView);
    }

    chart->setTitle("Histogram jasności");
    chart->legend()->setVisible(false);
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(4, 4, 4, 4));

    ui->thresholdSlider->setRange(0, 255);
    ui->thresholdSpinBox->setRange(0, 255);
    ui->thresholdSlider->setValue(0);
    ui->thresholdSpinBox->setValue(0);

    connect(ui->thresholdSlider, &QSlider::valueChanged,
            this, &BinarizationPage::onThresholdSliderChanged);
    connect(ui->thresholdSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &BinarizationPage::onThresholdSpinChanged);
    connect(ui->applyManualButton, &QPushButton::clicked,
            this, &BinarizationPage::onApplyManualClicked);
    connect(ui->applyOtsuButton, &QPushButton::clicked,
            this, &BinarizationPage::onApplyOtsuClicked);

    if (ui->resetThresholdButton) {
        connect(ui->resetThresholdButton, &QPushButton::clicked,
                this, &BinarizationPage::onResetThresholdClicked);
    }

    if (ui->backButton) {
        connect(ui->backButton, &QPushButton::clicked,
                this, &BinarizationPage::onBackClicked);
    }

    updateLabels();
    setImageLoaded(false);
}

BinarizationPage::~BinarizationPage()
{
    delete ui;
}

void BinarizationPage::setImageLoaded(bool loaded)
{
    imageLoaded = loaded;

    ui->thresholdSlider->setEnabled(loaded);
    ui->thresholdSpinBox->setEnabled(loaded);
    ui->applyManualButton->setEnabled(loaded);
    ui->applyOtsuButton->setEnabled(loaded);

    if (ui->resetThresholdButton) {
        ui->resetThresholdButton->setEnabled(loaded);
    }

    if (!loaded) {
        ui->infoLabel->setText("Wczytaj obraz, aby wykonać binaryzację.");
        ui->otsuValueLabel->setText("Próg Otsu: —");

        chart->removeAllSeries();

        const auto axes = chart->axes();
        for (QAbstractAxis *axis : axes) {
            chart->removeAxis(axis);
            delete axis;
        }
    } else {
        updateLabels();
    }
}

void BinarizationPage::setThreshold(int value)
{
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }

    syncInProgress = true;
    ui->thresholdSlider->setValue(value);
    ui->thresholdSpinBox->setValue(value);
    syncInProgress = false;

    updateLabels();
}

int BinarizationPage::threshold() const
{
    return ui->thresholdSlider->value();
}

void BinarizationPage::setOtsuPreview(int value)
{
    otsuPreviewValue = value;
    updateLabels();
}

void BinarizationPage::onThresholdSliderChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    syncInProgress = true;
    ui->thresholdSpinBox->setValue(value);
    syncInProgress = false;

    updateLabels();
}

void BinarizationPage::onThresholdSpinChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    syncInProgress = true;
    ui->thresholdSlider->setValue(value);
    syncInProgress = false;

    updateLabels();
}

void BinarizationPage::onApplyManualClicked()
{
    if (!imageLoaded) {
        return;
    }

    emit manualThresholdRequested(threshold());
}

void BinarizationPage::onApplyOtsuClicked()
{
    if (!imageLoaded) {
        return;
    }

    emit otsuRequested();
}

void BinarizationPage::onResetThresholdClicked()
{
    if (!imageLoaded) {
        return;
    }

    setThreshold(0);
}

void BinarizationPage::onBackClicked()
{
    QMessageBox::information(
        this,
        QStringLiteral("Info o panelu"),
        QStringLiteral(
            "Pliki obsługujące panel:\n"
            "- binarizationhandler.h - deklaracje operacji binaryzacji\n"
            "- binarizationhandler.cpp - logika operacji binaryzacji\n\n"
            "Działania:\n"
            "- Progowanie ręczne - binaryzacja obrazu według zadanego progu - funkcja: applyManualThreshold | plik: binarizationhandler.cpp\n"
            "- Wyznaczenie progu Otsu - automatyczne obliczenie progu binaryzacji - funkcja: applyOtsuThreshold | plik: binarizationhandler.cpp\n"
            "- Podgląd histogramu - wyświetlenie rozkładu jasności obrazu - funkcja: refreshPage | plik: binarizationhandler.cpp\n"
            "- Aktualizacja podglądu progu Otsu - wyświetlenie bieżącej wartości automatycznej - funkcja: setOtsuPreview | plik: binarizationpage.cpp\n"
            "- Ręczne ustawienie progu - zmiana wartości suwakiem lub polem liczbowym - obsługa w panelu: BinarizationPage\n"
            "- Reset progu - przywrócenie wartości 0 - obsługa w panelu: BinarizationPage"
            )
        );
}

void BinarizationPage::updateLabels()
{
    if (otsuPreviewValue >= 0) {
        ui->otsuValueLabel->setText(QString("Próg Otsu: %1").arg(otsuPreviewValue));
    } else {
        ui->otsuValueLabel->setText("Próg Otsu: —");
    }

    if (imageLoaded) {
        ui->infoLabel->setText("Obraz jest zamieniany do skali szarości, a następnie binaryzowany według zadanego progu lub progu Otsu.");
    }
}

void BinarizationPage::updateHistogram(const std::array<int, 256>& histGray)
{
    chart->removeAllSeries();

    const auto oldAxes = chart->axes();
    for (QAbstractAxis *axis : oldAxes) {
        chart->removeAxis(axis);
        delete axis;
    }

    auto *series = new QLineSeries();
    QPen pen(QColor(52, 152, 219));
    pen.setWidth(2);
    series->setPen(pen);

    int maxValue = 1;
    for (int i = 0; i < 256; ++i) {
        series->append(i, histGray[i]);
        maxValue = std::max(maxValue, histGray[i]);
    }

    chart->addSeries(series);

    auto *axisX = new QCategoryAxis();
    axisX->setRange(0, 255);
    axisX->append("0", 0);
    axisX->append("128", 128);
    axisX->append("255", 255);
    axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisX->setGridLineVisible(true);
    axisX->setTitleText("Jasność");

    auto *axisY = new QValueAxis();
    axisY->setRange(0, std::max(1, static_cast<int>(maxValue * 1.1)));
    axisY->setTickCount(4);
    axisY->setMinorTickCount(0);
    axisY->setLabelFormat("%.0f");
    axisY->setTitleText("Liczba");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    series->attachAxis(axisX);
    series->attachAxis(axisY);
}