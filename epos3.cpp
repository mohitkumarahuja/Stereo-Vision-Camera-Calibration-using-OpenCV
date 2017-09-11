#include "epos3.h"
#include "ui_epos3.h"
#include "Epos3ReadWriteLib.h"
#include "cammonitor.h"
#include "opencv_lib.h"
#include <string.h>
#include <QElapsedTimer>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <math.h>
#include <stdio.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>




#include <stdio.h>
#include <stdlib.h>                                     (1)
#define IMAGE_FILE_NAME "Image.pgm"




using namespace std;

Epos3ReadWriteLib epos3Lib;
camera cm;
camera cm1;
stereoCalibrate sC;
ContentFinder finder;
ColorHistogram hc;

bool g_relative_abs = false; //false->abs - true->relative
bool g_relative_abs1 = false; //false->abs - true->relative
bool g_positionRecord=false;
bool g_velocityRecord=false;
bool g_recorder=false;
QString g_targetPosition="";
QString g_targetPosition1="";
QString camshiftangle1="";
QString camshiftangle2="";
QString camshiftangle1_1="";
QString camshiftangle2_1="";
QString str_camshiftDiff="";
float float_camshiftDiff=0;
QString str_camshiftDiff_1="";
float float_camshiftDiff_1=0;

double g_angle=0;
double g_angleFelxion=0;
double g_angleRotation=0;
double g_gearReductionFlexion=64;
double g_gearReductionRotation=256;
double g_gearReduction=43;
double g_lpr=254;
int qc=0;
int flagROI=0;


epos3::epos3(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::epos3)
{
    ui->setupUi(this);

}

epos3::~epos3()
{
    delete ui;
}
//restart button
void epos3::on_pushButton_9_clicked()
{
    epos3Lib.initEPOS3();
}
//pause button
void epos3::on_pushButton_3_clicked()
{
    epos3Lib.sendDownloadCommand("0","0x6040","0","uint16","0x017f");
}
//stop button
void epos3::on_pushButton_5_clicked()
{
    epos3Lib.sendDownloadCommand("0","0x6040","0","uint16","0x02");
}

void epos3::on_pushButton_7_clicked()
{
    epos3Lib.sendDownloadCommand("1","0x6040","0","uint16","0x017f");
}

void epos3::on_pushButton_8_clicked()
{
    epos3Lib.sendDownloadCommand("1","0x6040","0","uint16","0x02");
}
void epos3::on_pushButton_10_clicked()
{
//    QString sampleRate;
//    int index,index2,motor0, motor1;

//    index2 = ui->comboBox_2->currentIndex();

//    /*For recordering mode/Buffer Recorder*/

//    sampleRate = ui->lineEdit->text();

//    epos3Lib.sendDownloadCommand("0", "0x2010", "0", "uint16","0x01");//start recording
//    epos3Lib.sendDownloadCommand("0", "0x2011", "0", "uint16","0x01");//trigger at movement start
//    epos3Lib.sendDownloadCommand("0", "0x2012", "0", "uint16",sampleRate.toStdString());//samplig period at 1ms
//    epos3Lib.sendDownloadCommand("0", "0x2013", "0", "uint16","0");//
//    epos3Lib.sendDownloadCommand("0", "0x2014", "0", "uint16","1");//Number of variables

//    if(index2==0)//position
//    {
//        epos3Lib.sendDownloadCommand("0", "0x2015", "1", "uint16","0x6064");
//    }
//    else if(index2==1)//velocity
//    {
//        epos3Lib.sendDownloadCommand("0", "0x2015", "1", "uint16","0x606C");
//    }
//    else if(index2==2)//current
//    {
//        epos3Lib.sendDownloadCommand("0", "0x2015", "1", "uint16","0x6078");
//    }
//    else if(index2==3)//torque
//    {
//        epos3Lib.sendDownloadCommand("0", "0x2015", "1", "uint16","0x6077");
//    }

//    epos3Lib.sendDownloadCommand("0", "0x2016", "0", "uint16","0");//subindex
}

/*GetValues Actual values**********************************************************************/

