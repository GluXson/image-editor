#pragma once

#include <QWidget>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class ConvolutionPage;
}
QT_END_NAMESPACE

class ConvolutionPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConvolutionPage(QWidget *parent = nullptr);
    ~ConvolutionPage();

    void setImageLoaded(bool loaded);

signals:
    void applyRequested(const QString &filterName, int kernelSize, double sigma, bool normalize);
    void backRequested();

private slots:
    void onApplyClicked();
    void onFilterChanged(int index);
    void onKernelSliderChanged(int value);
    void onKernelSpinChanged(int value);
    void onSigmaSliderChanged(int value);
    void onSigmaSpinChanged(double value);
    void onResetClicked();
    void onPanelInfoClicked();

private:
    Ui::ConvolutionPage *ui;
    bool imageLoaded = false;
    bool syncInProgress = false;

    void setupUiState();
    void updateControlsForFilter();
    void updateGroupTitles();
    void updateFilterInfo();

    int kernelFromSlider(int sliderValue) const;
    int sliderFromKernel(int kernelSize) const;
    double sigmaFromSlider(int sliderValue) const;
    int sliderFromSigma(double sigma) const;
};