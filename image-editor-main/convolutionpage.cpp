#include "convolutionpage.h"
#include "ui_convolutionpage.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QtGlobal>

ConvolutionPage::ConvolutionPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ConvolutionPage)
{
    ui->setupUi(this);

    setupUiState();

    connect(ui->applyButton,
            &QPushButton::clicked,
            this,
            &ConvolutionPage::onApplyClicked);

    connect(ui->btnResetParams,
            &QPushButton::clicked,
            this,
            &ConvolutionPage::onResetClicked);

    connect(ui->filterComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ConvolutionPage::onFilterChanged);

    connect(ui->kernelSlider,
            &QSlider::valueChanged,
            this,
            &ConvolutionPage::onKernelSliderChanged);

    connect(ui->kernelSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &ConvolutionPage::onKernelSpinChanged);

    connect(ui->sigmaSlider,
            &QSlider::valueChanged,
            this,
            &ConvolutionPage::onSigmaSliderChanged);

    connect(ui->sigmaDoubleSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &ConvolutionPage::onSigmaSpinChanged);

    connect(ui->btnPanelInfo,
            &QPushButton::clicked,
            this,
            &ConvolutionPage::onPanelInfoClicked);

    setImageLoaded(false);
}

ConvolutionPage::~ConvolutionPage()
{
    delete ui;
}

void ConvolutionPage::setImageLoaded(bool loaded)
{
    imageLoaded = loaded;

    ui->applyButton->setEnabled(loaded);
    ui->filterComboBox->setEnabled(loaded);
    ui->kernelSlider->setEnabled(loaded);
    ui->kernelSpinBox->setEnabled(loaded);
    ui->sigmaSlider->setEnabled(loaded);
    ui->sigmaDoubleSpinBox->setEnabled(loaded);
    ui->normalizeCheckBox->setEnabled(loaded);
    ui->btnResetParams->setEnabled(loaded);

    if (!loaded) {
        ui->infoLabel->setText(QStringLiteral("Najpierw wczytaj obraz, aby zastosować filtr splotowy."));
    } else {
        updateControlsForFilter();
    }
}

void ConvolutionPage::setupUiState()
{
    const QSignalBlocker blockerFilter(ui->filterComboBox);

    ui->filterComboBox->clear();
    ui->filterComboBox->addItem(QStringLiteral("Rozmycie pudełkowe"));
    ui->filterComboBox->addItem(QStringLiteral("Rozmycie Gaussa"));
    ui->filterComboBox->addItem(QStringLiteral("Wyostrzanie"));
    ui->filterComboBox->addItem(QStringLiteral("Wykrywanie krawędzi"));
    ui->filterComboBox->addItem(QStringLiteral("Sobel"));

    ui->kernelSpinBox->setRange(3, 15);
    ui->kernelSpinBox->setSingleStep(2);
    ui->kernelSpinBox->setValue(3);

    ui->kernelSlider->setRange(0, 6);
    ui->kernelSlider->setSingleStep(1);
    ui->kernelSlider->setPageStep(1);
    ui->kernelSlider->setValue(sliderFromKernel(3));

    ui->sigmaDoubleSpinBox->setRange(0.1, 20.0);
    ui->sigmaDoubleSpinBox->setSingleStep(0.1);
    ui->sigmaDoubleSpinBox->setDecimals(2);
    ui->sigmaDoubleSpinBox->setValue(1.0);

    ui->sigmaSlider->setRange(1, 200);
    ui->sigmaSlider->setSingleStep(1);
    ui->sigmaSlider->setPageStep(5);
    ui->sigmaSlider->setValue(sliderFromSigma(1.0));

    ui->normalizeCheckBox->setChecked(true);

    updateGroupTitles();
    updateFilterInfo();
    updateControlsForFilter();
}

int ConvolutionPage::kernelFromSlider(int sliderValue) const
{
    const int clamped = qBound(0, sliderValue, 6);
    return 3 + clamped * 2;
}

int ConvolutionPage::sliderFromKernel(int kernelSize) const
{
    int normalized = kernelSize;

    if (normalized < 3) {
        normalized = 3;
    }
    if (normalized > 15) {
        normalized = 15;
    }
    if (normalized % 2 == 0) {
        --normalized;
    }

    return (normalized - 3) / 2;
}

double ConvolutionPage::sigmaFromSlider(int sliderValue) const
{
    const int clamped = qBound(1, sliderValue, 200);
    return static_cast<double>(clamped) / 10.0;
}

int ConvolutionPage::sliderFromSigma(double sigma) const
{
    const int value = qRound(sigma * 10.0);
    return qBound(1, value, 200);
}

