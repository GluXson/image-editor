#include "pointoperationpage.h"
#include "ui_pointoperationpage.h"

#include <QMessageBox>
#include <QPushButton>
#include <QSlider>

PointOperationPage::PointOperationPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PointOperationPage)
{
    ui->setupUi(this);

    connect(ui->sliderBrightness, &QSlider::valueChanged, this, [this](int value) {
        ui->groupBrightness->setTitle(QString("Jasność - %1").arg(value));
    });

    connect(ui->sliderSaturation, &QSlider::valueChanged, this, [this](int value) {
        ui->groupSaturation->setTitle(QString("Nasycenie - %1").arg(value));
    });

    connect(ui->sliderContrastParam, &QSlider::valueChanged, this, [this](int value) {
        ui->groupContrast->setTitle(
            QString("Kontrast - %1").arg(QString::number(value / 100.0, 'f', 2))
            );
    });

    connect(ui->sliderMonoA, &QSlider::valueChanged, this, [this](int value) {
        ui->labelMonoATitle->setText(
            QString("Parametr a - %1").arg(QString::number(value / 100.0, 'f', 2))
            );
    });

    connect(ui->sliderMonoB, &QSlider::valueChanged, this, [this](int value) {
        ui->labelMonoBTitle->setText(QString("Parametr b - %1").arg(value));
    });

    connect(ui->btnResetBrightness, &QPushButton::clicked, this, [this]() {
        ui->sliderBrightness->setValue(0);
    });

    connect(ui->btnResetSaturation, &QPushButton::clicked, this, [this]() {
        ui->sliderSaturation->setValue(0);
    });

    connect(ui->btnResetContrast, &QPushButton::clicked, this, [this]() {
        ui->sliderContrastParam->setValue(100);
    });

    connect(ui->btnResetMono, &QPushButton::clicked, this, [this]() {
        ui->sliderMonoA->setValue(50);
        ui->sliderMonoB->setValue(0);
    });

    connect(ui->btnPanelInfo, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(
            this,
            "Info o panelu",
            "Pliki obsługujące panel:\n"
            "- pointoperationhandler.h - deklaracje operacji punktowych\n"
            "- pointoperationhandler.cpp - logika operacji punktowych\n\n"
            "Działania:\n"
            "- Desaturacja - skala szarości - funkcja: applyDesaturation | plik: pointoperationhandler.cpp\n"
            "- Negatyw - odwrócenie kolorów - funkcja: applyNegative | plik: pointoperationhandler.cpp\n"
            "- Jasność - zmiana jasności obrazu - funkcja: applyBrightness | plik: pointoperationhandler.cpp\n"
            "- Nasycenie - zmiana intensywności kolorów - funkcja: applySaturation | plik: pointoperationhandler.cpp\n"
            "- Kontrast liniowy - liniowa zmiana kontrastu - funkcja: applyLinearContrast | plik: pointoperationhandler.cpp\n"
            "- Kontrast logarytmiczny - logarytmiczne przekształcenie kontrastu - funkcja: applyLogarithmicContrast | plik: pointoperationhandler.cpp\n"
            "- Kontrast potęgowy - potęgowe przekształcenie kontrastu - funkcja: applyPowerContrast | plik: pointoperationhandler.cpp\n"
            "- Transformacja monochromatyczna - przekształcenie z parametrami a i b - funkcja: applyMonochromeTransform | plik: pointoperationhandler.cpp\n"
            "- Import drugiego obrazu - wybór pliku pomocniczego - funkcja: importSecondImage | plik: pointoperationhandler.cpp\n"
            "- Suma - dodawanie dwóch obrazów - funkcja: applyImageSum | plik: pointoperationhandler.cpp\n"
            "- Różnica - odejmowanie obrazów - funkcja: applyImageDifference | plik: pointoperationhandler.cpp\n"
            "- Iloczyn - mnożenie obrazów - funkcja: applyImageMultiply | plik: pointoperationhandler.cpp"
            );
    });

    ui->groupBrightness->setTitle(QString("Jasność - %1").arg(ui->sliderBrightness->value()));
    ui->groupSaturation->setTitle(QString("Nasycenie - %1").arg(ui->sliderSaturation->value()));
    ui->groupContrast->setTitle(
        QString("Kontrast - %1").arg(QString::number(ui->sliderContrastParam->value() / 100.0, 'f', 2))
        );
    ui->labelMonoATitle->setText(
        QString("Parametr a - %1").arg(QString::number(ui->sliderMonoA->value() / 100.0, 'f', 2))
        );
    ui->labelMonoBTitle->setText(QString("Parametr b - %1").arg(ui->sliderMonoB->value()));
}

PointOperationPage::~PointOperationPage()
{
    delete ui;
}

void PointOperationPage::setSecondImageName(const QString &fileName)
{
    if (fileName.isEmpty()) {
        ui->labelSecondImageName->setText("Nie wybrano pliku");
    } else {
        ui->labelSecondImageName->setText(fileName);
    }
}

void PointOperationPage::on_btnImportSecondImage_clicked()
{
    emit importSecondImageRequested();
}

void PointOperationPage::on_btnDesaturate_clicked()
{
    emit desaturateRequested();
}

void PointOperationPage::on_btnNegative_clicked()
{
    emit negativeRequested();
}

void PointOperationPage::on_btnApplyBrightness_clicked()
{
    emit brightnessRequested(ui->sliderBrightness->value());
}

void PointOperationPage::on_btnApplySaturation_clicked()
{
    emit saturationRequested(ui->sliderSaturation->value());
}

void PointOperationPage::on_btnApplyContrast_clicked()
{
    const QString mode = ui->comboContrastMode->currentText();
    const double value = ui->sliderContrastParam->value() / 100.0;

    if (mode == "Liniowy") {
        emit linearContrastRequested(value);
    } else if (mode == "Logarytmiczny") {
        emit logarithmicContrastRequested(value);
    } else if (mode == "Potęgowy") {
        emit powerContrastRequested(value);
    }
}

void PointOperationPage::on_btnApplyMono_clicked()
{
    emit monoTransformRequested(
        ui->sliderMonoA->value() / 100.0,
        static_cast<double>(ui->sliderMonoB->value())
        );
}

void PointOperationPage::on_btnSum_clicked()
{
    emit sumImagesRequested();
}

void PointOperationPage::on_btnDiff_clicked()
{
    emit diffImagesRequested();
}

void PointOperationPage::on_btnMultiply_clicked()
{
    emit multiplyImagesRequested();
}