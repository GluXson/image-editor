#include "cannypage.h"
#include "ui_cannypage.h"

#include <utility>

#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QString>

CannyPage::CannyPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CannyPage)
{
    ui->setupUi(this);

    setupUiState();

    connect(ui->kernelSlider,
            &QSlider::valueChanged,
            this,
            &CannyPage::onAnyParameterChanged);

    connect(ui->sigmaSlider,
            &QSlider::valueChanged,
            this,
            &CannyPage::onAnyParameterChanged);

    connect(ui->lowThresholdSlider,
            &QSlider::valueChanged,
            this,
            &CannyPage::onAnyParameterChanged);

    connect(ui->highThresholdSlider,
            &QSlider::valueChanged,
            this,
            &CannyPage::onAnyParameterChanged);

    connect(ui->l2GradientCheckBox,
            &QCheckBox::checkStateChanged,
            this,
            &CannyPage::onAnyParameterChanged);

    connect(ui->applyButton,
            &QPushButton::clicked,
            this,
            &CannyPage::onApplyClicked);

    connect(ui->btnResetCanny,
            &QPushButton::clicked,
            this,
            [this]() {
                const QSignalBlocker blockerKernel(ui->kernelSlider);
                const QSignalBlocker blockerSigma(ui->sigmaSlider);
                const QSignalBlocker blockerLow(ui->lowThresholdSlider);
                const QSignalBlocker blockerHigh(ui->highThresholdSlider);
                const QSignalBlocker blockerL2(ui->l2GradientCheckBox);

                ui->kernelSlider->setValue(5);
                ui->sigmaSlider->setValue(140);
                ui->lowThresholdSlider->setValue(50);
                ui->highThresholdSlider->setValue(100);
                ui->l2GradientCheckBox->setChecked(false);

                updateTitles();
                updateInfoLabel();
            });

    connect(ui->btnPanelInfo,
            &QPushButton::clicked,
            this,
            &CannyPage::onPanelInfoClicked);

    setImageLoaded(false);
}

CannyPage::~CannyPage()
{
    delete ui;
}

void CannyPage::setImageLoaded(bool loaded)
{
    imageLoaded = loaded;

    ui->kernelSlider->setEnabled(loaded);
    ui->sigmaSlider->setEnabled(loaded);
    ui->lowThresholdSlider->setEnabled(loaded);
    ui->highThresholdSlider->setEnabled(loaded);
    ui->l2GradientCheckBox->setEnabled(loaded);
    ui->applyButton->setEnabled(loaded);
    ui->btnResetCanny->setEnabled(loaded);

    updateInfoLabel();
}

void CannyPage::setupUiState()
{
    ui->kernelSlider->setRange(3, 15);
    ui->kernelSlider->setSingleStep(2);
    ui->kernelSlider->setPageStep(2);
    ui->kernelSlider->setValue(5);

    ui->sigmaSlider->setRange(10, 2000);
    ui->sigmaSlider->setSingleStep(1);
    ui->sigmaSlider->setPageStep(10);
    ui->sigmaSlider->setValue(140);

    ui->lowThresholdSlider->setRange(0, 255);
    ui->lowThresholdSlider->setValue(50);

    ui->highThresholdSlider->setRange(0, 255);
    ui->highThresholdSlider->setValue(100);

    ui->l2GradientCheckBox->setChecked(false);

    updateTitles();
    updateInfoLabel();
}

void CannyPage::onApplyClicked()
{
    if (!imageLoaded) {
        return;
    }

    int kernelSize = ui->kernelSlider->value();
    double sigma = ui->sigmaSlider->value() / 100.0;
    int lowThreshold = ui->lowThresholdSlider->value();
    int highThreshold = ui->highThresholdSlider->value();
    const bool useL2Gradient = ui->l2GradientCheckBox->isChecked();

    if (kernelSize % 2 == 0) {
        ++kernelSize;
        const QSignalBlocker blocker(ui->kernelSlider);
        ui->kernelSlider->setValue(kernelSize);
    }

    if (lowThreshold > highThreshold) {
        std::swap(lowThreshold, highThreshold);

        const QSignalBlocker blockerLow(ui->lowThresholdSlider);
        const QSignalBlocker blockerHigh(ui->highThresholdSlider);
        ui->lowThresholdSlider->setValue(lowThreshold);
        ui->highThresholdSlider->setValue(highThreshold);
    }

    updateTitles();
    updateInfoLabel();

    emit applyRequested(kernelSize,
                        sigma,
                        lowThreshold,
                        highThreshold,
                        useL2Gradient);
}

