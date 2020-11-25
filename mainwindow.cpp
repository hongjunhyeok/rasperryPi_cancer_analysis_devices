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
int x_limit=130;

int sticky_y=305;
int x_start =130;
//int y_start=200;
int y_start=200;

int sticky_x=x_start-80;

int x_width=600-x_start;
int y_height=5;
int x_circle=400;
int y_circle=380;
//roi}}}

//{{{algorithm'

bool USER_MODE =true;
bool ADMIN_MODE =false;
bool mode = USER_MODE;
bool panel_mode =true;
bool PANEL_1 = true;
bool PANEL_2 = false;




int CUT_OFF_VALUE =30;          //일정 미만값을 0으로 cut-off
int PEAK_BOUNDARY_VALUE= 100;  // 해당 값 이하로 픽셀평균값이 내려가면 peak 검사실행.
int side_length =12;			// 픽 baseline을  구하기 위한 피크값과 양옆간의 거리차
int SIZE_BOUNDARY_VALUE= 50;
int CHECK_BAND_REVERSE_LIMIT=200;



int avg_x[601];//x_start+x_width+1
vector<int> frame_data;
vector<int> circle;
vector<int> sticker;
vector<int> panel_test;
//
double ratio = 0.99;
int std_ed_point[13]={0};
int std_st_point[13]={0};
//algorithm}}}

//DB{{{
QString GenoType="";
QString Risk="";
QString HostName ="bic.cmjwukoyptwy.ap-northeast-2.rds.amazonaws.com";
QString UserID = "biclab";
QString UserPW = "biolab11!";
QString DBname = "Malaria";
QString username = "Paxgen_1";
QString userarea="SEOUL";

//DB}}}




void stage_return(){
    softPwmWrite(1, 20);
    digitalWrite(2, HIGH);

    delay(1000);
    softPwmWrite(1,0);
    //softPwmStop(1);
}
void MainWindow::stage_insert(){

    softPwmWrite(1,10);
    digitalWrite(2, LOW);
    delay(3000);
    //softPwmStop(1);


    softPwmWrite(1,0);
}

int calculation_cutoff(int a){
    if(a<=CUT_OFF_VALUE)return 1;
    if(CUT_OFF_VALUE<a) return 2;
}


void DB_connect(vector<int>info,int result){
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL3");
    db.setPort(3306);

    //hosting
    db.setHostName(HostName);
    db.setUserName(UserID);
    db.setPassword(UserPW);//raspberry
    db.setDatabaseName(DBname);//db


    if(db.open()){
        qDebug()<<"Database connected";
    }

    else{
        qDebug()<<"Database connect failed";
    }


    bool userResult;
    QSqlQuery query;


    query.prepare("INSERT into `Malaria`.`db`"
                  " (`area`, `name`, `result`,`GenoType`,`Risk`,"
                  "`band_1`, `band_2`, `band_3`, `band_4`,"
                  "`band_5`, `band_6`, `band_7`, `band_8`,"
                  "`band_9`, `band_10`, `band_11`, `band_12`, `band_13`)"
                  "  VALUES (:a,:b,:c,:d,:e,"
                  ":b1,:b2,:b3,:b4,"
                  ":b5,:b6,:b7,:b8,"
                  ":b9,:b10,:b11,:b12,:b13)");

    query.bindValue(":a",userarea);
    query.bindValue(":b",username);
    if(result==1)
    {
        userResult=true;
        query.bindValue(":c","POSITIVE");
    }
        else
    {
        userResult=false;
        query.bindValue(":c","NEGATIVE");
    }
    query.bindValue(":d",GenoType);
    query.bindValue(":e",Risk);
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
void MainWindow::data_write(){

    cv::imwrite( "DiagnosisImage.jpg",image );
    IplImage* IMG=cvLoadImage("DiagnosisImage.jpg",0);
    FILE *file1 = fopen("1.txt","w");

        for (int j=y_start+2;j<y_start+3; j++)
        {
            for(int i=1;i<640;i++)
            {
                fprintf(file1,"%d \t",(unsigned char)(IMG->imageData[i+IMG->widthStep*j]));
            }
            fprintf(file1,"\n");
        }
     fclose(file1);
}


bool check_device_ok(vector<int>peak_area,vector<int>peak_location){
    //case 1: is chip reversed?
    if(peak_location[0]>x_start+52||peak_area[0]<CUT_OFF_VALUE){
        return false;
    }
    //case 2: is chip invalid?
    return true;
}
bool check_is_panel1(){
    //true = panel 1 : ink


    if(sticker[0]<70){

        return true;
    }else
        return false;

}
bool check_is_positive(vector<int>peak_area){
    int cnt=0;

    for(int i=0;i<peak_area.size()-1;i++){
    if(peak_area[i]>CUT_OFF_VALUE)
        cnt++;
    }

    if(cnt<2){
        return false;
    }
    return true;
}
void set_panel(){

}



//Fuction:: get Peak's area
//avg[] = average of RGB value of all the pixels
//vector X = position of Peaks

int find_peak(int check_point){


    int most=avg_x[check_point];
    int peak=check_point;
    if(check_point-15<=x_start){
        check_point=x_start+18;
    }

    for(int i=check_point-10;i<check_point+10;i++){
        if(avg_x[i]<=most){
            most=avg_x[i];
            peak =i;
        }

    }
    return peak;
}


int find_line(int pt_from,int pt_to){
    int line;

    int tmp_1=0;
    int tmp_2=0;
    for(int i=0;i<5;i++){
        tmp_1+=avg_x[pt_from+i-2];
        tmp_2+=avg_x[pt_to+i-2];
    }
    tmp_1/=5;
    tmp_2/=5;

    line=(tmp_1+tmp_2)/2;
    return line;
}

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
            Peak_Integral.push_back(1);
            continue;
        }

