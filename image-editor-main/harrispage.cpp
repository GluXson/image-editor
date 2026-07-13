#include "harrispage.h"
#include "ui_harrispage.h"

#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QtGlobal>

HarrisPage::HarrisPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HarrisPage)
{
    ui->setupUi(this);

    setupUiState();

    connect(ui->blockSizeSlider,
            &QSlider::valueChanged,
            this,
            &HarrisPage::onBlockSizeSliderChanged);

    connect(ui->blockSizeSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &HarrisPage::onBlockSizeSpinChanged);

    connect(ui->sobelKernelSlider,
            &QSlider::valueChanged,
            this,
            &HarrisPage::onSobelKernelSliderChanged);

    connect(ui->sobelKernelSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &HarrisPage::onSobelKernelSpinChanged);

    connect(ui->kSlider,
            &QSlider::valueChanged,
            this,
            &HarrisPage::onKSliderChanged);

    connect(ui->kDoubleSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &HarrisPage::onKSpinChanged);

    connect(ui->thresholdSlider,
            &QSlider::valueChanged,
            this,
            &HarrisPage::onThresholdSliderChanged);

    connect(ui->thresholdSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &HarrisPage::onThresholdSpinChanged);

    connect(ui->applyButton,
            &QPushButton::clicked,
            this,
            &HarrisPage::onApplyClicked);

    connect(ui->btnResetHarris,
            &QPushButton::clicked,
            this,
            &HarrisPage::onResetClicked);

    connect(ui->btnPanelInfo,
            &QPushButton::clicked,
            this,
            &HarrisPage::onPanelInfoClicked);

    setImageLoaded(false);
}

HarrisPage::~HarrisPage()
{
    delete ui;
}

void HarrisPage::setImageLoaded(bool loaded)
{
    imageLoaded = loaded;

    ui->blockSizeSlider->setEnabled(loaded);
    ui->blockSizeSpinBox->setEnabled(loaded);

    ui->sobelKernelSlider->setEnabled(loaded);
    ui->sobelKernelSpinBox->setEnabled(loaded);

    ui->kSlider->setEnabled(loaded);
    ui->kDoubleSpinBox->setEnabled(loaded);

    ui->thresholdSlider->setEnabled(loaded);
    ui->thresholdSpinBox->setEnabled(loaded);

    ui->applyButton->setEnabled(loaded);
    ui->btnResetHarris->setEnabled(loaded);

    updateInfoLabel();
}

void HarrisPage::setupUiState()
{
    ui->blockSizeSpinBox->setRange(2, 15);
    ui->blockSizeSpinBox->setSingleStep(1);
    ui->blockSizeSpinBox->setValue(3);

    ui->blockSizeSlider->setRange(2, 15);
    ui->blockSizeSlider->setSingleStep(1);
    ui->blockSizeSlider->setPageStep(1);
    ui->blockSizeSlider->setValue(3);

    ui->sobelKernelSpinBox->setRange(3, 7);
    ui->sobelKernelSpinBox->setSingleStep(2);
    ui->sobelKernelSpinBox->setValue(3);

    ui->sobelKernelSlider->setRange(3, 7);
    ui->sobelKernelSlider->setSingleStep(2);
    ui->sobelKernelSlider->setPageStep(2);
    ui->sobelKernelSlider->setValue(3);

    ui->kDoubleSpinBox->setRange(0.01, 0.20);
    ui->kDoubleSpinBox->setSingleStep(0.01);
    ui->kDoubleSpinBox->setDecimals(2);
    ui->kDoubleSpinBox->setValue(0.04);

    ui->kSlider->setRange(1, 20);
    ui->kSlider->setSingleStep(1);
    ui->kSlider->setPageStep(1);
    ui->kSlider->setValue(sliderFromK(0.04));

    ui->thresholdSpinBox->setRange(0, 255);
    ui->thresholdSpinBox->setSingleStep(1);
    ui->thresholdSpinBox->setValue(125);

    ui->thresholdSlider->setRange(0, 255);
    ui->thresholdSlider->setSingleStep(1);
    ui->thresholdSlider->setPageStep(5);
    ui->thresholdSlider->setValue(125);

    updateTitles();
    updateInfoLabel();
}

double HarrisPage::kFromSlider(int value) const
{
    const int clamped = qBound(1, value, 20);
    return static_cast<double>(clamped) / 100.0;
}

int HarrisPage::sliderFromK(double value) const
{
    const double clamped = qBound(0.01, value, 0.20);
    return qBound(1, qRound(clamped * 100.0), 20);
}

