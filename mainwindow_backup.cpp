#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <wiringPi.h>
#include <softPwm.h>

#include "stdio.h"
#include <vector>
#include <QString>
#include <algorithm>
#include <QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QtSql>
#include <QtSql/QSqlDatabase>
#include <QCoreApplication>
#include <QMessageBox>
using namespace std;
//{{{roi
int sticky_x=25;
int sticky_y=190;
int x_start =130;
int y_start=185;
int x_width=600-x_start;
int y_height=30;
int x_circle=400;
int y_circle=380;
//roi}}}

//{{{algorithm
int PEAK_BOUNDARY_VALUE= 100;  // 해당 값 이하로 픽셀평균값이 내려가면 peak 검사실행.
int side_length =12;			// 픽 baseline을  구하기 위한 피크값과 양옆간의 거리차
int SIZE_BOUNDARY_VALUE= 100;
int STD_BAND_LIMIT=200;

int avg_x[601];//x_start+x_width+1
vector<int> circle;
vector<int> sticker;

double ratio = 0.99;
int std_ed_point[12]={0};
int std_st_point[12]={0};
//algorithm}}}

bool isChip_Ok(vector<int>X,vector<int>Y){
    if(X[0]>STD_BAND_LIMIT||Y[0]<100){
        return false;
    }
    return true;
}

void stage_return(){
    softPwmWrite(1, 20);
    digitalWrite(2, HIGH);
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
    delay(1000);
    softPwmWrite(1,0);
    //softPwmStop(1);

}


void DB_connect(vector<int>info,int result){
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setUserName("ygyg331");
    db.setPassword("raspberry");
    db.setDatabaseName("db");

    if(db.open()){
        qDebug()<<"Database connected";
    }

    else{
        qDebug()<<"Database connect failed";
    }

    QString username;
    username="BIO.IT.LAB";
    QSqlQuery query;


    query.prepare("INSERT INTO data"
                  " (`number`, `Name`, `Result`,"
                  " `Band#1`, `Band#2`, `Band#3`, `Band#4`,"
                  " `Band#5`, `Band#6`, `Band#7`, `Band#8`,"
                  " `Band#9`, `Band#10`, `Band#11`, `Band#12`, `Band#13`) "

                  "VALUES (:a,:b,:c,"
                  ":b1,:b2,:b3,:b4,"
                  ":b5,:b6,:b7,:b8,"
                  ":b9,:b10,:b11,:b12,:b13)");

    query.bindValue(":a","");
    query.bindValue(":b",username);
    if(result==1)
        query.bindValue(":c","POSITIVE");
    else
        query.bindValue(":c","NEGATIVE");

    query.bindValue(":b1",info[12]);
    query.bindValue(":b2",info[11]);
    query.bindValue(":b3",info[10]);
    query.bindValue(":b4",info[9]);
    query.bindValue(":b5",info[8]);
    query.bindValue(":b6",info[7]);
    query.bindValue(":b7",info[6]);
    query.bindValue(":b8",info[5]);
    query.bindValue(":b9",info[4]);
    query.bindValue(":b10",info[3]);
    query.bindValue(":b11",info[2]);
    query.bindValue(":b12",info[1]);
    query.bindValue(":b13",info[0]);
    if(!query.exec())
        qDebug()<< query.lastError().text();
    else{
        qDebug()<<"Database Insert success";
    }


    db.close();
}

bool check_device_ok(vector<int>peak_area){
    //case 1: is chip reversed?


    //case 2: is chip invalid?
    if(peak_area[0]<5 || !peak_area[0]){
        return false;

    }

    return true;
}
bool check_is_panel1(){
    //true = panel 1
    //false = panel 2
    return true;
}
bool check_is_positive(vector<int>peak_area){

    return true;
}



//Fuction:: get Peak's area
//avg[] = average of RGB value of all the pixels
//vector X = position of Peaks
vector<int> get_area(int avg[], vector<int> X) {


    vector<int>Peak_Integral;  // 넓이 반환.
    int base_line = 0;
    int left, right;
    int l_p,r_p;
    int area;
    int size = X.size();

    try{
    for (int i = 0; i < size; i++) {
        area = 0;
        left=0;
        right=0;


        if(X[i]==0){
            Peak_Integral.push_back(area);
            continue;
        }


        l_p=std_st_point[i];
        r_p=std_ed_point[i];
        if(r_p>=590){
            r_p=595;
        }



        for(int k=1;k<=5;k++){
            left += (int)avg[l_p-3+k];
            right +=(int)avg[r_p-3+k];
        }

        left/=5;
        right/=5;

        base_line = (left + right) / 2 * ratio;
        //base_line = max(left,right);

        for (int j = l_p ;j <= r_p; j++) {
            if (avg[j] < base_line)
                area += base_line - avg[j];
        }
        if(area<10) area=0;

        Peak_Integral.push_back(area);
        }
    }catch(exception e){
        e.what();
    }

    return Peak_Integral;
}