void epos3::on_pushButton_11_clicked()
{
    int operationMode_int;
    int actualPosition_int;
    int actualVelocity_int;
    int controlWord_int;
    int statusWord_int;
    int recoderStatus_int;
    int angleDoub = 0;

    operationMode_int = epos3Lib.sendUploadCommand("0", "0x6060", "0", "uint8");
    actualPosition_int = epos3Lib.sendUploadCommand("0", "0x6064", "0", "uint32");
    actualVelocity_int = epos3Lib.sendUploadCommand("0", "0x2028", "0", "uint32");//Velocity Actual Value
    controlWord_int = epos3Lib.sendUploadCommand("0", "0x6040", "0", "uint16");
    statusWord_int = epos3Lib.sendUploadCommand("0", "0x6041", "0", "uint16");
    recoderStatus_int = epos3Lib.sendUploadCommand("0", "0x2017", "0", "uint16");

    angleDoub = (actualPosition_int*360/(4*g_lpr))/g_gearReduction;

    ui->lcdNumber_5->display(operationMode_int);
    ui->lcdNumber_4->display(angleDoub);
    ui->lcdNumber_3->display(actualVelocity_int);
    ui->lcdNumber_2->display(controlWord_int);
    ui->lcdNumber->display(statusWord_int);
    ui->lcdNumber_11->display(recoderStatus_int);

    /*read buffer*/

    epos3Lib.sendUploadCommandBuffer("0", "0x201B", "0", "string");
    //epos3Lib.sendDownloadCommand("0", "0x2010", "0", "uint16","0");


    //offsetDataBuffer_int = epos3Lib.sendUploadCommand("0", "0x2019", "0", "uint16");


    /***********************************************************************/
    //Creating the graph

    QVector<double> x(256), y(256), in(256); // initialize with entries 0..100
    QVector<double> temp(x.size());
    int input = 0;
    int max,min,index2;
    vector<int>yArray(x.size());
    vector<int>xArray(x.size());
    QString sampleRateStr = "1"; //1 ms

    index2 = ui->comboBox_2->currentIndex();


    if(index2==0)//position
    {
        input = ui->lineEdit_11->text().toInt();
    }
    else if(index2==1)//velocity
    {
        input = ui->lineEdit_2->text().toInt();
    }
//    else if(index2==2)//current
//    {
//        input = epos3Lib.sendUploadCommand("0", "0x2031", "0", "uint16");
//    }
//    else if(index2==3)//torque
//    {
//        input = epos3Lib.sendUploadCommand("0", "0x6071", "0", "uint16");
//    }


    sampleRateStr = ui->lineEdit->text();
    cout<<"sampleRateStr: "<<sampleRateStr.toStdString() <<endl;
    for (int i=0; i<256; ++i)
    {
      x[i] = i*0.1*sampleRateStr.toInt();

      //y[i] = (double)atof(epos3Lib.dataBuffer[i].c_str());
      if(index2==0)
      {
          y[i] = (std::atoi(epos3Lib.dataBuffer[i].c_str())) * 360/(4*g_lpr*g_gearReduction);
      }
      else if(index2==1){
        y[i] = std::atoi(epos3Lib.dataBuffer[i].c_str());
      }
      in[i] = input;
      if(y[i]>(pow(2,32)/2 - 1 ))
      {
          y[i] = (pow(2,32)-y[i])*-1.0;
      }
      yArray[i] = y[i];
      //y[i]= 0xffffffff ^ y[i];

    }


    max = *max_element(yArray.begin(),yArray.end());
    min = *min_element(yArray.begin(),yArray.end());


    // create graph and assign data to it:
    ui->widget->addGraph();
    ui->widget->graph(0)->setData(x, y);
    ui->widget->addGraph();
    ui->widget->graph(1)->setPen(QPen(Qt::red));
    ui->widget->graph(1)->setData(x, in);
    // give the axes some labels:
    ui->widget->xAxis->setLabel("time(ms)");
    ui->widget->yAxis->setLabel("Output");
    ui->widget->yAxis2->setLabel("In");
    // set axes ranges, so we see all data:
    ui->widget->xAxis->setRange(x[0], x[255]);
    ui->widget->yAxis->setRange(min, max);
    ui->widget->yAxis2->setRange(0, in[0]);


    ui->widget->replot();

}

//move backward
void epos3::on_pushButton_clicked()
{    
    QString targetVelocity;
    char newTargetPosition_chr[11];
    string newTargetPosition_str;
    int newTargetPosition_int;

    //targetPosition = ui->lineEdit->text();
    targetVelocity = ui->lineEdit_2->text();

    newTargetPosition_int = 0xffffffff - g_targetPosition.toInt() + 1;

    sprintf(newTargetPosition_chr,"%x",newTargetPosition_int);

    newTargetPosition_str = "0x" + string(newTargetPosition_chr);

    epos3Lib.initEPOS3();
    epos3Lib.moveMotor("0", newTargetPosition_str,targetVelocity.toStdString(),g_relative_abs );
}
//move foreward
void epos3::on_pushButton_2_clicked()
{

    //QString targetPosition,
    QString targetVelocity;

    targetVelocity = ui->lineEdit_11->text();
    epos3Lib.initEPOS3();
    epos3Lib.moveMotor("0",g_targetPosition.toStdString(),targetVelocity.toStdString(),g_relative_abs);

}

void epos3::on_pushButton_14_clicked()
{
    /*cout<<"Write test"<<endl;
    for(int i=0; i<100; i++)
    {
        cout<<i<<" ";
        epos3Lib.sendDownloadCommand("0", "0x60FF", "0", "uint32","950");
    }*/
    //cout<<"Read test"<<endl;
    for(int k=0; k<1000; k++)
    {
        cout<<k<<" ";
        epos3Lib.sendUploadCommand("0", "0x207C", "1", "int16");
        epos3Lib.sendUploadCommand("0", "0x207C", "2", "int16");
    }


}
//Applying settings

void epos3::on_pushButton_15_clicked()
{
    int index,index2,motor0, motor1;
    QString targetAngle;
    QString targetVelocity;


    char tempDouble[9];

    index = ui->comboBox->currentIndex();
    index2 = ui->comboBox_2->currentIndex();


    if (index==2)//PPM Relative
    {
        epos3Lib.sendDownloadCommand("0", "0x6060", "0", "uint8","0x01");
        g_relative_abs = true;
    }
    if (index==3)//PPM absolute
    {
        epos3Lib.sendDownloadCommand("0", "0x6060", "0", "uint8","0x01");
        g_relative_abs = false;
    }
    else if(index==0)//PVM
    {
        epos3Lib.sendDownloadCommand("0", "0x6060", "0", "uint8","0x03");
    }
    else if(index==1)//CSV
    {
        epos3Lib.sendDownloadCommand("0", "0x6060", "0", "uint8","0x09");
    }

    targetAngle = ui->lineEdit_11->text();
    g_angle = targetAngle.toDouble(); //radians

    //radians
    //qc = 4*(g_angle*gearReduction)*lpr*180/360;
    //degrees
    qc = 4*(g_angle*g_gearReductionFlexion)*g_lpr/360;

//    targetPosition = ui->lineEdit_11->text();

    sprintf(tempDouble,"%d",qc);
    g_targetPosition = tempDouble;


    targetVelocity = ui->lineEdit_2->text();

    if(g_targetPosition !=""){
        epos3Lib.sendDownloadCommand("0", "0x607A", "0", "uint32",g_targetPosition.toStdString());
    }

    if(targetVelocity !=""){
        epos3Lib.sendDownloadCommand("0", "0x60FF", "0", "uint32",targetVelocity.toStdString());
    }
}