void HarrisPage::onBlockSizeSliderChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    const int corrected = qBound(2, value, 15);

    syncInProgress = true;
    ui->blockSizeSlider->setValue(corrected);
    ui->blockSizeSpinBox->setValue(corrected);
    syncInProgress = false;

    updateTitles();
    updateInfoLabel();
}

void HarrisPage::onBlockSizeSpinChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    const int corrected = qBound(2, value, 15);

    syncInProgress = true;
    ui->blockSizeSpinBox->setValue(corrected);
    ui->blockSizeSlider->setValue(corrected);
    syncInProgress = false;

    updateTitles();
    updateInfoLabel();
}

void HarrisPage::onSobelKernelSliderChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    int corrected = qBound(3, value, 7);
    if (corrected % 2 == 0) {
        --corrected;
    }
    if (corrected < 3) {
        corrected = 3;
    }

    syncInProgress = true;
    ui->sobelKernelSlider->setValue(corrected);
    ui->sobelKernelSpinBox->setValue(corrected);
    syncInProgress = false;

    updateTitles();
    updateInfoLabel();
}

void HarrisPage::onSobelKernelSpinChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    int corrected = qBound(3, value, 7);
    if (corrected % 2 == 0) {
        --corrected;
    }
    if (corrected < 3) {
        corrected = 3;
    }

    syncInProgress = true;
    ui->sobelKernelSpinBox->setValue(corrected);
    ui->sobelKernelSlider->setValue(corrected);
    syncInProgress = false;

    updateTitles();
    updateInfoLabel();
}

void HarrisPage::onKSliderChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    const int corrected = qBound(1, value, 20);

    syncInProgress = true;
    ui->kSlider->setValue(corrected);
    ui->kDoubleSpinBox->setValue(kFromSlider(corrected));
    syncInProgress = false;

    updateTitles();
    updateInfoLabel();
}

void HarrisPage::onKSpinChanged(double value)
{
    if (syncInProgress) {
        return;
    }

    const double corrected = qBound(0.01, value, 0.20);

    syncInProgress = true;
    ui->kDoubleSpinBox->setValue(corrected);
    ui->kSlider->setValue(sliderFromK(corrected));
    syncInProgress = false;

    updateTitles();
    updateInfoLabel();
}

void HarrisPage::onThresholdSliderChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    const int corrected = qBound(0, value, 255);

    syncInProgress = true;
    ui->thresholdSlider->setValue(corrected);
    ui->thresholdSpinBox->setValue(corrected);
    syncInProgress = false;

    updateTitles();
    updateInfoLabel();
}

void HarrisPage::onThresholdSpinChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    const int corrected = qBound(0, value, 255);

    syncInProgress = true;
    ui->thresholdSpinBox->setValue(corrected);
    ui->thresholdSlider->setValue(corrected);
    syncInProgress = false;

    updateTitles();
    updateInfoLabel();
}

void HarrisPage::onApplyClicked()
{
    if (!imageLoaded) {
        return;
    }

    int blockSize = ui->blockSizeSpinBox->value();
    int sobelKernelSize = ui->sobelKernelSpinBox->value();
    const double k = ui->kDoubleSpinBox->value();
    const int threshold = ui->thresholdSpinBox->value();

    if (blockSize < 2) {
        blockSize = 2;
    }

    if (sobelKernelSize < 3) {
        sobelKernelSize = 3;
    }

    if (sobelKernelSize % 2 == 0) {
        --sobelKernelSize;
    }

    if (sobelKernelSize < 3) {
        sobelKernelSize = 3;
    }

    {
        const QSignalBlocker blocker1(ui->blockSizeSlider);
        const QSignalBlocker blocker2(ui->blockSizeSpinBox);
        ui->blockSizeSlider->setValue(blockSize);
        ui->blockSizeSpinBox->setValue(blockSize);
    }

    {
        const QSignalBlocker blocker1(ui->sobelKernelSlider);
        const QSignalBlocker blocker2(ui->sobelKernelSpinBox);
        ui->sobelKernelSlider->setValue(sobelKernelSize);
        ui->sobelKernelSpinBox->setValue(sobelKernelSize);
    }

    updateTitles();
    updateInfoLabel();

    emit applyRequested(blockSize, sobelKernelSize, k, threshold);
}

