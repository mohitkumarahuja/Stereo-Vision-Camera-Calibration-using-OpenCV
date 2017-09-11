#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <vector>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <math.h>
#include <QVector>
#include <QString>
#include <QTime>
#include <opencv2/cudastereo.hpp>
#include <QDir>
#include <QFileDialog>
#include <QElapsedTimer>
#include <opencv2/core/core.hpp>
#include <opencv2/viz.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include "ml.hpp"
#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <thread>
#include <QMessageBox>
#include <exception>
#include <semaphore.h>
#include "opencv2/calib3d.hpp"
#include "stereo_calibration.h"

using namespace cv;
using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:


    //videoDisplay vDisplay;
    sem_t g_mtx;
    stereoCalibrateLib stcl;
    stereo_calib stereocalib;


    cv::Mat imgl;
    cv::Mat imgr;



    int g_imgCount = 0;
    int g_markerCount = 0;
    int g_usleep = 1000;
    int g_usleepForce = 1000;
    int g_ForceMarkerSelect;
    int g_timeInSeconds;
    int g_counterForceSens;
    int g_xaxisMarker;
    int g_noiseFilter = 4;
    int g_samplesSize;

    float g_forceSensibility;
    float g_maxFx = 0.0f;
    float g_minFx = 0.0f;
    float g_maxFy = 0.0f;
    float g_minFy = 0.0f;
    float g_maxFz = 0.0f;
    float g_minFz = 0.0f;
    float gTx, gTy, gTz, gZborder;

   // bool g_saveValues = true;
   // bool g_captureVideoON = false;
   // bool g_esayTrackerON = false;
   // bool g_trigerUCT = false;
  //  bool g_updateEtkLedValues = false;
   // bool g_forceExceeded = false;
   // bool g_acquisitionRunning=false;
   // bool g_startSensForce = false;
   // bool g_gravityComp = false;

    cv::Scalar color;
    std::thread g_Tcv,g_etk, g_uct, g_fT, g_opglT;



    QElapsedTimer g_sampleTime;

    std::list<cv::Mat> g_Tip_ForceSensPos;
    std::vector<cv::Point3f> g_ftxyz;
    vector<cv::Mat> g_forceSensorvaluesBIN_vector;
    std::vector<cv::Mat> g_ForceSensPos;
    std::vector<cv::Mat> g_FTXYZ;
    cv::Mat g_FTXYZ_MAT,g_ForceSensPos_MAT;
    cv::Mat g_RotMatrix, g_RotMatrixForce2Etk = cv::Mat(3,3,CV_32FC1);
    cv::Mat g_F_off = cv::Mat(3,1,CV_32FC1);
    cv::Mat g_Mg = cv::Mat(3,1,CV_32FC1);
    cv::Point3f g_tipForceSensor, g_tipForceSensorDefault, g_cloudModelTempPoint;
    cv::Point3f g_initialForcePos,g_currentForcePos,g_deltaPos;
    cv::Point2f g_pixelTipPosition;


    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void *calibrateThread(void);
    cv::Mat image;
    cv::Mat image_2;
    std::vector<std::string> filelist;   //to store filenames of chessboard pattern for left
    //std::vector<std::string> filelist_2; //to store filenames of chessboard pattern for right

private slots:

    void on_LoadImages_clicked();

    void on_Test_clicked();

    void on_Camera_Parametrs_clicked();

    void on__3D_Map_clicked();

    void on_cameraParameters_clicked();

private:
    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