void epos3::on_horizontalSlider_sliderMoved(int position)
{
    ui->lcdNumber_14->display(ui->horizontalSlider->sliderPosition());
}

void epos3::on_horizontalSlider_sliderPressed()
{
    //ui->lcdNumber_14->display(ui->horizontalSlider->valueChanged(););
}

void epos3::on_horizontalSlider_2_sliderMoved(int position)
{
    ui->lcdNumber_13->display(ui->horizontalSlider_2->sliderPosition());
}

void epos3::on_horizontalSlider_sliderReleased()
{
    int position;
    char positionChr[5];
    QString targetVelocity;

    position = ui->horizontalSlider->sliderPosition();
    position = 64*position;

    sprintf(positionChr,"%d",position);
    targetVelocity = positionChr;

    //position = ui->horizontalSlider->sliderPosition();
    cout << "targetVelocity 1: " << targetVelocity.toStdString() << endl;
    //epos3Lib.sendDownloadCommand("0", "0x60FF", "0", "uint32",targetVelocity.toStdString());
}

void epos3::on_horizontalSlider_2_sliderReleased()
{
    int position;
    char positionChr[5];
    QString targetVelocity;

    position = ui->horizontalSlider_2->sliderPosition();
    position = 256*position;

    sprintf(positionChr,"%d",position);
    targetVelocity = positionChr;

    //position = ui->horizontalSlider_2->sliderPosition();
    cout << "targetVelocity 2: "<< targetVelocity.toStdString() << endl;
    //epos3Lib.sendDownloadCommand("1", "0x60FF", "0", "uint32",targetVelocity.toStdString());
}
//get values for rotation
void epos3::on_pushButton_18_clicked()
{
    int operationMode_int;
    int actualPosition_int;
    int actualVelocity_int;
    int controlWord_int;
    int statusWord_int;
    int recoderStatus_int;
    int angleDoub = 0;

    operationMode_int = epos3Lib.sendUploadCommand("1", "0x6060", "0", "uint8");
    actualPosition_int = epos3Lib.sendUploadCommand("1", "0x6064", "0", "uint32");
    actualVelocity_int = epos3Lib.sendUploadCommand("1", "0x2028", "0", "uint32");//Velocity Actual Value Averaged
    controlWord_int = epos3Lib.sendUploadCommand("1", "0x6040", "0", "uint16");
    statusWord_int = epos3Lib.sendUploadCommand("1", "0x6041", "0", "uint16");
    recoderStatus_int = epos3Lib.sendUploadCommand("1", "0x2017", "0", "uint16");

    angleDoub = (actualPosition_int*360/(4*g_lpr))/g_gearReductionRotation;

    ui->lcdNumber_6->display(operationMode_int);
    ui->lcdNumber_7->display(angleDoub);
    ui->lcdNumber_8->display(actualVelocity_int);
    ui->lcdNumber_9->display(controlWord_int);
    ui->lcdNumber_10->display(statusWord_int);
    ui->lcdNumber_12->display(recoderStatus_int);

    /*read buffer*/

    epos3Lib.sendUploadCommandBuffer("1", "0x201B", "0", "string");

    /***********************************************************************/
    //Creating the graph

    QVector<double> x(256), y(256), in(256); // initialize with entries 0..100
    QVector<double> temp(x.size());
    int input = 0;
    int max,min,index2;
    vector<int>yArray(x.size());
    vector<int>xArray(x.size());
    QString sampleRateStr = "1"; //1 ms

    index2 = ui->comboBox_4->currentIndex();


    if(index2==0)//position
    {
        input = ui->lineEdit_3->text().toInt();
    }
    else if(index2==1)//velocity
    {
        input = ui->lineEdit_10->text().toInt();
    }


    sampleRateStr = ui->lineEdit_12->text();
    cout<<"sampleRateStr: "<<sampleRateStr.toStdString() <<endl;
    for (int i=0; i<256; ++i)
    {
      x[i] = i*0.1*sampleRateStr.toInt();

      cout<<"X: "<<x[i]<<endl;

      //y[i] = (double)atof(epos3Lib.dataBuffer[i].c_str());
      if(index2==0)
      {
          y[i] = (std::atoi(epos3Lib.dataBuffer[i].c_str())) * 360/(4*g_lpr*g_gearReduction);
      }
      else if(index2==1){
        y[i] = std::atoi(epos3Lib.dataBuffer[i].c_str());
      }
      in[i] = input;
      if(y[i]>(pow(2,32)/2 - 1 ))
      {
          y[i] = (pow(2,32)-y[i])*-1.0;
      }
      yArray[i] = y[i];
      //y[i]= 0xffffffff ^ y[i];

    }


    max = *max_element(yArray.begin(),yArray.end());
    min = *min_element(yArray.begin(),yArray.end());


    // create graph and assign data to it:
    ui->widget->addGraph();
    ui->widget->graph(0)->setData(x, y);
    ui->widget->addGraph();
    ui->widget->graph(1)->setPen(QPen(Qt::red));
    ui->widget->graph(1)->setData(x, in);
    // give the axes some labels:
    ui->widget->xAxis->setLabel("time(ms)");
    ui->widget->yAxis->setLabel("Output");
    ui->widget->yAxis2->setLabel("In");
    // set axes ranges, so we see all data:
    ui->widget->xAxis->setRange(x[0], x[255]);
    ui->widget->yAxis->setRange(min, max);
    ui->widget->yAxis2->setRange(0, in[0]);


    ui->widget->replot();
}