void HarrisPage::onResetClicked()
{
    const QSignalBlocker blocker1(ui->blockSizeSlider);
    const QSignalBlocker blocker2(ui->blockSizeSpinBox);
    const QSignalBlocker blocker3(ui->sobelKernelSlider);
    const QSignalBlocker blocker4(ui->sobelKernelSpinBox);
    const QSignalBlocker blocker5(ui->kSlider);
    const QSignalBlocker blocker6(ui->kDoubleSpinBox);
    const QSignalBlocker blocker7(ui->thresholdSlider);
    const QSignalBlocker blocker8(ui->thresholdSpinBox);

    ui->blockSizeSlider->setValue(3);
    ui->blockSizeSpinBox->setValue(3);

    ui->sobelKernelSlider->setValue(3);
    ui->sobelKernelSpinBox->setValue(3);

    ui->kSlider->setValue(sliderFromK(0.04));
    ui->kDoubleSpinBox->setValue(0.04);

    ui->thresholdSlider->setValue(125);
    ui->thresholdSpinBox->setValue(125);

    updateTitles();
    updateInfoLabel();
}

void HarrisPage::onPanelInfoClicked()
{
    QMessageBox::information(
        this,
        QStringLiteral("Info o panelu"),
        QStringLiteral(
            "Pliki obsługujące panel:\n"
            "- harrishandler.h - deklaracje operacji detekcji narożników Harrisa\n"
            "- harrishandler.cpp - logika detekcji narożników Harrisa\n\n"
            "Działania:\n"
            "- Konwersja do skali szarości - przygotowanie obrazu do analizy narożników - funkcja: toGrayscaleImage | plik: harrishandler.cpp\n"
            "- Obliczenie gradientów - wyznaczenie zmian jasności w kierunku X i Y - funkcje pomocnicze w harrishandler.cpp\n"
            "- Budowa macierzy Harrisa - wyznaczenie odpowiedzi narożnikowej na podstawie lokalnych gradientów - funkcja: applyHarris | plik: harrishandler.cpp\n"
            "- Progowanie odpowiedzi - wybór punktów o odpowiednio silnej odpowiedzi narożnikowej - funkcja: applyHarris | plik: harrishandler.cpp\n"
            "- Zaznaczenie narożników - naniesienie wykrytych punktów na obraz wynikowy - funkcja: applyHarris | plik: harrishandler.cpp\n"
            "- Ustawienie rozmiaru okna - regulacja lokalnego obszaru analizy - obsługa w panelu HarrisPage\n"
            "- Ustawienie maski Sobela - regulacja rozmiaru operatora gradientu - obsługa w panelu HarrisPage\n"
            "- Ustawienie współczynnika k - regulacja czułości funkcji Harrisa - obsługa w panelu HarrisPage\n"
            "- Ustawienie progu - regulacja liczby akceptowanych narożników - obsługa w panelu HarrisPage"
            )
        );
}

void HarrisPage::updateTitles()
{
    ui->groupBlockSize->setTitle(
        QStringLiteral("Rozmiar okna - %1").arg(ui->blockSizeSpinBox->value()));

    ui->groupSobelKernel->setTitle(
        QStringLiteral("Maska Sobela - %1").arg(ui->sobelKernelSpinBox->value()));

    ui->groupK->setTitle(
        QStringLiteral("Współczynnik k - %1")
            .arg(QString::number(ui->kDoubleSpinBox->value(), 'f', 2)));

    ui->groupThreshold->setTitle(
        QStringLiteral("Próg odpowiedzi - %1").arg(ui->thresholdSpinBox->value()));
}

void HarrisPage::updateInfoLabel()
{
    if (!imageLoaded) {
        ui->infoLabel->setText(
            QStringLiteral("Najpierw wczytaj obraz, aby wykonać detekcję narożników Harrisa."));
        return;
    }

    const int blockSize = ui->blockSizeSpinBox->value();
    const int sobelKernelSize = ui->sobelKernelSpinBox->value();
    const double k = ui->kDoubleSpinBox->value();
    const int threshold = ui->thresholdSpinBox->value();

    ui->infoLabel->setText(
        QStringLiteral(
            "Detekcja Harrisa: okno %1, maska Sobela %2, współczynnik k = %3, próg odpowiedzi %4. "
            "Wyższy próg zwykle zmniejsza liczbę wykrywanych narożników, a niższy zwiększa czułość.")
            .arg(blockSize)
            .arg(sobelKernelSize)
            .arg(QString::number(k, 'f', 2))
            .arg(threshold));
}