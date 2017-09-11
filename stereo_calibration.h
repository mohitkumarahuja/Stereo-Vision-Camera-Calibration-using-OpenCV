
#ifndef STEREO_CALIBRATION_H
#define STEREO_CALIBRATION_H
#include <vector>
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv/cv.h"
#include "opencv/cxmisc.h"
#include "opencv2/calib3d.hpp"
#include <opencv2/videoio.hpp>
#include <opencv2/cudastereo.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudabgsegm.hpp>
#include <opencv2/cudacodec.hpp>
#include <opencv2/cudafeatures2d.hpp>
#include <opencv2/cudafilters.hpp>
//#include <opencv2/cudalegacy.hpp>
#include <opencv2/cudaobjdetect.hpp>
#include <opencv2/cudaoptflow.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>
#include "opencv2/imgcodecs.hpp"
#include <opencv2/core/opengl.hpp>
#include "opencv2/opencv_modules.hpp"
#include "opencv2/viz/viz3d.hpp"
#include "opencv2/viz/vizcore.hpp"
#include "opencv2/viz/widgets.hpp"
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



using namespace cv;
using namespace std;

class stereoCalibrateLib {
public:
    // ARRAY AND VECTOR STORAGE:
    double gM1[3][3], gM2[3][3], gD1[5], gD2[5];
    double gR[3][3], gT[3], gE[3][3], gF[3][3];
    double gQ[4][4];
    CvMat g_M1;
    CvMat g_M2;
    CvMat g_D1;
    CvMat g_D2;
    CvMat g_R;
    CvMat g_T;
    CvMat g_E;
    CvMat g_F;
    CvMat g_Q;

    cv::Mat Q;//(4,4,CV_32F);

    cv::Mat g_M1_mat;
    cv::Mat g_M2_mat;
    cv::Mat g_D1_mat;
    cv::Mat g_D2_mat;
    int g_pixelX;
    int g_pixelY;
    cv::Mat g_R_mat;
    cv::Mat g_T_mat;
    cv::Mat g_E_mat;
    cv::Mat g_F_mat;
    cv::Mat g_Q_mat;
    cv::Mat g_Mx1_mat;
    cv::Mat g_My1_mat;
    cv::Mat g_Mx2_mat;
    cv::Mat g_My2_mat;
    cv::Mat g_P1_mat;
    cv::Mat g_R1_mat;
    cv::Mat gMM1, gDD1, gRR, gTT;
    cv::Mat dispColor;
    cv::Mat g_R1_vector;
    cv::Mat g_RT_CE;
    cv::Mat g_RT_C;
    cv::Mat g_FXYZ,g_TXYZ,g_calFT,g_FTXYZ;

    int gblocksize, gnDisp;
    cv::Size gsize;
    int NumIters, Numlevels;
    int selectStereomatch=0;

    cv::Ptr<cv::cuda::StereoBM> sgbm;
    cv::Ptr<cv::cuda::StereoBeliefPropagation> bp;
    cv::Ptr<cv::cuda::StereoConstantSpaceBP> csbp;

    cv::Mat g_cloudModel;

public:
    stereoCalibrateLib()
    {
        g_M1 = cvMat(3, 3, CV_64F, gM1 );
        g_M2 = cvMat(3, 3, CV_64F, gM2 );
        g_D1 = cvMat(1, 5, CV_64F, gD1 );
        g_D2 = cvMat(1, 5, CV_64F, gD2 );
        g_R = cvMat(3, 3, CV_64F, gR );
        g_T = cvMat(3, 1, CV_64F, gT );
        g_E = cvMat(3, 3, CV_64F, gE );
        g_F = cvMat(3, 3, CV_64F, gF );
        g_Q = cvMat(4, 4, CV_32F,gQ);

        gblocksize = 11;
        gnDisp = 112;
        gsize.width = 658;
        gsize.height = 492;

        //sgbm = cv::cuda::createStereoBM(gnDisp);
        //bp = cv::cuda::createStereoBeliefPropagation(gnDisp);
        //csbp = cv::cuda::createStereoConstantSpaceBP(gnDisp);

    }
    cv::Mat  ComputeRect3dReprojectOnline(cv::Mat imgl,cv::Mat imgr, cv::Mat Q);
    bool loadCameraMatrix();
    cv::Mat computeMatrixFT(cv::Mat fxyz, cv::Mat txyz, cv::Mat calFT);
};


class stereo_calib
{
public:
    void StereoCalib(const vector<string>& imagelist, cv::Size boardSize,bool displayCorners, bool useCalibrated, bool showRectified);
    bool readStringList( const string& filename, vector<string>& l );
    int StereoCalibMain(const vector<string>& imagelist, cv::Size boardSize,bool displayCorners, bool useCalibrated, bool showRectified);

};


#endif // STEREO_CALIBRATION_H