//Start recorder for flexion
void epos3::on_pushButton_19_clicked()
{
    QString sampleRate;
    int index2;

    index2 = ui->comboBox_2->currentIndex();

    /*For recordering mode/Buffer Recorder*/

    sampleRate = ui->lineEdit->text();

    epos3Lib.sendDownloadCommand("0", "0x2010", "0", "uint16","0x01");//start recording
    epos3Lib.sendDownloadCommand("0", "0x2011", "0", "uint16","0x01");//trigger at movement start
    epos3Lib.sendDownloadCommand("0", "0x2012", "0", "uint16",sampleRate.toStdString());//samplig period at 1ms
    epos3Lib.sendDownloadCommand("0", "0x2013", "0", "uint16","0");//
    epos3Lib.sendDownloadCommand("0", "0x2014", "0", "uint16","1");//Number of variables

    if(index2==0)//position
    {
        epos3Lib.sendDownloadCommand("0", "0x2015", "1", "uint16","0x6064");
    }
    else if(index2==1)//velocity
    {
        epos3Lib.sendDownloadCommand("0", "0x2015", "1", "uint16","0x606C");
    }
    else if(index2==2)//current
    {
        epos3Lib.sendDownloadCommand("0", "0x2015", "1", "uint16","0x6078");
    }
    else if(index2==3)//torque
    {
        epos3Lib.sendDownloadCommand("0", "0x2015", "1", "uint16","0x6077");
    }

    epos3Lib.sendDownloadCommand("0", "0x2016", "0", "uint16","0");//subindex
}

void epos3::on_pushButton_20_clicked()
{
    QString sampleRate;
    int index2;

    index2 = ui->comboBox_4->currentIndex();

    /*For recordering mode/Buffer Recorder*/

    sampleRate = ui->lineEdit_12->text();

    epos3Lib.sendDownloadCommand("1", "0x2010", "0", "uint16","0x01");//start recording
    epos3Lib.sendDownloadCommand("1", "0x2011", "0", "uint16","0x01");//trigger at movement start
    epos3Lib.sendDownloadCommand("1", "0x2012", "0", "uint16",sampleRate.toStdString());//samplig period at 1ms
    epos3Lib.sendDownloadCommand("1", "0x2013", "0", "uint16","0");//
    epos3Lib.sendDownloadCommand("1", "0x2014", "0", "uint16","1");//Number of variables

    if(index2==0)//position
    {
        epos3Lib.sendDownloadCommand("1", "0x2015", "1", "uint16","0x6064");
    }
    else if(index2==1)//velocity
    {
        epos3Lib.sendDownloadCommand("1", "0x2015", "1", "uint16","0x606C");
    }
    else if(index2==2)//current
    {
        epos3Lib.sendDownloadCommand("1", "0x2015", "1", "uint16","0x6078");
    }
    else if(index2==3)//torque
    {
        epos3Lib.sendDownloadCommand("1", "0x2015", "1", "uint16","0x6077");
    }

    epos3Lib.sendDownloadCommand("1", "0x2016", "0", "uint16","0");//subindex
}

//OpenCV and Camera Basler

