#ifndef EPOS3_H
#define EPOS3_H

#include <QMainWindow>

namespace Ui {
class epos3;
}

class epos3 : public QMainWindow
{
    Q_OBJECT

public:    
    explicit epos3(QWidget *parent = 0);
    ~epos3();


private slots:
    void on_pushButton_9_clicked();

    void on_pushButton_10_clicked();

    void on_pushButton_11_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_14_clicked();

    void on_pushButton_15_clicked();

    void on_horizontalSlider_sliderMoved(int position);

    void on_horizontalSlider_sliderPressed();

    void on_horizontalSlider_2_sliderMoved(int position);

    void on_horizontalSlider_sliderReleased();

    void on_horizontalSlider_2_sliderReleased();

    void on_pushButton_18_clicked();

    void on_pushButton_19_clicked();

    void on_pushButton_20_clicked();

    void on_pushButton_21_clicked();

    void on_pushButton_22_clicked();

    static void onmouse(int event, int x, int y, int flags, void* param);

    void on_pushButton_16_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_17_clicked();

    void on_pushButton_12_clicked();

private:
    Ui::epos3 *ui;
};

#endif // EPOS3_H