void ConvolutionPage::onKernelSliderChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    syncInProgress = true;
    ui->kernelSpinBox->setValue(kernelFromSlider(value));
    syncInProgress = false;

    updateGroupTitles();
    updateFilterInfo();
}

void ConvolutionPage::onKernelSpinChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    int corrected = value;

    if (corrected < 3) {
        corrected = 3;
    }
    if (corrected > 15) {
        corrected = 15;
    }
    if (corrected % 2 == 0) {
        --corrected;
    }
    if (corrected < 3) {
        corrected = 3;
    }

    syncInProgress = true;
    ui->kernelSpinBox->setValue(corrected);
    ui->kernelSlider->setValue(sliderFromKernel(corrected));
    syncInProgress = false;

    updateGroupTitles();
    updateFilterInfo();
}

void ConvolutionPage::onSigmaSliderChanged(int value)
{
    if (syncInProgress) {
        return;
    }

    syncInProgress = true;
    ui->sigmaDoubleSpinBox->setValue(sigmaFromSlider(value));
    syncInProgress = false;

    updateGroupTitles();
    updateFilterInfo();
}

void ConvolutionPage::onSigmaSpinChanged(double value)
{
    if (syncInProgress) {
        return;
    }

    const double corrected = qBound(0.1, value, 20.0);

    syncInProgress = true;
    ui->sigmaDoubleSpinBox->setValue(corrected);
    ui->sigmaSlider->setValue(sliderFromSigma(corrected));
    syncInProgress = false;

    updateGroupTitles();
    updateFilterInfo();
}

void ConvolutionPage::onResetClicked()
{
    {
        const QSignalBlocker blocker1(ui->kernelSpinBox);
        const QSignalBlocker blocker2(ui->kernelSlider);
        ui->kernelSpinBox->setValue(3);
        ui->kernelSlider->setValue(sliderFromKernel(3));
    }

    {
        const QSignalBlocker blocker1(ui->sigmaDoubleSpinBox);
        const QSignalBlocker blocker2(ui->sigmaSlider);
        ui->sigmaDoubleSpinBox->setValue(1.0);
        ui->sigmaSlider->setValue(sliderFromSigma(1.0));
    }

    {
        const QSignalBlocker blocker(ui->normalizeCheckBox);
        ui->normalizeCheckBox->setChecked(true);
    }

    updateControlsForFilter();
    updateGroupTitles();
    updateFilterInfo();
}

void ConvolutionPage::onApplyClicked()
{
    if (!imageLoaded) {
        return;
    }

    QString filterName = ui->filterComboBox->currentText();
    int kernelSize = ui->kernelSpinBox->value();
    double sigma = ui->sigmaDoubleSpinBox->value();
    bool normalize = ui->normalizeCheckBox->isChecked();

    if (filterName == QStringLiteral("Sobel")) {
        kernelSize = 3;
        sigma = 1.0;
        normalize = false;
    } else if (filterName == QStringLiteral("Wykrywanie krawędzi")) {
        kernelSize = 3;
        sigma = 1.0;
        normalize = false;
    } else if (filterName == QStringLiteral("Wyostrzanie")) {
        sigma = 1.0;
        normalize = false;
    }

    if (kernelSize % 2 == 0) {
        --kernelSize;
    }

    emit applyRequested(filterName, kernelSize, sigma, normalize);
}

void ConvolutionPage::onFilterChanged(int)
{
    updateControlsForFilter();
}

void ConvolutionPage::updateControlsForFilter()
{
    const QString filter = ui->filterComboBox->currentText();

    const bool needsKernel =
        filter == QStringLiteral("Rozmycie pudełkowe") ||
        filter == QStringLiteral("Rozmycie Gaussa") ||
        filter == QStringLiteral("Wyostrzanie") ||
        filter == QStringLiteral("Wykrywanie krawędzi") ||
        filter == QStringLiteral("Sobel");

    const bool needsSigma =
        filter == QStringLiteral("Rozmycie Gaussa");

    const bool canNormalize =
        filter == QStringLiteral("Rozmycie pudełkowe") ||
        filter == QStringLiteral("Rozmycie Gaussa");

    const bool fixedKernel =
        filter == QStringLiteral("Sobel") ||
        filter == QStringLiteral("Wykrywanie krawędzi");

    ui->groupKernel->setEnabled(needsKernel);
    ui->kernelSlider->setEnabled(imageLoaded && needsKernel && !fixedKernel);
    ui->kernelSpinBox->setEnabled(imageLoaded && needsKernel && !fixedKernel);

    ui->groupSigma->setEnabled(needsSigma);
    ui->sigmaSlider->setEnabled(imageLoaded && needsSigma);
    ui->sigmaDoubleSpinBox->setEnabled(imageLoaded && needsSigma);

    ui->groupNormalize->setEnabled(canNormalize);
    ui->normalizeCheckBox->setEnabled(imageLoaded && canNormalize);

    ui->applyButton->setEnabled(imageLoaded && needsKernel);
    ui->btnResetParams->setEnabled(imageLoaded);

    if (!canNormalize) {
        const QSignalBlocker blocker(ui->normalizeCheckBox);
        ui->normalizeCheckBox->setChecked(false);
    }

    if (fixedKernel) {
        const QSignalBlocker blocker1(ui->kernelSpinBox);
        const QSignalBlocker blocker2(ui->kernelSlider);
        ui->kernelSpinBox->setValue(3);
        ui->kernelSlider->setValue(sliderFromKernel(3));
    }

    if (!needsSigma) {
        const QSignalBlocker blocker1(ui->sigmaDoubleSpinBox);
        const QSignalBlocker blocker2(ui->sigmaSlider);
        ui->sigmaDoubleSpinBox->setValue(1.0);
        ui->sigmaSlider->setValue(sliderFromSigma(1.0));
    }

    updateGroupTitles();
    updateFilterInfo();
}