void epos3::on_pushButton_21_clicked()
{
    //cv::Mat image = cv::imread("jaymi_image1.jpg");

    //cm.cameraMain(0);
    cv::Mat image;
    cv::Mat imageBGR,imageBGRx,imageBGR2,imageBGR3,hsv1,hsv2,result1,result2,imageROI_1,imageROI_2;
    cv::Mat imageBGR_1,imageBGRx_1,imageBGR2_1,imageBGR3_1,hsv1_1,hsv2_1,result1_1,result2_1,imageROI_1_1,imageROI_2_1;
    cv::Mat imageReduced;  
    cv::MatND colorhist,colorhist_1;
    int ch[1];
    ch[0]=0;

    cv::Scalar color(255,255,255);
    cv::Point p1(10,100);
//    cv::Point p2(407,248);
    int minSat=65;


    std::vector<cv::Mat> v1,v1_1,v2,V2_1;
    cv::Rect rect1(0,0,20,20);
    cv::Rect rect2(0,0,20,20);
    cv::Rect rect1_1(0,0,20,20);
    cv::Rect rect2_1(0,0,20,20);
    int xdown,ydown,h,w,twoObject;

    cv::Point2f vertex1[4];
    cv::Point2f vertex2[4];
    cv::Point2f vertex1_1[4];
    cv::Point2f vertex2_1[4];

    cv::RotatedRect trackBox1,trackBox1_1,trackBox2,trackBox2_1;
    char key;

    flagROI=0;

    QElapsedTimer timer,timer2, timer3;
    float extraTime = 0;
    float elapsedTime = 0;
    int angleDiff = 0;
    int angleDiff_1 = 0;

    double frameRate=10;

    //cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 10, 1 );
    cv::TermCriteria criteria(cv::TermCriteria::MAX_ITER,10,0.01);

    //Calibration******************************************
    sC.loadCameraMatrix();
    //Calibration******************************************

    cv::namedWindow("Camera 0",1);
    cv::namedWindow("Camera 1",1);


    cm.cameraMain(1);
    cm.settings.num_buffers=2;
    cm.settings.framerate = frameRate;
    cm.set_framerate_jaymi(cm.c_handle,frameRate);
    cm.set_buffers_jaymi(cm.c_handle,&cm.settings.num_buffers);

    cm1.cameraMain(0);
    cm1.settings.num_buffers=2;
    cm1.settings.framerate = frameRate;
    cm1.set_framerate_jaymi(cm1.c_handle,frameRate);
    cm1.set_buffers_jaymi(cm1.c_handle,&cm.settings.num_buffers);

    cout<<"framerate: "<<cm.settings.framerate<<endl;
    cout<<"num_buffers: "<<cm.settings.num_buffers<<endl;
    cout<<"maxval: "<<cm.settings.maxval<<endl;
    cout<<"width: "<<cm.settings.width<<endl;
    cout<<"height: "<<cm.settings.height<<endl;
    cout<<"trigger: "<<cm.settings.trigger<<endl;
    //cout<<"timestamp: "<<cm.timestamp<<endl;

    char ch_contador[2];
    string scontador;
    int contador = 0;

    //sC.loadCameraMatrix();

    for(;;)
    {

         imageBGR = cm.cameraImage();

         imageBGR_1= cm1.cameraImage();
         cv::imshow("Camera 1",imageBGR_1);

        //Select ROI 1
        /********************************************************************************/
        if(flagROI==0)
        {
            cv::imshow("Camera 0",imageBGR);
            cv::setMouseCallback("Camera 0", onmouse, NULL);
            xdown=finder.mouseX_down;
            ydown=finder.mouseY_down;
            rect1.x=xdown;
            rect1.y=ydown;
        }
        else if(flagROI==1){

            h=finder.mouseX_up-finder.mouseX_down;
            w=finder.mouseY_up-finder.mouseY_down;

            if(h<0)
            {
                h = -1*h;
                rect1.x=finder.mouseX_up;
                rect1.y=finder.mouseY_up;
            }
            if(w<0)
            {
                w = -1*w;
                rect1.x=finder.mouseX_up;
                rect1.y=finder.mouseY_up;
            }

            rect1.width=w;
            rect1.height=h;

            //ROI
            imageROI_1= imageBGR(rect1);
            // Get the Hue histogram for object 1
            colorhist=hc.getHueHistogram(imageROI_1,minSat);
            finder.setHistogram(colorhist,0,0);

            colorhist_1=hc.getHueHistogram(imageROI_1,minSat);
            finder.setHistogram(colorhist,0,1);

            if(!twoObject)
                flagROI=8;
            else
                flagROI=2;
        }
        //Select ROI 2
        /********************************************************************************/
        else if((flagROI==2)&&(twoObject))
        {
            cv::putText(imageBGR,"Select ROI 2",p1,CV_FONT_NORMAL,1,color,1,cv::LINE_8,false);

            cv::imshow("Camera 0",imageBGR);
            cv::setMouseCallback("Camera 0", onmouse, NULL);
            xdown=finder.mouseX_down;
            ydown=finder.mouseY_down;
            rect2.x=xdown;
            rect2.y=ydown;
            //flagROI==3;
        }
        else if((flagROI==3)&&(twoObject))
        {

            h=finder.mouseX_up-finder.mouseX_down;
            w=finder.mouseY_up-finder.mouseY_down;

            if(h<0)
            {
                cv::imshow("Camera 0",imageBGR);
                h = -1*h;
                rect2.x=finder.mouseX_up;
                rect2.y=finder.mouseY_up;
            }
            if(w<0)
            {
                w = -1*w;
                rect2.x=finder.mouseX_up;
                rect2.y=finder.mouseY_up;
            }

            rect2.width=w;
            rect2.height=h;

            //ROI
            imageROI_2= imageBGR(rect2);
            // Get the Hue histogram for object 1
            colorhist=hc.getHueHistogram(imageROI_2,minSat);
            finder.setHistogram(colorhist,1,0);

            colorhist_1=hc.getHueHistogram(imageROI_2,minSat);
            finder.setHistogram(colorhist_1,1,1);

            flagROI=8;
        }
        //********************************************************************************/
        //Tracking objects
        //********************************************************************************/
        else if (flagROI==8)
        {

            //imageBGRx = cm.cameraImage();
            imageBGRx = imageBGR;
            imageBGR2 = imageBGRx;
            imageBGR3 = imageBGRx;

            imageBGRx_1 = imageBGR_1;
            imageBGR2_1 = imageBGRx_1;
            imageBGR3_1 = imageBGRx_1;


            /********************************************************************************/
            // Convert to HSV space
            cv::cvtColor(imageBGR2, hsv1, CV_BGR2HSV);
            cv::cvtColor(imageBGR2_1, hsv1_1, CV_BGR2HSV);
            // Split the image
            cv::split(hsv1,v1);
            cv::split(hsv1_1,v1_1);
            // Identify pixels with low saturation
            cv::threshold(v1[1],v1[1],minSat,255,cv::THRESH_BINARY);
            cv::threshold(v1_1[1],v1_1[1],minSat,255,cv::THRESH_BINARY);
            // Get back-projection of hue histogram
            result1= finder.find(hsv1,0.0f,180.0f,ch,1,0,0);
            result1_1= finder.find(hsv1_1,0.0f,180.0f,ch,1,0,1);
            // Eliminate low stauration pixels
            cv::bitwise_and(result1,v1[1],result1);
            cv::bitwise_and(result1_1,v1_1[1],result1_1);

            //Applying Camshift algorithm
            trackBox1 = cv::CamShift(result1,rect1,criteria);
            trackBox1_1 = cv::CamShift(result1_1,rect1_1,criteria);
            cout<<"Obj1 cam0: "<<trackBox1.center.x<<endl;
            cout<<"Obj1 cam0: "<<trackBox1.center.y<<endl;


            //Drawing the polyline to track the ROI
            trackBox1.points(vertex1);
            trackBox1_1.points(vertex1_1);
            for (int i = 0; i < 4; i++){
                cv::line(imageBGRx, vertex1[i], vertex1[(i+1)%4], cv::Scalar(255,255,255),2);
                cv::line(imageBGRx_1, vertex1_1[i], vertex1_1[(i+1)%4], cv::Scalar(255,255,255),2);
            }

            camshiftangle1 = QString::number(trackBox1.angle);
            camshiftangle1_1 = QString::number(trackBox1_1.angle);
            cv::putText(imageBGRx,"FLAG = 4",p1,CV_FONT_NORMAL,1,color,1,cv::LINE_8,false);
            cv::putText(imageBGRx_1,"FLAG = 4",p1,CV_FONT_NORMAL,1,color,1,cv::LINE_8,false);
            //cv::putText(imageBGRx,"ANGLE 1 = " + camshiftangle1.toStdString(),2*p1,CV_FONT_NORMAL,0.50,color,1,cv::LINE_8,false);

            //*******************************************************************************/
            if(twoObject){

            // Convert to HSV space
            cv::cvtColor(imageBGR3, hsv2, CV_BGR2HSV);
            cv::cvtColor(imageBGR3_1, hsv2_1, CV_BGR2HSV);
            // Split the image
            cv::split(hsv2,v2);
            cv::split(hsv2_1,V2_1);
            // Identify pixels with low saturation
            cv::threshold(v2[1],v2[1],minSat,255,cv::THRESH_BINARY);
            cv::threshold(V2_1[1],V2_1[1],minSat,255,cv::THRESH_BINARY);
            // Get back-projection of hue histogram
            result2= finder.find(hsv2,0.0f,180.0f,ch,1,1,0);
            result2_1= finder.find(hsv2_1,0.0f,180.0f,ch,1,1,1);
            // Eliminate low stauration pixels
            cv::bitwise_and(result2,v2[1],result2);
            cv::bitwise_and(result2_1,V2_1[1],result2_1);
            //Applying Camshift algorithm
            trackBox2 = cv::CamShift(result2,rect2,criteria);
            trackBox2_1 = cv::CamShift(result2_1,rect2_1,criteria);
            //Drawing the polyline to track the ROI
            trackBox2.points(vertex2);
            trackBox2_1.points(vertex2_1);

            cout<<"Obj2 cam0: "<<trackBox2.center.x<<endl;
            cout<<"Obj2 cam0: "<<trackBox2.center.y<<endl;

            for (int i = 0; i < 4; i++){
                cv::line(imageBGRx, vertex2[i], vertex2[(i+1)%4], cv::Scalar(255,255,255),2);
                cv::line(imageBGRx_1, vertex2_1[i], vertex2_1[(i+1)%4], cv::Scalar(255,255,255),2);
            }
            camshiftangle2 = QString::number(trackBox2.angle);
            camshiftangle2_1 = QString::number(trackBox2_1.angle);

            float_camshiftDiff = camshiftangle1.toFloat()-camshiftangle2.toFloat();
            float_camshiftDiff_1 = camshiftangle1_1.toFloat()-camshiftangle2_1.toFloat();
            angleDiff = float_camshiftDiff;
            angleDiff_1 = float_camshiftDiff_1;
            float_camshiftDiff = round(angleDiff);
            float_camshiftDiff_1 = round(angleDiff_1);
            str_camshiftDiff = QString::number(float_camshiftDiff);
            str_camshiftDiff_1 = QString::number(float_camshiftDiff_1);
            //cv::putText(imageBGRx,"ANGLE 2 = " + camshiftangle2.toStdString(),2.5*p1,CV_FONT_NORMAL,0.50,color,1,cv::LINE_8,false);


            cv::putText(imageBGRx,"ANGLE DIFF = " + str_camshiftDiff.toStdString(),3*p1,CV_FONT_NORMAL,0.50,color,1,cv::LINE_8,false);
            cv::putText(imageBGRx_1,"ANGLE DIFF = " + str_camshiftDiff_1.toStdString(),3*p1,CV_FONT_NORMAL,0.50,color,1,cv::LINE_8,false);

//            cv::namedWindow("Image 3",1);
//            cv::imshow("Image 3",result2);

            }





            //cv::namedWindow("Image 1",1);
            cv::imshow("Camera 0",imageBGRx);
            cv::setMouseCallback("Camera 0", onmouse, NULL);
            cv::imshow("Camera 1",imageBGRx_1);
            cv::setMouseCallback("Camera 1", onmouse, NULL);


            //cv::setMouseCallback("Image 2", onmouse, NULL);


//            cv::namedWindow("Image 2",1);
//            cv::imshow("Image 2",result1);

//            cv::Mat jaimy;

//            jaimy = cv::imread("FOTO JAYMI.png");
            /********************************************************************************/
//            cv::cvtColor(imageBGRx, image, CV_BGR2RGB);
//            if(!image.data)
//            {
//                QErrorMessage *errorMessageDialog;
//                errorMessageDialog = new QErrorMessage(this);
//                errorMessageDialog->showMessage(tr("Error"));
//                delete errorMessageDialog;
//            }
//            cv::Size size(340,251);
//            cv::resize(image,imageReduced,size);
//            QImage img = QImage((const unsigned char*)(imageReduced.data),imageReduced.cols,imageReduced.rows,imageReduced.cols*3, QImage::Format_RGB888);

//            ui->label_27->setPixmap(QPixmap::fromImage(img));
//            ui->label_27->resize(ui->label_27->pixmap()->size());
        }


//        cv::Mat imgl=cv::imread("calibration/l0.jpg",0);
//        cv::Mat imgr=cv::imread("calibration/r0.jpg",0);

//        cv::namedWindow("Original left",1 );
//        cv::imshow("Original left", imgl );


        sC.ComputeRect3dReprojectOnline(imageBGR,imageBGR_1,sC.gmx1,sC.gmy1,sC.gmx2,sC.gmy2,sC.g_Q_mat);



        //key=cv::waitKey(60/cm.settings.framerate);
        key=cv::waitKey(1);

                if(key=='r')
                {
                    flagROI=0;
                    contador = 0;
                }
                else if(key=='0')
                {
                    cout << "saving images"<<endl;
                    cout << "contador: "<<contador<<endl;
                    sprintf(ch_contador,"%d",contador);
                    contador=contador+1;
                    scontador = ch_contador;
                    cout << "scontador: "<<scontador<<endl;
                    imwrite( "calibration/l"+scontador+".jpg", imageBGR );
                    imwrite( "calibration/r"+scontador+".jpg", imageBGR_1 );
                }
                else if(key=='1')
                {
                    twoObject=0;
                    flagROI=0;
                }
                else if(key=='2')
                {
                    twoObject=1;
                    flagROI=0;
                }
                else if(key=='3')
                {
                    cm.settings.activity=cm.settings.stopped;
                    cm.set_stop(cm.c_handle);
                    cm1.settings.activity=cm1.settings.stopped;
                    cm1.set_stop(cm1.c_handle);

                    cm.cameraMain(0);
                    cm.settings.num_buffers=2;
                    cm.settings.framerate = frameRate;
                    cm.set_framerate_jaymi(cm.c_handle,frameRate);
                    cm.set_buffers_jaymi(cm.c_handle,&cm.settings.num_buffers);

                    cm1.cameraMain(1);
                    cm1.settings.num_buffers=2;
                    cm1.settings.framerate = frameRate;
                    cm1.set_framerate_jaymi(cm1.c_handle,frameRate);
                    cm1.set_buffers_jaymi(cm1.c_handle,&cm.settings.num_buffers);

                    cout<<"framerate: "<<cm.settings.framerate<<endl;
                    cout<<"num_buffers: "<<cm.settings.num_buffers<<endl;
                    cout<<"maxval: "<<cm.settings.maxval<<endl;
                    cout<<"width: "<<cm.settings.width<<endl;
                    cout<<"height: "<<cm.settings.height<<endl;
                    cout<<"trigger: "<<cm.settings.trigger<<endl;
                    contador = 0;
                }
                else if(key=='4')
                {
                    frameRate=25;
                    cm.settings.activity=cm.settings.stopped;
                    cm.set_stop(cm.c_handle);
                    cm1.settings.activity=cm1.settings.stopped;
                    cm1.set_stop(cm1.c_handle);

                    cm.cameraMain(1);
                    cm.settings.num_buffers=2;
                    cm.settings.framerate = frameRate;
                    cm.set_framerate_jaymi(cm.c_handle,frameRate);
                    cm.set_buffers_jaymi(cm.c_handle,&cm.settings.num_buffers);

                    cm1.cameraMain(0);
                    cm1.settings.num_buffers=2;
                    cm1.settings.framerate = frameRate;
                    cm1.set_framerate_jaymi(cm1.c_handle,frameRate);
                    cm1.set_buffers_jaymi(cm1.c_handle,&cm.settings.num_buffers);

                    cout<<"framerate: "<<cm.settings.framerate<<endl;
                    cout<<"num_buffers: "<<cm.settings.num_buffers<<endl;
                    cout<<"maxval: "<<cm.settings.maxval<<endl;
                    cout<<"width: "<<cm.settings.width<<endl;
                    cout<<"height: "<<cm.settings.height<<endl;
                    cout<<"trigger: "<<cm.settings.trigger<<endl;
                    contador = 0;
                }

    }/*for*/
}

