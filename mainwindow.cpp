#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stereo_calibration.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void *MainWindow::calibrateThread(void)
{
    cv::Size boardsize;

    boardsize.width=9;//horizontal  12x7 petit   18x14 grand
    boardsize.height=6;//vertical

    stereocalib.StereoCalibMain(filelist,boardsize,true,true,true);
}

void MainWindow::on_LoadImages_clicked()
{
    // generate list of chessboard image filename Left
    for (int i=1; i<=39; i++) {
        std::string str_L,str_R;
        str_L = "C://Users/ROBMED/Desktop/Mohit/CameraCalibration/old pics/Left/Left_" + std::to_string( i ) + ".jpg";
        filelist.push_back(str_L);
        image= cv::imread(str_L,0);
                cout << "Loading Left Image......" << i << endl;

        str_R = "C://Users/ROBMED/Desktop/Mohit/CameraCalibration/old pics/Right/Right_"  + std::to_string( i ) +  ".jpg";
        filelist.push_back(str_R);
        image= cv::imread(str_R,0);
        cout << "Loading Right Image......" << i << endl;
        if(i >= 39){QMessageBox msgBox;
            msgBox.setText("Left and right Camera Images Loaded...");
            msgBox.exec();}}

        for (int i=0; i<filelist.size(); i++) {
            cout<<filelist.at(i)<<endl;
        }
        cout<<filelist.size()<<endl;
}

void MainWindow::on_Test_clicked()
{
    // read an image
    image= cv::imread("D:/Mohit/1.jpg", 1);
    // create image window named "My Image"
    cv::namedWindow("Mr. Mohit Ahuja",cv::WINDOW_NORMAL);
    // show the image on window
    cv::imshow("Mr. Mohit Ahuja", image);
}

void MainWindow::on_Camera_Parametrs_clicked()
{
    //    std::thread Tc(&MainWindow::calibrateThread,this);
    //    Tc.detach();

    calibrateThread();
}

void MainWindow::on__3D_Map_clicked()
{

   imgl = cv::imread("C://Users/ROBMED/Desktop/CameraCalibration/Left_Image.jpg", 1);
   imgr = cv::imread("C://Users/ROBMED/Desktop/CameraCalibration/Right_Image.jpg", 1);
  // cv::imshow("Left Image",imgl);
  // cv::imshow("Right Image",imgr);

   cv::Mat img = stcl.ComputeRect3dReprojectOnline( imgl, imgr, stcl.Q);
   //void cv::triangulatePoints(x,r,f)

}

void MainWindow::on_cameraParameters_clicked()
{
    bool bla = stcl.loadCameraMatrix();
}