void CannyPage::onAnyParameterChanged()
{
    if (syncInProgress) {
        return;
    }

    syncInProgress = true;

    if (ui->kernelSlider->value() % 2 == 0) {
        ui->kernelSlider->setValue(ui->kernelSlider->value() + 1);
    }

    syncInProgress = false;

    updateTitles();
    updateInfoLabel();
}

void CannyPage::onPanelInfoClicked()
{
    QMessageBox::information(
        this,
        "Info o panelu",
        "Pliki obsługujące panel:\n"
        "- cannyhandler.h - deklaracje operacji detekcji krawędzi Canny'ego\n"
        "- cannyhandler.cpp - logika detekcji krawędzi Canny'ego\n\n"
        "Działania:\n"
        "- Konwersja do skali szarości - przekształcenie obrazu wejściowego do odcieni szarości - funkcja: toGrayscaleImage | plik: cannyhandler.cpp\n"
        "- Budowa maski Gaussa - utworzenie jądra wygładzającego o zadanym rozmiarze i sigma - funkcja: buildGaussianKernel | plik: cannyhandler.cpp\n"
        "- Wygładzanie obrazu - filtracja obrazu w celu redukcji szumu - funkcja: convolveGray | plik: cannyhandler.cpp\n"
        "- Gradient Sobela X - obliczenie składowej poziomej gradientu - funkcja: buildSobelKernelX | plik: cannyhandler.cpp\n"
        "- Gradient Sobela Y - obliczenie składowej pionowej gradientu - funkcja: buildSobelKernelY | plik: cannyhandler.cpp\n"
        "- Moduł gradientu - wyznaczenie siły krawędzi metodą L1 lub L2 - funkcja: computeGradientMagnitude | plik: cannyhandler.cpp\n"
        "- Kierunek gradientu - wyznaczenie kierunku zmian jasności - funkcja: computeGradientDirection | plik: cannyhandler.cpp\n"
        "- Non-maximum suppression - pocienianie krawędzi przez tłumienie odpowiedzi niemaksymalnych - funkcja: nonMaximumSuppression | plik: cannyhandler.cpp\n"
        "- Progowanie z histerezą - wybór silnych i słabych krawędzi na podstawie dwóch progów - funkcja: applyHysteresis | plik: cannyhandler.cpp\n"
        "- Detekcja Canny'ego - pełne wykonanie algorytmu i zapis wyniku do obrazu - funkcja: applyCanny | plik: cannyhandler.cpp"
        );
}

void CannyPage::updateInfoLabel()
{
    if (!imageLoaded) {
        ui->infoLabel->setText("Najpierw wczytaj obraz, aby wykonać detekcję krawędzi Canny'ego.");
        return;
    }

    const int kernelSize = ui->kernelSlider->value();
    const double sigma = ui->sigmaSlider->value() / 100.0;
    const int lowThreshold = ui->lowThresholdSlider->value();
    const int highThreshold = ui->highThresholdSlider->value();
    const bool useL2Gradient = ui->l2GradientCheckBox->isChecked();

    QString text =
        QString("Detekcja Canny'ego: maska %1x%1, sigma %2, próg dolny %3, próg górny %4, gradient %5.")
            .arg(kernelSize)
            .arg(sigma, 0, 'f', 2)
            .arg(lowThreshold)
            .arg(highThreshold)
            .arg(useL2Gradient ? "L2" : "L1");

    if (lowThreshold > highThreshold) {
        text += " Uwaga: próg dolny jest większy od górnego — wartości zostaną zamienione przy zastosowaniu.";
    } else if (lowThreshold == highThreshold) {
        text += " Uwaga: identyczne progi histerezy zwykle dają słabszą separację krawędzi.";
    } else {
        text += " Niższy próg zwiększa czułość, a wyższy ogranicza słabe odpowiedzi.";
    }

    ui->infoLabel->setText(text);
}

void CannyPage::updateTitles()
{
    ui->groupKernel->setTitle(QString("Rozmiar maski - %1").arg(ui->kernelSlider->value()));
    ui->groupSigma->setTitle(QString("Sigma - %1").arg(QString::number(ui->sigmaSlider->value() / 100.0, 'f', 2)));
    ui->groupLowThreshold->setTitle(QString("Próg dolny - %1").arg(ui->lowThresholdSlider->value()));
    ui->groupHighThreshold->setTitle(QString("Próg górny - %1").arg(ui->highThresholdSlider->value()));
}