void epos3::on_pushButton_22_clicked()
{
    //sC.StereoCalib("calibration/calibration_list.txt",6,6);
    //sC.stereoCalibopencv(0,"calibration/calibration_list.txt",6,9);


    cv::Mat imgl=cv::imread("calibration/l0.jpg",0);
    cv::Mat imgr=cv::imread("calibration/r0.jpg",0);

    cv::namedWindow("Original left",1 );
    cv::imshow("Original left", imgl );

    sC.loadCameraMatrix();
    sC.ComputeRect3dReprojectOnline(imgl,imgr,sC.gmx1,sC.gmy1,sC.gmx2,sC.gmy2,sC.g_Q_mat);
    cout<<"Camera calibration finished"<<endl;
}
/****************************************************************************/
void epos3::onmouse(int event, int x, int y, int flags, void* param){

    if((event==CV_EVENT_LBUTTONDOWN)&&(flagROI==0))
    {
        finder.mouseX_down=x;
        finder.mouseY_down=y;       
        flagROI=0;
    }
    else if((event==CV_EVENT_LBUTTONUP)&&(flagROI==0))
    {
        finder.mouseX_up=x;
        finder.mouseY_up=y;
        flagROI=1;
    }
    if((event==CV_EVENT_LBUTTONDOWN)&&(flagROI==2))
    {
        finder.mouseX_down=x;
        finder.mouseY_down=y;
        flagROI=2;
    }
    else if((event==CV_EVENT_LBUTTONUP)&&(flagROI==2))
    {
        finder.mouseX_up=x;
        finder.mouseY_up=y;
        flagROI=3;
    }
    else if((event==CV_EVENT_LBUTTONDOWN)&&(flagROI==4))
    {
        finder.mouseX_down=x;
        finder.mouseY_down=y;
        flagROI=4;
    }
    else if((event==CV_EVENT_LBUTTONUP)&&(flagROI==4))
    {
        finder.mouseX_up=x;
        finder.mouseY_up=y;
        flagROI=5;
    }
    if((event==CV_EVENT_LBUTTONDOWN)&&(flagROI==6))
    {
        finder.mouseX_down=x;
        finder.mouseY_down=y;
        flagROI=6;
    }
    else if((event==CV_EVENT_LBUTTONUP)&&(flagROI==6))
    {
        finder.mouseX_up=x;
        finder.mouseY_up=y;
        flagROI=7;
    }
}
/****************************************************************************/