void ConvolutionPage::updateGroupTitles()
{
    ui->groupKernel->setTitle(
        QStringLiteral("Rozmiar maski - %1").arg(ui->kernelSpinBox->value()));

    ui->groupSigma->setTitle(
        QStringLiteral("Sigma - %1")
            .arg(QString::number(ui->sigmaDoubleSpinBox->value(), 'f', 2)));
}

void ConvolutionPage::updateFilterInfo()
{
    if (!imageLoaded) {
        ui->infoLabel->setText(QStringLiteral("Najpierw wczytaj obraz, aby zastosować filtr splotowy."));
        return;
    }

    const QString filter = ui->filterComboBox->currentText();

    if (filter == QStringLiteral("Rozmycie pudełkowe")) {
        ui->infoLabel->setText(QStringLiteral(
            "Prosty filtr uśredniający. Większa maska daje mocniejsze rozmycie."));
    } else if (filter == QStringLiteral("Rozmycie Gaussa")) {
        ui->infoLabel->setText(QStringLiteral(
            "Rozmycie ważone rozkładem Gaussa. Parametr sigma steruje miękkością efektu."));
    } else if (filter == QStringLiteral("Wyostrzanie")) {
        ui->infoLabel->setText(QStringLiteral(
            "Wzmacnia lokalny kontrast i podkreśla krawędzie obrazu."));
    } else if (filter == QStringLiteral("Wykrywanie krawędzi")) {
        ui->infoLabel->setText(QStringLiteral(
            "Uwypukla szybkie zmiany jasności i granice obiektów."));
    } else if (filter == QStringLiteral("Sobel")) {
        ui->infoLabel->setText(QStringLiteral(
            "Oblicza gradient krawędzi przy użyciu operatora Sobela."));
    } else {
        ui->infoLabel->setText(QStringLiteral(
            "Wybierz filtr splotowy i ustaw jego parametry."));
    }
}

void ConvolutionPage::onPanelInfoClicked()
{
    QMessageBox::information(
        this,
        QStringLiteral("Info o panelu"),
        QStringLiteral(
            "Pliki obsługujące panel:\n"
            "- convolutionhandler.h - deklaracje operacji sąsiedztwa\n"
            "- convolutionhandler.cpp - logika operacji sąsiedztwa\n\n"
            "Działania:\n"
            "- Rozmycie pudełkowe - uśrednianie pikseli w masce - funkcja: applyConvolution | plik: convolutionhandler.cpp\n"
            "- Rozmycie Gaussa - rozmycie ważone rozkładem Gaussa - funkcja: applyConvolution | plik: convolutionhandler.cpp\n"
            "- Wyostrzanie - wzmacnianie lokalnego kontrastu i krawędzi - funkcja: applyConvolution | plik: convolutionhandler.cpp\n"
            "- Wykrywanie krawędzi - uwypuklanie granic obiektów - funkcja: applyConvolution | plik: convolutionhandler.cpp\n"
            "- Sobel - obliczanie gradientu obrazu operatorem Sobela - funkcja: applyConvolution | plik: convolutionhandler.cpp\n"
            "- Dobór rozmiaru maski - ustawienie wielkości okna splotu - obsługa w panelu: ConvolutionPage\n"
            "- Ustawienie sigma - regulacja parametru filtra Gaussa - obsługa w panelu: ConvolutionPage\n"
            "- Normalizacja - włączenie normalizacji dla filtrów rozmywających - obsługa w panelu: ConvolutionPage"
            ));
}