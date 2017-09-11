#include "stereo_calibration.h"


cv::Mat  stereoCalibrateLib::ComputeRect3dReprojectOnline(cv::Mat imgGpu1,cv::Mat imgGpu2, cv::Mat Q)
{
     cv::Mat vdisp( imgGpu1.rows,imgGpu1.cols, CV_8U );
     cv::Mat result3DImageMat(imgGpu1.rows,imgGpu1.cols, CV_32FC3);
     cv::Mat dispGpu(imgGpu1.rows,imgGpu1.cols,CV_8U);
     cv::cuda::GpuMat dispGpuColor;
     cv::cuda::GpuMat result3DImageGPUMat(imgGpu1.rows,imgGpu1.cols, CV_32FC3);

     sgbm->setMinDisparity(0);
     sgbm->setNumDisparities(gnDisp);
     sgbm->setBlockSize(gblocksize);
     sgbm->setUniquenessRatio(10);
     sgbm->setSpeckleWindowSize(100);
     sgbm->setSpeckleRange(32);
     sgbm->setDisp12MaxDiff(1);
     bp->setNumDisparities(gnDisp);
     csbp->setNumDisparities(gnDisp);


     cv::cvtColor(imgGpu1,imgGpu1,CV_BGR2GRAY);
     cv::cvtColor(imgGpu2,imgGpu2,CV_BGR2GRAY);
     cv::resize(imgGpu1,imgGpu1,gsize);
     cv::resize(imgGpu2,imgGpu2,gsize);

    if(selectStereomatch==0)
     sgbm->compute(imgGpu1, imgGpu2, dispGpu);
    if(selectStereomatch==1)
     bp->compute(imgGpu1, imgGpu2, dispGpu);
    if(selectStereomatch==2)
     csbp->compute(imgGpu1, imgGpu2, dispGpu);

     //cv::drawColorDisp(dispGpu,dispGpuColor,gnDisp);
     dispGpuColor.download(dispColor);
     cv::imshow("disparity", dispColor );
     cv::reprojectImageTo3D(dispGpu,result3DImageGPUMat,Q,3);

     result3DImageGPUMat.download(result3DImageMat);
     cv::imshow("3D Map", result3DImageMat );
    return result3DImageMat;
}

bool stereoCalibrateLib::loadCameraMatrix()
{

    Q= cv::Mat::zeros(4,4,CV_32F);

    bool loadfile1 = false;
    bool loadfile2 = false;

    bool retVal = true;

    cout<<"Loading Camera Matrix..."<<endl;

    //LOADING CAMERA MATRIX CALIBRATION


    cv::FileStorage fs1("C://Users/ROBMED/Desktop/CameraCalibration/extrinsics.yml", cv::FileStorage::READ);
    if(fs1.isOpened()){
        fs1["R"]>>g_R_mat;
        fs1["T"]>>g_T_mat;
        fs1["P1"]>>g_P1_mat;
        fs1["Q"] >> Q;
        Q.convertTo(g_Q_mat,CV_32F);
        fs1.release();
        loadfile1 = true;
         cout<<"C://Users/ROBMED/Desktop/CameraCalibration/extrinsics.yml opened..."<<endl;
         cout<<"Q = "<<Q<<endl;
    }
     else
    {
        loadfile1 = false;
        cout<<"C://Users/ROBMED/Desktop/CameraCalibration/extrinsics.yml could not be opened..."<<endl;
    }

    cv::FileStorage fs2("C://Users/ROBMED/Desktop/CameraCalibration/intrinsics.yml", cv::FileStorage::READ);
    if(fs2.isOpened()){
        fs2["D1"]>>g_D1_mat;
        fs2["M1"]>>g_M1_mat;
        fs2.release();
        loadfile2 = true;
         cout<<"C://Users/ROBMED/Desktop/CameraCalibration/intrinsics.yml opened..."<<endl;
    }
    else
    {
        loadfile2 = false;
        cout<<"C://Users/ROBMED/Desktop/CameraCalibration/intrinsics.yml could not be opened..."<<endl;
    }

    if( loadfile1 & loadfile2 ){
        g_calFT = computeMatrixFT(g_FXYZ,g_TXYZ,g_calFT);
        g_calFT.convertTo(g_calFT,CV_32FC1);
        cout<<"Parameters loaded..."<<endl;
        retVal = true;
    }
    else
    {
        retVal = false;
        cout<<"Parameters not loaded..."<<endl;

    }
    return retVal;
}

