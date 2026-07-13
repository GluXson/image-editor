#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class HarrisPage;
}
QT_END_NAMESPACE

class HarrisPage : public QWidget
{
    Q_OBJECT

public:
    explicit HarrisPage(QWidget *parent = nullptr);
    ~HarrisPage() override;

    void setImageLoaded(bool loaded);

signals:
    void applyRequested(int blockSize, int sobelKernelSize, double k, int threshold);

private slots:
    void onBlockSizeSliderChanged(int value);
    void onBlockSizeSpinChanged(int value);

    void onSobelKernelSliderChanged(int value);
    void onSobelKernelSpinChanged(int value);

    void onKSliderChanged(int value);
    void onKSpinChanged(double value);

    void onThresholdSliderChanged(int value);
    void onThresholdSpinChanged(int value);

    void onApplyClicked();
    void onResetClicked();
    void onPanelInfoClicked();

private:
    Ui::HarrisPage *ui = nullptr;

    bool imageLoaded = false;
    bool syncInProgress = false;

    void setupUiState();
    void updateTitles();
    void updateInfoLabel();

    double kFromSlider(int value) const;
    int sliderFromK(double value) const;
};