//Fuction:: get Peak's Height
//avg[] = average of RGB value of all the pixels
//vector X = position of Peaks
vector<int> get_peak(int avg[], vector<int> X) {

    vector<int>Peak_Height;  // 넓이 반환.
    int base_line = 0.0;
    int left, right;
    int l_p,r_p;
    int area;
    int size = X.size();
    for (int i = 0; i < size; i++) {
        area = 0;
        left=0;
        right=0;

        if(X[i]==0){
            Peak_Height.push_back(area);
            continue;
        }

        l_p=X[i]-side_length;
        r_p=X[i]+side_length;

        for(int i=1;i<=5;i++){
            left += (int)avg[l_p-3+i];
            right +=(int)avg[r_p-3+i];
        }
        left/=5;
        right/=5;

        base_line = (left + right) / 2*ratio;

        for (int j = l_p ;j <= r_p; j++) {
            if (avg[j] < base_line)
                area += base_line - avg[j];

        }
        Peak_Height.push_back(area);
    }
    return Peak_Height;
}


//픽셀 데이터에서 peak 좌표값을 저장하는 함수.
vector<int> pixel_data(int avg[]) {
    vector<int> location(13,0);
    int index=1;
    int absol_x[12]={0};

    int std_line[12]={0};
    int base_line=0;
    int tmp_st=0;
    int cnt=0;
    bool first=true;
    //1.187
    //2.221
    //3.252
    //4.281
    //5.312
    //6.343
    //7.375
    //8.407
    //9439
    //10. 470
    //11.501
    for(int i=x_start+10;i<=600;i++){
        if(index>11) return location;
        tmp_st=avg[x_start+10];

        //first std setting.

        if(avg[i]<tmp_st-4 && first){
            location[0]=i+4;
            absol_x[0]=i+4;

            qDebug()<<tmp_st << avg[i] << i;

            std_st_point[0] =absol_x[0]-15;
            std_ed_point[0]=absol_x[0]+15;
            std_line[0]=max(avg[std_st_point[0]],avg[std_ed_point[0]]);

            for(int j=1;j<12;j++){
                if(j%2==0){
                    std_st_point[j]=std_st_point[j-1]+37;
                    std_ed_point[j]=std_ed_point[j-1]+37;
                }else{
                    std_st_point[j]=std_st_point[j-1]+38;
                    std_ed_point[j]=std_ed_point[j-1]+38;
                }
                std_line[j]=max(avg[std_st_point[j]],avg[std_ed_point[j]])-1;
            }
            base_line=std_line[0];
            first=false;
            cnt=0;
        }

        if(!first && avg[i] <=std_line[index]&& i>=std_st_point[index]){
            if(i>590) return location;


//            cnt =0;
            location[index]=i+4;
            absol_x[index]=i+4;
//            base_line=(avg[std_st_point[index]]+avg[std_ed_point[index]])/2*ratio;
            i=std_ed_point[index];
            index+=1;


        }
    }



    return location;
}



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    QMainWindow::showNormal();
    wiringPiSetup();

    softPwmCreate(1,20,100);
    pinMode(2,OUTPUT);
    pinMode(3,OUTPUT);
    pinMode(4,OUTPUT);
    pinMode(5,OUTPUT);

    Camera.set(CV_CAP_PROP_FORMAT, CV_8UC3);
    Camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    Camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    if(!Camera.open())
        {
            ui->plainTextEdit->appendPlainText("error : Picam not accessed successfully");
            return;
        }

        tmrTimer = new QTimer(this);
        connect(tmrTimer, SIGNAL(timeout()), this, SLOT(processFrameAndUpdateGUI()));
        tmrTimer->start(50);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{

        digitalWrite(2, HIGH);
        digitalWrite(3, HIGH);
        digitalWrite(4, HIGH);
        digitalWrite(5, HIGH);
}

