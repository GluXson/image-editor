#pragma once

#include <QWidget>
#include <array>

QT_BEGIN_NAMESPACE
namespace Ui {
class HistogramPage;
}
QT_END_NAMESPACE

class QChart;
class QChartView;

class HistogramPage : public QWidget
{
    Q_OBJECT

public:
    explicit HistogramPage(QWidget *parent = nullptr);
    ~HistogramPage();

    void updateHistogram(const std::array<int, 256> &histR,
                         const std::array<int, 256> &histG,
                         const std::array<int, 256> &histB,
                         const std::array<int, 256> &histGray);

signals:
    void histogramDisplayChanged();
    void stretchHistogramRequested();
    void equalizeHistogramRequested();

private slots:
    void on_checkR_toggled(bool checked);
    void on_checkG_toggled(bool checked);
    void on_checkB_toggled(bool checked);
    void on_checkGray_toggled(bool checked);
    void on_btnStretchHistogram_clicked();
    void on_btnEqualizeHistogram_clicked();
    void on_btnHistogramInfo_clicked();

private:
    Ui::HistogramPage *ui = nullptr;
    QChart *chart = nullptr;
    QChartView *chartView = nullptr;
};