void epos3::on_pushButton_16_clicked()
{
    int index,index2,motor0, motor1;
    QString targetAngle;
    QString targetVelocity;


    char tempDouble[9];

    index = ui->comboBox_3->currentIndex();
    index2 = ui->comboBox_4->currentIndex();


    if (index==2)//PPM Relative
    {
        epos3Lib.sendDownloadCommand("1", "0x6060", "0", "uint8","0x01");
        g_relative_abs1 = true;
    }
    if (index==3)//PPM absolute
    {
        epos3Lib.sendDownloadCommand("1", "0x6060", "0", "uint8","0x01");
        g_relative_abs1 = false;
    }
    else if(index==0)//PVM
    {
        epos3Lib.sendDownloadCommand("1", "0x6060", "0", "uint8","0x03");
    }
    else if(index==1)//CSV
    {
        epos3Lib.sendDownloadCommand("1", "0x6060", "0", "uint8","0x09");
    }

    targetAngle = ui->lineEdit_11->text();
    g_angle = targetAngle.toDouble(); //radians

    //radians
    //qc = 4*(g_angle*gearReduction)*lpr*180/360;
    //degrees
    qc = 4*(g_angle*g_gearReductionRotation)*g_lpr/360;

//    targetPosition = ui->lineEdit_11->text();

    sprintf(tempDouble,"%d",qc);
    g_targetPosition1 = tempDouble;


    targetVelocity = ui->lineEdit_2->text();

    if(g_targetPosition1 !=""){
        epos3Lib.sendDownloadCommand("1", "0x607A", "0", "uint32",g_targetPosition1.toStdString());
    }

    if(targetVelocity !=""){
        epos3Lib.sendDownloadCommand("1", "0x60FF", "0", "uint32",targetVelocity.toStdString());
    }
}