void MainWindow::on_pushButton_2_clicked()
{
        digitalWrite(2, LOW);
        digitalWrite(3, LOW);
        digitalWrite(4, LOW);
        digitalWrite(5, LOW);
}

void MainWindow::on_pushButton_3_clicked()
{
    //softPwmCreate(1,13,100);
    //softPwmWrite(1,13);
    softPwmWrite(1,13);
    delay(100);
   // softPwmStop(1);
    softPwmWrite(1,0);

}

void MainWindow::on_pushButton_4_clicked()
{

    //softPwmCreate(1,25,100);
    //softPwmWrite(1,25);
    softPwmWrite(1,20);
    delay(100);
    //softPwmStop(1);
    softPwmWrite(1,0);

}





void MainWindow::processFrameAndUpdateGUI()
{
    Camera.grab();
    Camera.retrieve(image);
    if(image.empty() == true)
        return;

    cv::Vec3b rgb;

    cv::Rect roi(x_start,y_start,x_width,y_height ); //(x_start,y_start,x_width,y_height)
    cv::Rect roi_black1(x_circle,y_circle,30,10);
    cv::Rect roi_sticker(sticky_x,sticky_y,10,10);
    cv::Point center;
    center.x=x_circle;
    center.y=y_circle;

    cv::Scalar color(122, 122, 122);
    cv::Scalar color2(100,255,100);
    cv::rectangle(image, roi, color, 2);
    cv::rectangle(image,roi_sticker,color2,2);
    cv::circle(image,center,10,color2,2);

//    cv::rectangle(image,roi_black1,color,1);
//    cv::rectangle(image,roi_black2,color,1);
    cv::Point roi_point;
    roi_point.x = 100;
    roi_point.y = y_start+5;
    rgb = image.at<cv::Vec3b>(roi_point);
    int r = rgb.val[2];
    int g = rgb.val[1];
    int b = rgb.val[0];
    PEAK_BOUNDARY_VALUE=(r+g+b)/3;
    cv::Point tmp;
    cv::Vec3b tmp_p;
    for(int i=0;i<x_start+x_width;i++){

            tmp.x=i;
            tmp.y=y_start+10;
            tmp_p=image.at<cv::Vec3b>(tmp);
            avg_x[i]=(tmp_p.val[1]+tmp_p.val[2]+tmp_p.val[0])/3;

    }
    circle.clear();
    for(int i=x_circle-5;i<x_circle+5;i++){
        tmp.x=i;
        tmp.y=y_circle;
        tmp_p=image.at<cv::Vec3b>(tmp);
        int a=(tmp_p.val[1]+tmp_p.val[2]+tmp_p.val[0])/3;

        circle.push_back(a);
    }
    sticker.clear();
    for(int i=sticky_x;i<sticky_x+10;i++){
        tmp.x=i;
        tmp.y=sticky_y+5;
        tmp_p=image.at<cv::Vec3b>(tmp);
        int a=(tmp_p.val[1]+tmp_p.val[2]+tmp_p.val[0])/3;

        sticker.push_back(a);
    }
    cv::Rect roiRULER5(x_start, 50, 2, 20);
    cv::rectangle(image, roiRULER5, color2, 2);

    cv::Rect roiRULER6(x_start+50, 50, 2, 10);
    cv::rectangle(image, roiRULER6, color2, 2);

    cv::Rect roiRULER7(x_start+100, 50, 2, 20);
    cv::rectangle(image, roiRULER7, color2, 2);

    cv::Rect roiRULER8(x_start+150, 50, 2, 10);
    cv::rectangle(image, roiRULER8, color2, 2);

    cv::Rect roiRULER9(x_start+200, 50, 2, 20);
    cv::rectangle(image, roiRULER9, color2, 2);

    cv::Rect roiRULER10(x_start+250, 50, 2, 10);
    cv::rectangle(image, roiRULER10, color2, 2);

    cv::Rect roiRULER11(x_start+300, 50, 2, 20);
    cv::rectangle(image, roiRULER11, color2, 2);

    cv::Rect roiRULER12(x_start+350, 50, 2, 10);
    cv::rectangle(image, roiRULER12, color2, 2);

    cv::Rect roiRULER13(x_start+400, 50, 2, 20);
    cv::rectangle(image, roiRULER13, color2, 2);

    cv::Rect roiRULER14(x_start+450, 50, 2, 10);
    cv::rectangle(image, roiRULER14, color2, 2);

    cv::Rect roiRULER15(x_start+500, 50, 2, 20);
    cv::rectangle(image, roiRULER15, color2, 2);




    ui->plainTextEdit->appendPlainText(QString("Red average : ")+QString::number(r).rightJustified(4, ' ')
                                       +QString("Green average : ")+QString::number(g).rightJustified(4, ' ')
                                       +QString("Blue average : ")+QString::number(b).rightJustified(4, ' '));

    cv::cvtColor(image, image, CV_BGR2RGB);
    QImage qimgOriginal((uchar*)image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
    ui->VideoLabel->setPixmap(QPixmap::fromImage(qimgOriginal));

}




bool isOk=false;
bool IS_STAGE_IN=false;
void MainWindow::on_PlayCameraButton_clicked()
{

    vector<int>peak_location(13,0);
    vector<int>peak_area(13,0);
    ui->HPV_view->setText("");
    ui->Risk_view->setText("");
    ui->geno_view->setText("");
    ui->lineEdit->setText(QString::number(0));
    ui->lineEdit_2->setText(QString::number(0));
    ui->lineEdit_3->setText(QString::number(0));
    ui->lineEdit_4->setText(QString::number(0));
    ui->lineEdit_5->setText(QString::number(0));
    ui->lineEdit_6->setText(QString::number(0));
    ui->lineEdit_7->setText(QString::number(0));
    ui->lineEdit_8->setText(QString::number(0));
    ui->lineEdit_9->setText(QString::number(0));
    ui->lineEdit_10->setText(QString::number(0));
    ui->lineEdit_11->setText(QString::number(0));
    ui->lineEdit_12->setText(QString::number(0));



    if(IS_STAGE_IN){

        if(check_device_ok){
        peak_location=pixel_data(avg_x);
        peak_area=get_area(avg_x,peak_location);

        else{

        }
    }
    //if(is_stage_in?)///
    else
    {
        tmrTimer->start(20);
        //softPwmCreate(1,13,100);
        //softPwmWrite(1, 13);
        IS_STAGE_IN=true;
        softPwmWrite(1,12);
        delay(3000);
        //softPwmStop(1);
        digitalWrite(2, HIGH);
        digitalWrite(3, HIGH);
        digitalWrite(4, LOW);
        digitalWrite(5, LOW);
        softPwmWrite(1,0);

        ui->PlayCameraButton->setText("Measure");
    }
}

bool flag =true;
void MainWindow::on_btn_panel_pressed()
{


       if(flag)
       {
            ui->btn_panel->setText("PANEL 1");


            ui->HPV->setText("PANEL 1");
            ui->Risk->setText("PANEL 1");
            ui->GenoType->setText("PANEL 1");
            ui->lineEdit->setText(QString::number(0));
            ui->lineEdit_2->setText(QString::number(0));
            ui->lineEdit_3->setText(QString::number(0));
            ui->lineEdit_4->setText(QString::number(0));
            ui->lineEdit_5->setText(QString::number(0));
            ui->lineEdit_6->setText(QString::number(0));
            ui->lineEdit_7->setText(QString::number(0));
            ui->lineEdit_8->setText(QString::number(0));
            ui->lineEdit_9->setText(QString::number(0));
            ui->lineEdit_10->setText(QString::number(0));
            ui->lineEdit_11->setText(QString::number(0));
            ui->lineEdit_12->setText(QString::number(0));

            flag=false;
       }
       else{
           ui->btn_panel->setText("PANEL 2");
           ui->HPV->setText("PANEL 2");
           ui->Risk->setText("PANEL 2");
           ui->GenoType->setText("PANEL 2");
           ui->lineEdit->setText(QString::number(0));
           ui->lineEdit_2->setText(QString::number(0));
           ui->lineEdit_3->setText(QString::number(0));
           ui->lineEdit_4->setText(QString::number(0));
           ui->lineEdit_5->setText(QString::number(0));
           ui->lineEdit_6->setText(QString::number(0));
           ui->lineEdit_7->setText(QString::number(0));
           ui->lineEdit_8->setText(QString::number(0));
           ui->lineEdit_9->setText(QString::number(0));
           ui->lineEdit_10->setText(QString::number(0));
           ui->lineEdit_11->setText(QString::number(0));
           ui->lineEdit_12->setText(QString::number(0));

           flag=true;
       }
}