//        l_p=std_st_point[i];
//        r_p=std_ed_point[i];
        l_p=X[i]-7;
        r_p=X[i]+7;

        if(r_p>=590){
            r_p=595;
        }

        for(int k=1;k<=5;k++){
            left += (int)avg[l_p-3+k];
            right +=(int)avg[r_p-3+k];
        }

        left/=5;
        right/=5;

        base_line = (left + right) / 2 ;

        //base_line = max(left,right);

        for (int j = l_p ;j <= r_p; j++) {
            if (avg[j] < base_line)
                area += base_line - avg[j];
        }
        if(area<CUT_OFF_VALUE) area=0;

        Peak_Integral.push_back(area);

        }
    }catch(exception e){
        e.what();
    }

    return Peak_Integral;
}


//픽셀 데이터에서 peak 좌표값을 저장하는 함수.
vector<int> pixel_data(int avg[]) {
    vector<int> location(13,0);
    int index=1;
    int absol_x[13]={0};

    int std_line[13]={0};
    int base_line=0;
    int tmp_st=0;
    int cnt=0;
    bool first=true;

    for(int i=x_start+2;i<=600;i++){



        if(index>12) return location;
        tmp_st=avg[x_start-2];

        //first std setting.

        if(avg[i]<tmp_st-2 && first){

            location[0]=find_peak(i);
            absol_x[0]=location[0];

            if(i>x_start+52) return location;

            std_st_point[0] =absol_x[0]-15;
            std_ed_point[0]=absol_x[0]+15;
            std_line[0]=max(avg[std_st_point[0]],avg[std_ed_point[0]]);

            for(int j=1;j<=12;j++){

                if(j%2==0){
                    std_st_point[j]=std_st_point[j-1]+31;
                    std_ed_point[j]=std_ed_point[j-1]+31;
                    location[j]=location[j-1]+31;
                }else{
                    std_st_point[j]=std_st_point[j-1]+32;
                    std_ed_point[j]=std_ed_point[j-1]+32;
                    location[j]=location[j-1]+32;
                }

//                std_line[j]=max(avg[std_st_point[j]],avg[std_ed_point[j]])-2;
                std_line[j]=tmp_st;
            }
//            for(int k=1;k<=12;k++){
//                std_st_point[k]+=(2*k);
//                std_ed_point[k]+=(2*k);
//                location[k]+=(2*k);
//            }


            base_line=std_line[0];
            first=false;
            cnt=0;
        }

        if(!first && std_st_point[index]<=i&& i<=std_ed_point[index]){
            if(i>590) return location;

            if(avg[i]<std_line[index]-2){


//                qDebug()<<13-index<<i;
                location[index]=find_peak(i);


//                if(index<11){
//                std_st_point[index+1]=location[index]+33-15;
//                std_ed_point[index+1]=location[index]+33+15;
//                }
                index+=1;
            }
            else{
                if(i==location[index]){

//                    qDebug()<<i;
                    location[index]=find_peak(i);
                    index+=1;

                }else{

                }
            }


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
//       Camera.setAWB(0);
       if(!Camera.open())
           {
//               ui->plainTextEdit->appendPlainText("error : Picam not accessed successfully");
//               return;
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
        softPwmWrite(1,12);
        delay(100);
       // softPwmStop(1);
        softPwmWrite(1,0);
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

    cv::Rect roi(x_start,y_start,540-x_start,1 ); //(x_start,y_start,x_width,y_height)
    cv::Rect roi_2(x_start,y_start+7,540-x_start,1 ); //(x_start,y_start,x_width,y_height)

    cv::Rect roi_c(x_start+50,y_start-y_height,2,y_height ); //(x_start,y_start,x_width,y_height)

    cv::Rect roi_(sticky_x,y_start,10,10);
    cv::Point center;
    center.x=x_circle;
    center.y=y_circle;

    cv::Scalar color(122, 122, 122);
    cv::Scalar color2(100,255,100);
    cv::rectangle(image, roi, color, 2);
    cv::rectangle(image, roi_2, color, 2);
    cv::rectangle(image,roi_,color2,2);
    cv::rectangle(image,roi_c,color2,1);
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
                tmp.y=y_start+2;
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
    for(int i=sticky_x+2;i<sticky_x+8;i++){
        tmp.x=i;
        tmp.y=y_start+5;
        tmp_p=image.at<cv::Vec3b>(tmp);
        int a=(tmp_p.val[1]+tmp_p.val[2]+tmp_p.val[0])/3;
        sticker.push_back(a);
    }
    panel_test.clear();
    for(int i=61;i<70;i++){
        tmp.x=i;
        tmp.y=y_start;
        tmp_p=image.at<cv::Vec3b>(tmp);
        int a=(tmp_p.val[1]+tmp_p.val[2]+tmp_p.val[0])/3;

        panel_test.push_back(a);
    }



    QTimer::singleShot(1000,this,SLOT(set_area()));








    if(!mode){
        cv::cvtColor(image, image, CV_BGR2RGB);
        QImage qimgOriginal((uchar*)image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
        ui->VideoLabel->setPixmap(QPixmap::fromImage(qimgOriginal));
    }
}




bool isOk=false;
bool IS_STAGE_IN=false;
void MainWindow::on_PlayCameraButton_clicked()
{

    ui->PlayCameraButton->setEnabled(false);

    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    vector<int>peak_location_avg[10];
    vector<int>peak_area_avg[10];

    vector<int>peak_location(13,0);
    vector<int>peak_area(13,0);




    ui->lineEdit_13->setText(QString::number(0));
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
    ui->ResultEdit->setText("");

    //1.
    if(IS_STAGE_IN){



        ui->PlayCameraButton->setEnabled(true);

        int f_avg=0;
        int f_size=frame_data.size();
        for(int i=0;i<f_size;i++){
            f_avg+=frame_data[i];
        }
        f_avg=f_avg/f_size;

        qDebug()<<"f_avg :"<<f_avg;



        peak_location=pixel_data(avg_x);
        peak_area=get_area(avg_x,peak_location);
//        peak_area[0]=f_avg;

        int size =peak_location.size();

        data_write();

        for(int i=0;i<size;i++){
            qDebug()<<13-i<<":" <<peak_location[i]<<peak_area[i];

        }


        //2.
        if(check_device_ok(peak_area,peak_location)){

            for(int i=0;i<sticker.size();i++){
                qDebug()<<"sticker "<<sticker[i];
            }
            //2-1 : set panel////////////////////////////////////
            if(check_is_panel1()){
                ui->btn_panel->setText("PANEL 1");
                ui->label_2->setText("16");
                ui->label_3->setText("18");
                ui->label_4->setText("31");
                ui->label_5->setText("33");
                ui->label_6->setText("35");
                ui->label_7->setText("39");
                ui->label_8->setText("45");
                ui->label_9->setText("52");
                ui->label_10->setText("58");
                ui->label_11->setText("59");
                ui->label_12->setText("IC");
                panel_mode=PANEL_1;

            }else{
                ui->btn_panel->setText("PANEL 2");

                ui->label_2->setText("6");
                ui->label_3->setText("11");
                ui->label_4->setText("51");
                ui->label_5->setText("53");
                ui->label_6->setText("56");
                ui->label_7->setText("66");
                ui->label_8->setText("68");
                ui->label_9->setText("69");
                ui->label_10->setText("70");
                ui->label_11->setText("73");
                ui->label_12->setText("IC");
                panel_mode=PANEL_2;





            }///////////////////////////////////////////////////

            //2-2 : .
            if(check_is_positive(peak_area)){
                int most=0;
                vector<int> geno;
                QString msg="";
                GenoType="";
                Risk="";
                //set the INFO
                int a=0;
                a=calculation_cutoff(peak_area[0]);



                if(mode==USER_MODE){
                    if(a==1){
                        ui->lineEdit_13->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_13->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_13->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_13->setText(QString::fromLocal8Bit("양성"));
                    }

                    a=calculation_cutoff(peak_area[11]);
                    if(a==1){
                        ui->lineEdit_2->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_2->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_2->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_2->setText(QString::fromLocal8Bit("양성"));
                    }

                    a=calculation_cutoff(peak_area[10]);
                    if(a==1){
                        ui->lineEdit_3->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_3->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_3->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_3->setText(QString::fromLocal8Bit("양성"));
                    }

                    a=calculation_cutoff(peak_area[9]);
                    if(a==1){
                        ui->lineEdit_4->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_4->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_4->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_4->setText(QString::fromLocal8Bit("양성"));
                    }
                    a=calculation_cutoff(peak_area[8]);
                    if(a==1){
                        ui->lineEdit_5->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_5->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_5->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_5->setText(QString::fromLocal8Bit("양성"));
                    }

                    a=calculation_cutoff(peak_area[7]);
                    if(a==1){
                        ui->lineEdit_6->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_6->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_6->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_6->setText(QString::fromLocal8Bit("양성"));
                    }
                    a=calculation_cutoff(peak_area[6]);
                    if(a==1){
                        ui->lineEdit_7->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_7->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_7->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_7->setText(QString::fromLocal8Bit("양성"));
                    }
                    a=calculation_cutoff(peak_area[5]);
                    if(a==1){
                        ui->lineEdit_8->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_8->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_8->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_8->setText(QString::fromLocal8Bit("양성"));
                    }
                    a=calculation_cutoff(peak_area[4]);
                    if(a==1){
                        ui->lineEdit_9->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_9->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_9->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_9->setText(QString::fromLocal8Bit("양성"));
                    }
                    a=calculation_cutoff(peak_area[3]);
                    if(a==1){
                        ui->lineEdit_10->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_10->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_10->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_10->setText(QString::fromLocal8Bit("양성"));
                    }
                    a=calculation_cutoff(peak_area[2]);
                    if(a==1){
                        ui->lineEdit_11->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_11->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_11->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_11->setText(QString::fromLocal8Bit("양성"));
                    }
                    a=calculation_cutoff(peak_area[1]);
                    if(a==1){
                        ui->lineEdit_12->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
                        ui->lineEdit_12->setText(QString::fromLocal8Bit("음성"));
                    }else if(a==2){
                        ui->lineEdit_12->setStyleSheet("QLineEdit{color:rgb(255,100,100);font:'나눔고딕';}");
                        ui->lineEdit_12->setText(QString::fromLocal8Bit("양성"));
                    }



                    if(panel_mode==PANEL_1){
                        for(int i=size-1;i>0;i--){
                            if(peak_area[i]>0){

                                if(i==1) geno.push_back(12);
                                if(i==2) geno.push_back(59);
                                if(i==3) geno.push_back(58);
                                if(i==4) geno.push_back(52);
                                if(i==5) geno.push_back(45);
                                if(i==6) geno.push_back(39);
                                if(i==7) geno.push_back(35);
                                if(i==8) geno.push_back(33);
                                if(i==9) geno.push_back(31);
                                if(i==10) geno.push_back(18);
                                if(i==11) geno.push_back(16);

                            }
                        }

                    }else if(panel_mode==PANEL_2){
                        for(int i=size-1;i>0;i--){
                            if(peak_area[i]>0){


                                if(i==1) geno.push_back(12);
                                if(i==2) geno.push_back(73);
                                if(i==3) geno.push_back(70);
                                if(i==4) geno.push_back(69);
                                if(i==5) geno.push_back(68);
                                if(i==6) geno.push_back(66);
                                if(i==7) geno.push_back(56);
                                if(i==8) geno.push_back(53);
                                if(i==9) geno.push_back(51);
                                if(i==10) geno.push_back(11);
                                if(i==11) geno.push_back(6);

                            }
                        }

                    }



                    for(int i=0;i<geno.size();i++){
                        GenoType+=QString::number(geno[i]);
                        GenoType+=" ";
                        if(i%4==0) GenoType +="\n";
                    }

                    ui->ResultEdit->setStyleSheet("QLineEdit{color:rgb(0,0,0);font: 75 40pt '나눔고딕';}");
                    ui->ResultEdit->setText(GenoType);



                }else if(mode==ADMIN_MODE){
                    ui->lineEdit_13->setText(QString::number(peak_area[0]));
                    ui->lineEdit_2->setText(QString::number(peak_area[11]));
                    ui->lineEdit_3->setText(QString::number(peak_area[10]));
                    ui->lineEdit_4->setText(QString::number(peak_area[9]));
                    ui->lineEdit_5->setText(QString::number(peak_area[8]));
                    ui->lineEdit_6->setText(QString::number(peak_area[7]));
                    ui->lineEdit_7->setText(QString::number(peak_area[6]));
                    ui->lineEdit_8->setText(QString::number(peak_area[5]));
                    ui->lineEdit_9->setText(QString::number(peak_area[4]));
                    ui->lineEdit_10->setText(QString::number(peak_area[3]));
                    ui->lineEdit_11->setText(QString::number(peak_area[2]));
                    ui->lineEdit_12->setText(QString::number(peak_area[1]));


                    if(panel_mode==PANEL_1){
                        for(int i=size-1;i>0;i--){
                            if(peak_area[i]>0){

                                if(i==1) geno.push_back(12);
                                if(i==2) geno.push_back(59);
                                if(i==3) geno.push_back(58);
                                if(i==4) geno.push_back(52);
                                if(i==5) geno.push_back(45);
                                if(i==6) geno.push_back(39);
                                if(i==7) geno.push_back(35);
                                if(i==8) geno.push_back(33);
                                if(i==9) geno.push_back(31);
                                if(i==10) geno.push_back(18);
                                if(i==11) geno.push_back(16);

                            }
                        }

                    }else if(panel_mode==PANEL_2){
                        for(int i=size-1;i>0;i--){
                            if(peak_area[i]>0){


                                if(i==1) geno.push_back(12);
                                if(i==2) geno.push_back(73);
                                if(i==3) geno.push_back(70);
                                if(i==4) geno.push_back(69);
                                if(i==5) geno.push_back(68);
                                if(i==6) geno.push_back(66);
                                if(i==7) geno.push_back(56);
                                if(i==8) geno.push_back(53);
                                if(i==9) geno.push_back(51);
                                if(i==10) geno.push_back(11);
                                if(i==11) geno.push_back(6);

                            }
                        }

                    }


                    for(int i=0;i<geno.size();i++){
                        GenoType+=QString::number(geno[i]);
                        GenoType+=" ";
                        if(i%4==0) GenoType +="\n";
                    }

                    ui->ResultEdit->setStyleSheet("QLineEdit{color:rgb(0,0,0);font: 75 40pt '나눔고딕';}");
                    ui->ResultEdit->setText(GenoType);

                }


                //set the INFO

                DB_connect(peak_area,1);


                stage_return();
                IS_STAGE_IN=false;
                tmrTimer->stop();
                ui->VideoLabel->setText(QString::fromLocal8Bit("측정이 완료되었습니다.\n다음 ULFA Panel을 삽입해주세요"));

                ui->PlayCameraButton->setText("Insert");


            } //2-2 : if(check_is_positive?)///
            else{



                DB_connect(peak_area,-1);

                stage_return();
                IS_STAGE_IN=false;
                tmrTimer->stop();
//                ui->VideoLabel->setText(QString::fromLocal8Bit("칩의 상태를 확인해주세요."));
                ui->ResultEdit->setStyleSheet("QLineEdit{color:rgb(0,0,0);font: 75 40pt '나눔고딕';}");
                ui->ResultEdit->setText(QString::fromLocal8Bit("음성"));
                ui->PlayCameraButton->setText("Insert");
            }
        }//2.if(check_device_ok)///
        else{

            tmrTimer->stop();
            ui->PlayCameraButton->setText("Insert");
            QMessageBox::information(this,"INFO","Check the chip","OK");


            stage_return();

            IS_STAGE_IN=false;
        }
    }

    //1.if(is_stage_in?)///
    else
    {

        stage_insert();
        IS_STAGE_IN=true;
        ui->PlayCameraButton->setText("Measure");


        QTimer *aa=new QTimer(this);
        ui->VideoLabel->setText(QString::fromLocal8Bit("측정중... 잠시만 기다려주세요."));

//        QTimer::singleShot(3000,this,SLOT(enableMyButton()));

        tmrTimer->start(100);

        ui->lineEdit_2->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_3->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_4->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_5->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_6->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_7->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_8->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_9->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_10->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_11->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_12->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->lineEdit_13->setStyleSheet("QLineEdit{color:rgb(0,0,0);font:'나눔고딕';}");
        QTimer::singleShot(5000,this,SLOT(on_PlayCameraButton_clicked()));



     }

}

void MainWindow::enableMyButton(){
//    ui->PlayCameraButton->setEnabled(true);
    ui->VideoLabel->setText(QString::fromLocal8Bit("측정중... 잠시만 기다려주세요"));

}


void MainWindow::on_btn_panel_pressed()
{

    
       if(panel_mode)
       {
            ui->btn_panel->setText("PANEL 1");



            ui->label_2->setText("16");
            ui->label_3->setText("18");
            ui->label_4->setText("31");
            ui->label_5->setText("33");
            ui->label_6->setText("35");
            ui->label_7->setText("39");
            ui->label_8->setText("45");
            ui->label_9->setText("52");
            ui->label_10->setText("58");
            ui->label_11->setText("59");
            ui->label_12->setText("IC");






            ui->lineEdit_13->setText(QString::number(0));
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

            panel_mode=false;
       }
       else{
           ui->btn_panel->setText("PANEL 2");

           ui->label_2->setText("6");
           ui->label_3->setText("11");
           ui->label_4->setText("51");
           ui->label_5->setText("53");
           ui->label_6->setText("56");
           ui->label_7->setText("66");
           ui->label_8->setText("68");
           ui->label_9->setText("69");
           ui->label_10->setText("70");
           ui->label_11->setText("73");
           ui->label_12->setText("IC");

           ui->lineEdit_13->setText(QString::number(0));
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

           panel_mode=true;
       }
}

void MainWindow::on_pushButton_Left_clicked()
{

    if(x_start>100){
        x_start-=5;
        sticky_x-=5;
    }
    else{
        x_start=100;
        sticky_x=25;
    }
}

void MainWindow::on_pushButton_up_clicked()
{
    if(y_start>200){
        y_start-=5;
        sticky_y-=5;
    }
    else{
        y_start=230;
        sticky_y=305;
    }
}

void MainWindow::on_pushButton_Right_clicked()
{
    if(x_limit-x_start>0){
        x_start+=5;
        sticky_x+=5;
    }else{
        x_start=x_limit;
        sticky_x=305;
    }
}

void MainWindow::on_pushButton_Down_clicked()
{
    if(y_start<260){
        y_start+=5;
        sticky_y+=5;
    }
    else{
        y_start=230;
        sticky_y=305;
    }
}


void MainWindow::on_btn_exit_clicked()
{
    softPwmWrite(1,10);
    vector<int>get_data;
    get_data=frame_data;

    FILE *file1 = fopen("frame.txt","w");

        for (int j=0;j<get_data.size(); j++)
        {

            fprintf(file1,"%d\n",get_data[j]);
        }
     fclose(file1);
     this->close();

}



void MainWindow::on_btn_mode_pressed()
{
    if(mode){
        ui->btn_mode->setText(QString::fromLocal8Bit("관리자 모드"));
        ui->btn_mode->setStyleSheet("QPushButton{color:rgb(255,0,0);font:'나눔고딕';}");
        ui->VideoLabel->setText(QString::fromLocal8Bit("모드변경감지 : 관리자모드"));
        mode= ADMIN_MODE;
    }else{
        ui->btn_mode->setText(QString::fromLocal8Bit("사용자 모드"));
        ui->btn_mode->setStyleSheet("QPushButton{color:rgb(0,0,0);font:'나눔고딕';}");
        ui->VideoLabel->setText(QString::fromLocal8Bit("모드변경감지 : 사용자모드"));
        mode= USER_MODE;
    }

}


void MainWindow::set_area()
{

    vector<int>peak_location(13,0);
    vector<int>peak_area(13,0);

    peak_location=pixel_data(avg_x);
    peak_area=get_area(avg_x,peak_location);


    if(peak_area[0]>CUT_OFF_VALUE){


        qDebug()<<"area"<<peak_area[0];
        frame_data.push_back(peak_area[0]);
}

    int frame_size= frame_data.size();
    if(frame_size==120){
        frame_data.clear();
    }

}