cv::Mat stereoCalibrateLib::computeMatrixFT(cv::Mat fxyz, cv::Mat txyz, cv::Mat calFT)
{
    cv::Mat matrix(6,6,CV_32FC1);

    for(int i = 0;i<calFT.cols;i++)
    {
        matrix.at<float>(0,i)=calFT.at<float>(0,i)/fxyz.at<float>(0,0);
        matrix.at<float>(1,i)=calFT.at<float>(1,i)/fxyz.at<float>(1,0);
        matrix.at<float>(2,i)=calFT.at<float>(2,i)/fxyz.at<float>(2,0);
    }
    for(int j = 0;j<calFT.cols;j++)
    {
        matrix.at<float>(3,j)=calFT.at<float>(3,j)/txyz.at<float>(0,0);
        matrix.at<float>(4,j)=calFT.at<float>(4,j)/txyz.at<float>(1,0);
        matrix.at<float>(5,j)=calFT.at<float>(5,j)/txyz.at<float>(2,0);
    }

    return matrix;
}

void stereo_calib::StereoCalib(const vector<string>& imagelist, cv::Size boardSize,bool displayCorners = false, bool useCalibrated=true, bool showRectified=true)
{
    cout<<"Starting Stereo Calibration... "<<endl;

    if( imagelist.size() % 2 != 0 )
    {
        cout << "Error: the image list contains odd (non-even) number of elements\n";
        return;
    }

    const int maxScale = 2;
    const float squareSize = 0.0025f;  // Set this to your actual square size
    // ARRAY AND VECTOR STORAGE:

    vector<vector<cv::Point2f> > imagePoints[2];
    vector<vector<cv::Point3f> > objectPoints;
    cv::Size imageSize;

    int i, j, k, nimages = (int)imagelist.size()/2;

    imagePoints[0].resize(nimages);
    imagePoints[1].resize(nimages);
    vector<string> goodImageList;

    cout<<"before For"<<endl;

    for( i = j = 0; i < nimages; i++ )
    {

        cout<<"first For"<<endl;
        for( k = 0; k < 2; k++ )
        {
            const string& filename = imagelist[i*2+k];
            cv::Mat img = cv::imread(filename, 0);

            if(img.empty())
                break;
            if( imageSize == cv::Size() )
                imageSize = img.size();
            else if( img.size() != imageSize )
            {
                cout << "The image " << filename << " has the size different from the first image size. Skipping the pair\n";
                break;
            }
            bool found = false;
            vector<cv::Point2f>& corners = imagePoints[k][j];
            for( int scale = 1; scale <= maxScale; scale++ )
            {
                cv::Mat timg;
                if( scale == 1 )
                    timg = img;
                else
                    cv::resize(img, timg, cv::Size(), scale, scale);
                found = cv::findChessboardCorners(timg, boardSize, corners,
                    cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
                if( found )
                {
                    if( scale > 1 )
                    {
                        cv::Mat cornersMat(corners);
                        cornersMat *= 1./scale;
                    }
                    break;
                }
            }
            if( displayCorners )
            {
                cout << filename << endl;
                cv::Mat cimg, cimg1;
                cv::cvtColor(img, cimg, cv::COLOR_GRAY2BGR);
                cv::drawChessboardCorners(cimg, boardSize, corners, found);
                double sf = 640./MAX(img.rows, img.cols);
                cv::resize(cimg, cimg1, cv::Size(), sf, sf);
                cv::imshow("corners", cimg1);
                //imshow("corners", cimg);
                char c = (char)cv::waitKey(500);
                if( c == 27 || c == 'q' || c == 'Q' ) //Allow ESC to quit
                    exit(-1);
            }
            else
                putchar('.');
            if( !found )
                break;
            cv::cornerSubPix(img, corners, cv::Size(11,11), cv::Size(-1,-1),
                         cv::TermCriteria(cv::TermCriteria::COUNT+cv::TermCriteria::EPS,
                                      30, 0.01));
        }
        if( k == 2 )
        {
            goodImageList.push_back(imagelist[i*2]);
            goodImageList.push_back(imagelist[i*2+1]);
            j++;
        }
    }
    cout << j << " pairs have been successfully detected.\n";
    nimages = j;
    if( nimages < 2 )
    {
        cout << "Error: too little pairs to run the calibration\n";
        return;
    }

    imagePoints[0].resize(nimages);
    imagePoints[1].resize(nimages);
    objectPoints.resize(nimages);

    for( i = 0; i < nimages; i++ )
    {
        for( j = 0; j < boardSize.height; j++ )
            for( k = 0; k < boardSize.width; k++ )
                objectPoints[i].push_back(cv::Point3f(k*squareSize, j*squareSize, 0));
    }

    cout << "Running stereo calibration ...\n";


    cv::Mat cameraMatrix[2], distCoeffs[2];

    //*****************************************************************************
//    cv::FileStorage fs("images/intrinsics.yml", cv::FileStorage::WRITE);
//    if( fs.isOpened() )
//    {
//        fs["M1"]>>cameraMatrix[0];
//        fs["M2"]>>cameraMatrix[1];
//        fs["D1"]>>distCoeffs[0];
//        fs["D2"]>>distCoeffs[1];

//           fs.release();
//    }

    //*****************************************************************************

    cameraMatrix[0] = initCameraMatrix2D(objectPoints,imagePoints[0],imageSize,0);
    cameraMatrix[1] = initCameraMatrix2D(objectPoints,imagePoints[1],imageSize,0);
    cv::Mat R, T, E, F;

    double rms = cv::stereoCalibrate(objectPoints, imagePoints[0], imagePoints[1],
                    cameraMatrix[0], distCoeffs[0],
                    cameraMatrix[1], distCoeffs[1],
                    imageSize, R, T, E, F,
//                    cv::CALIB_FIX_INTRINSIC+//+//using intrinsics param, computing only extrinsics
                    cv::CALIB_USE_INTRINSIC_GUESS+
                    cv::CALIB_FIX_PRINCIPAL_POINT+
                    cv::CALIB_FIX_ASPECT_RATIO+
                    cv::CALIB_SAME_FOCAL_LENGTH+
                    cv::CALIB_ZERO_TANGENT_DIST+
                    cv::CALIB_RATIONAL_MODEL+
                    cv::CALIB_FIX_K3 +
                    cv::CALIB_FIX_K4 +
                    cv::CALIB_FIX_K5,
                    cv::TermCriteria(cv::TermCriteria::COUNT+cv::TermCriteria::EPS, 100, 1e-5) );
    cout << "done with RMS error=" << rms << endl;

// CALIBRATION QUALITY CHECK
// because the output fundamental matrix implicitly
// includes all the output information,
// we can check the quality of calibration using the
// epipolar geometry constraint: m2^t*F*m1=0
    double err = 0;
    int npoints = 0;
    vector<cv::Vec3f> lines[2];
    for( i = 0; i < nimages; i++ )
    {
        int npt = (int)imagePoints[0][i].size();
        cv::Mat imgpt[2];
        for( k = 0; k < 2; k++ )
        {
            imgpt[k] = cv::Mat(imagePoints[k][i]);
            cv::undistortPoints(imgpt[k], imgpt[k], cameraMatrix[k], distCoeffs[k], cv::Mat(), cameraMatrix[k]);
            computeCorrespondEpilines(imgpt[k], k+1, F, lines[k]);
        }
        for( j = 0; j < npt; j++ )
        {
            double errij = fabs(imagePoints[0][i][j].x*lines[1][j][0] +
                                imagePoints[0][i][j].y*lines[1][j][1] + lines[1][j][2]) +
                           fabs(imagePoints[1][i][j].x*lines[0][j][0] +
                                imagePoints[1][i][j].y*lines[0][j][1] + lines[0][j][2]);
            err += errij;
        }
        npoints += npt;
    }
    cout << "average epipolar err = " <<  err/npoints << endl;

    cv::FileStorage fs("C://Users/ROBMED/Desktop/CameraCalibration/intrinsics.yml", cv::FileStorage::WRITE);
    if( fs.isOpened() )
    {
        fs << "M1" << cameraMatrix[0] << "D1" << distCoeffs[0] <<
            "M2" << cameraMatrix[1] << "D2" << distCoeffs[1];
        fs.release();
    }
    else
        cout << "Error: can not save the intrinsic parameters\n";

    cv::Mat R1, R2, P1, P2, Q;
    cv::Rect validRoi[2];

    cv::stereoRectify(cameraMatrix[0], distCoeffs[0],
                  cameraMatrix[1], distCoeffs[1],
                  imageSize, R, T, R1, R2, P1, P2, Q,
                  0,-1, imageSize, &validRoi[0], &validRoi[1]);

    fs.open("C://Users/ROBMED/Desktop/CameraCalibration/extrinsics.yml", cv::FileStorage::WRITE);
    if( fs.isOpened() )
    {
        fs << "R" << R << "T" << T << "R1" << R1 << "R2" << R2 << "P1" << P1 << "P2" << P2 << "Q" << Q;
        fs.release();
    }
    else
        cout << "Error: can not save the extrinsic parameters\n";

    // OpenCV can handle left-right
    // or up-down camera arrangementsuseCalibrated
    bool isVerticalStereo = fabs(P2.at<double>(1, 3)) > fabs(P2.at<double>(0, 3));

// COMPUTE AND DISPLAY RECTIFICATION
    if( !showRectified )
        return;

    cv::Mat rmap[2][2];
// IF BY CALIBRATED (BOUGUET'S METHOD)
    if( useCalibrated )
    {
        // we already computed everything
    }
// OR ELSE HARTLEY'S METHOD
    else
 // use intrinsic parameters of each camera, but
 // compute the rectification transformation directly
 // from the fundamental matrix
    {
        vector<cv::Point2f> allimgpt[2];
        for( k = 0; k < 2; k++ )
        {
            for( i = 0; i < nimages; i++ )
                std::copy(imagePoints[k][i].begin(), imagePoints[k][i].end(), back_inserter(allimgpt[k]));
        }
        F = cv::findFundamentalMat(cv::Mat(allimgpt[0]), cv::Mat(allimgpt[1]), cv::FM_8POINT, 0, 0);
        cv::Mat H1, H2;
        cv::stereoRectifyUncalibrated(cv::Mat(allimgpt[0]), cv::Mat(allimgpt[1]), F, imageSize, H1, H2, 3);

        R1 = cameraMatrix[0].inv()*H1*cameraMatrix[0];
        R2 = cameraMatrix[1].inv()*H2*cameraMatrix[1];
        P1 = cameraMatrix[0];
        P2 = cameraMatrix[1];
    }

    //Precompute maps for cv::remap()
    initUndistortRectifyMap(cameraMatrix[0], distCoeffs[0], R1, P1, imageSize, CV_16SC2, rmap[0][0], rmap[0][1]);/*mx1*my1*/
    initUndistortRectifyMap(cameraMatrix[1], distCoeffs[1], R2, P2, imageSize, CV_16SC2, rmap[1][0], rmap[1][1]);/*mx2*my2*/

//    initUndistortRectifyMap(cameraMatrix[0], distCoeffs[0], R1, P1, imageSize, CV_32FC1 , rmap[0][0], rmap[0][1]);/*mx1*my1*/
//    initUndistortRectifyMap(cameraMatrix[1], distCoeffs[1], R2, P2, imageSize, CV_32FC1 , rmap[1][0], rmap[1][1]);/*mx2*my2*/


    fs.open("images/rectificationMap.yml", cv::FileStorage::WRITE);
    if( fs.isOpened() )
    {
        fs << "mx1" << rmap[0][0] << "my1" << rmap[0][1] << "mx2" << rmap[1][0] << "my2" << rmap[1][1] << "Q" << Q;
        fs.release();
    }
    else
        cout << "Error: can not save the extrinsic parameters\n";


    cv::Mat canvas;
    double sf;
    int w, h;
    if( !isVerticalStereo )
    {
        sf =  600./MAX(imageSize.width, imageSize.height);
        w = cvRound(imageSize.width*sf);
        h = cvRound(imageSize.height*sf);
        canvas.create(h, w*2, CV_8UC3);
    }
    else
    {
        sf = 300./MAX(imageSize.width, imageSize.height);
        w = cvRound(imageSize.width*sf);
        h = cvRound(imageSize.height*sf);
        canvas.create(h*2, w, CV_8UC3);
    }

    for( i = 0; i < nimages; i++ )
    {
        for( k = 0; k < 2; k++ )
        {
            cv::Mat img = cv::imread(goodImageList[i*2+k], 0), rimg, cimg;
            cv::remap(img, rimg, rmap[k][0], rmap[k][1], cv::INTER_LINEAR);
            cv::cvtColor(rimg, cimg, cv::COLOR_GRAY2BGR);
            cv::Mat canvasPart = !isVerticalStereo ? canvas(cv::Rect(w*k, 0, w, h)) : canvas(cv::Rect(0, h*k, w, h));
            cv::resize(cimg, canvasPart, canvasPart.size(), 0, 0, cv::INTER_AREA);
            if( useCalibrated )
            {
                cv::Rect vroi(cvRound(validRoi[k].x*sf), cvRound(validRoi[k].y*sf),
                          cvRound(validRoi[k].width*sf), cvRound(validRoi[k].height*sf));
                cv::rectangle(canvasPart, vroi, cv::Scalar(0,0,255), 3, 8);
            }
        }

        if( !isVerticalStereo )
            for( j = 0; j < canvas.rows; j += 16 )
                cv::line(canvas, cv::Point(0, j), cv::Point(canvas.cols, j),cv::Scalar(0, 255, 0), 1, 8);
        else
            for( j = 0; j < canvas.cols; j += 16 )
                cv::line(canvas, cv::Point(j, 0), cv::Point(j, canvas.rows), cv::Scalar(0, 255, 0), 1, 8);
        cv::imshow("rectified", canvas);
        char c = (char)cv::waitKey();
        if( c == 27 || c == 'q' || c == 'Q' )
            break;
    }
    cout<<"Calibration finsihed"<<endl;
}

/*void videoDisplay::mouseClick(int event, int x, int y, int flags, void* param)
{
    g_pixelX=x;
    g_pixelY=y;
    vD.g_catpturePoints = true;
}*/


int stereo_calib::StereoCalibMain(const vector<string>& imagelist, cv::Size boardSize,bool displayCorners = true, bool useCalibrated=true, bool showRectified=true)
{
    if(imagelist.empty())
    {
        cout << "the string list is empty" << endl;
        //return print_help();
    }
    StereoCalib(imagelist, boardSize,displayCorners, useCalibrated, showRectified);
    return 0;
}

