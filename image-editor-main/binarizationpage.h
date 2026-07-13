#pragma once

#include <QWidget>
#include <array>

QT_BEGIN_NAMESPACE
namespace Ui {
class BinarizationPage;
}
QT_END_NAMESPACE

class QChart;
class QChartView;

class BinarizationPage : public QWidget
{
    Q_OBJECT

public:
    explicit BinarizationPage(QWidget *parent = nullptr);
    ~BinarizationPage();

    void setImageLoaded(bool loaded);
    void updateHistogram(const std::array<int, 256>& histGray);
    void setThreshold(int value);
    int threshold() const;
    void setOtsuPreview(int value);

signals:
    void manualThresholdRequested(int threshold);
    void otsuRequested();
    void backRequested();

private slots:
    void onThresholdSliderChanged(int value);
    void onThresholdSpinChanged(int value);
    void onApplyManualClicked();
    void onApplyOtsuClicked();
    void onResetThresholdClicked();
    void onBackClicked();

private:
    Ui::BinarizationPage *ui;
    QChart *chart;
    QChartView *chartView;
    bool imageLoaded = false;
    bool syncInProgress = false;
    int otsuPreviewValue = -1;

    void updateLabels();
};