void epos3::on_pushButton_6_clicked()
{
    //QString targetPosition,
    QString targetVelocity;

    //targetPosition = ui->lineEdit->text();
    targetVelocity = ui->lineEdit_10->text();
    epos3Lib.initEPOS3();
    epos3Lib.moveMotor("1",g_targetPosition1.toStdString(),targetVelocity.toStdString(),g_relative_abs1);
}

void epos3::on_pushButton_4_clicked()
{
    QString targetVelocity;
    char newTargetPosition_chr[11];
    string newTargetPosition_str;
    int newTargetPosition_int;

    //targetPosition = ui->lineEdit->text();
    targetVelocity = ui->lineEdit_10->text();

    newTargetPosition_int = 0xffffffff - g_targetPosition1.toInt() + 1;

    sprintf(newTargetPosition_chr,"%x",newTargetPosition_int);

    newTargetPosition_str = "0x" + string(newTargetPosition_chr);

    epos3Lib.initEPOS3();
    epos3Lib.moveMotor("1", newTargetPosition_str,targetVelocity.toStdString(),g_relative_abs1 );
}

void epos3::on_pushButton_17_clicked()
{

}

void epos3::on_pushButton_12_clicked()
{
    QString str_squareSize;
    float squareSize;
    str_squareSize = ui->lineEdit_4->text();
    squareSize = str_squareSize.toFloat();
    //sC.StereoCalib("calibration/calibration_list.txt",6,6);
    sC.stereoCalibopencv(0,squareSize,"calibration/calibration_list.txt",9,6);
    cout<<"Camera calibration finished"<<endl;
}




















