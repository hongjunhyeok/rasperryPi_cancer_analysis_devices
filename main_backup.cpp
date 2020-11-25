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
int sticky_x=25;
int sticky_y=305;
int x_start =80;
int y_start=200;
int x_width=600-x_start;
int y_height=5;
int x_circle=400;
int y_circle=380;
//roi}}}

//{{{algorithm
int PEAK_BOUNDARY_VALUE= 100;  // 해당 값 이하로 픽셀평균값이 내려가면 peak 검사실행.
int side_length =12;			// 픽 baseline을  구하기 위한 피크값과 양옆간의 거리차
int SIZE_BOUNDARY_VALUE= 100;
int CHECK_BAND_REVERSE_LIMIT=200;
int CHECK_BAND_REACTION_LIMIT=11;

int avg_x[601];//x_start+x_width+1
int avg_y[601];

vector<int> circle;
vector<int> sticker;
vector<int> panel_test;
//
double ratio = 0.99;
int std_ed_point[13]={0};
int std_st_point[13]={0};
//algorithm}}}



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
void stage_insert(){
    softPwmWrite(1,9);
    delay(3000);
    //softPwmStop(1);
    digitalWrite(2, HIGH);
    digitalWrite(3, HIGH);
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    softPwmWrite(1,0);
}

void DB_connect(vector<int>info,int result){
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL3");
    db.setPort(3306);

    //hosting
    db.setHostName("bic.cmjwukoyptwy.ap-northeast-2.rds.amazonaws.com");
    db.setUserName("biclab");
    db.setPassword("biolab11!");//raspberry
    db.setDatabaseName("Malaria");//db

    if(db.open()){
        qDebug()<<"Database connected";
    }

    else{
        qDebug()<<"Database connect failed";
    }

    QString username;
    QString userarea="SEOUL";
    username="BIO.IT.LAB";
    bool userResult;
    QSqlQuery query;


    query.prepare("INSERT into quickstart_patient"
                  " (`area`, `name`, `result`,"
                  "`band_1`, `band_2`, `band_3`, `band_4`,"
                  "`band_5`, `band_6`, `band_7`, `band_8`,"
                  "`band_9`, `band_10`, `band_11`, `band_12`, `band_13`)"
                  "  VALUES (:a,:b,:c,"
                  ":b1,:b2,:b3,:b4,"
                  ":b5,:b6,:b7,:b8,"
                  ":b9,:b10,:b11,:b12,:b13)");

//    query.prepare("INSERT INTO db"
//                              " area=:a,"
//                              " name=:b,"
//                              " result=:c,"
//                              " band_1=:b1,"
//                              " band_2=:b2,"
//                              " band_3=:b3,"
//                              " band_4=:b4,"
//                              " band_5=:b5,"
//                              " band_6=:b6,"
//                              " band_7=:b7,"
//                              " band_8=:b8,"
//                              " band_9=:b9,"
//                              " band_10=:b10,"
//                              " band_11=:b11"
//                              " band_12= :b12"
//                              " band_13= :b13");


    query.bindValue(":a",userarea);
    query.bindValue(":b",username);
    if(result==1)
    {
        userResult=true;
        query.bindValue(":c",userResult);
    }
        else
    {
        userResult=false;
        query.bindValue(":c",userResult);
    }
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



bool check_device_ok(vector<int>peak_area,vector<int>peak_location){
    //case 1: is chip reversed?
    if(peak_location[0]>CHECK_BAND_REVERSE_LIMIT||peak_area[0]<CHECK_BAND_REACTION_LIMIT){
        return false;
    }


    //case 2: is chip invalid?


    return true;
}
bool check_is_panel1(){
    //true = panel 1 : ink
    if(sticker[5]<50){
        return true;
    }else
        return false;

}
bool check_is_positive(vector<int>peak_area){
    int cnt=0;

    for(int i=0;i<peak_area.size()-1;i++){
    if(peak_area[i]>CHECK_BAND_REACTION_LIMIT)
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


    int most=9999;
    int peak=0;
    if(check_point-15<=x_start){
        check_point=x_start+18;
    }

    for(int i=check_point-10;i<check_point+30;i++){
        if(avg_x[i]<most-2){
            most=avg_x[i];
            peak =i;
        }

    }
    int adjust=0,abs=0;
    adjust = peak %5 ;
    abs = 5- adjust;
    if(adjust >=3){
        peak+=abs;

    }else{
        peak-=adjust;
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

        base_line = (left + right) / 2 ;


        //base_line = max(left,right);

        for (int j = l_p ;j <= r_p; j++) {
            if (avg[j] < base_line)
                area += base_line - avg[j];
        }
       // if(area<10) area=0;

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
//vector<int> get_peak(int avg[], vector<int> X) {

//    vector<int>Peak_Height;  // 넓이 반환.
//    int base_line = 0.0;
//    int left, right;
//    int l_p,r_p;
//    int area;
//    int size = X.size();
//    for (int i = 0; i < size; i++) {
//        area = 0;
//        left=0;
//        right=0;

//        if(X[i]==0){
//            Peak_Height.push_back(area);
//            continue;
//        }

//        l_p=X[i]-side_length;
//        r_p=X[i]+side_length;

//        for(int i=1;i<=5;i++){
//            left += (int)avg[l_p-3+i];
//            right +=(int)avg[r_p-3+i];
//        }
//        left/=5;
//        right/=5;

//        base_line = (left + right) / 2*ratio;

//        for (int j = l_p ;j <= r_p; j++) {
//            if (avg[j] < base_line)
//                area += base_line - avg[j];

//        }
//        Peak_Height.push_back(area);
//    }
//    return Peak_Height;
//}


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
                    std_st_point[j]=std_st_point[j-1]+33;
                    std_ed_point[j]=std_ed_point[j-1]+33;
                    location[j]=location[j-1]+33;
                }else{
                    std_st_point[j]=std_st_point[j-1]+34;
                    std_ed_point[j]=std_ed_point[j-1]+34;
                    location[j]=location[j-1]+34;

                }



//                std_line[j]=max(avg[std_st_point[j]],avg[std_ed_point[j]])-2;
                std_line[j]=tmp_st;
            }
            for(int k=1;k<=12;k++){
                std_st_point[k]+=2*k;
                std_ed_point[k]+=2*k;
            }


            base_line=std_line[0];
            first=false;
            cnt=0;
        }

        if(!first && std_ed_point[index-1]<=i&& i<=std_ed_point[index]){
            if(i>590) return location;

            if(avg[i]<std_line[index]-2){


//                qDebug()<<13-index<<i;
                location[index]=find_peak(i);


                if(index<11){
                std_st_point[index+1]=location[index]+33-15;
                std_ed_point[index+1]=location[index]+33+15;
                }
                index+=1;
            }
            else{
//                qDebug()<<13-index<<i;
                location[index]=find_peak(i);


                location[index]=std_ed_point[index]-15;
                index+=1;
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
    if(!Camera.open())
        {
            ui->plainTextEdit->appendPlainText("error : Picam not accessed successfully");
            return;
        }

        tmrTimer = new QTimer(this);
        ui->progressBar->setRange(0,100);
        ui->progressBar->setValue(0);

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

    cv::Rect roi(x_start,y_start,500,1 ); //(x_start,y_start,x_width,y_height)
    cv::Rect roi_2(x_start,y_start+7,500,1 ); //(x_start,y_start,x_width,y_height)

    cv::Rect roi_c(x_start+50,y_start-y_height,2,y_height ); //(x_start,y_start,x_width,y_height)

    cv::Rect roi_black1(150,230,500,30);
    cv::Rect roi_sticker(sticky_x,y_start,10,10);
    cv::Rect tmp1(60,y_start-5,10,10);
    cv::Point center;
    center.x=x_circle;
    center.y=y_circle;

    cv::Scalar color(122, 122, 122);
    cv::Scalar color2(100,255,100);
    cv::rectangle(image, roi, color, 2);
    cv::rectangle(image, roi_2, color, 2);


    cv::rectangle(image,roi_sticker,color2,2);
    cv::rectangle(image,tmp1,color2,1);
    cv::circle(image,center,10,color2,2);

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

//    for(int j=0;j<2;j++){
//        Camera.grab();
//        Camera.retrieve ( image);



        for(int i=0;i<x_start+x_width;i++){

                tmp.x=i;
                tmp.y=y_start+2;
                tmp_p=image.at<cv::Vec3b>(tmp);

                avg_x[i]=(tmp_p.val[1]+tmp_p.val[2]+tmp_p.val[0])/3;


            }


//    }
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


    cv::Rect roiRULER5(100, 150, 2, 20);
    cv::rectangle(image, roiRULER5, color2, 2);

    cv::Rect roiRULER6(550, 150, 2, 10);
    cv::rectangle(image, roiRULER6, color2, 2);

    cv::Rect roiRULER7(600, 150, 2, 20);
    cv::rectangle(image, roiRULER7, color2, 2);

    cv::Rect roiRULER8(150, 150, 2, 10);
    cv::rectangle(image, roiRULER8, color2, 2);

    cv::Rect roiRULER9(200, 150, 2, 20);
    cv::rectangle(image, roiRULER9, color2, 2);

    cv::Rect roiRULER10(250, 150, 2, 10);
    cv::rectangle(image, roiRULER10, color2, 2);

    cv::Rect roiRULER11(300, 150, 2, 20);
    cv::rectangle(image, roiRULER11, color2, 2);

    cv::Rect roiRULER12(350, 150, 2, 10);
    cv::rectangle(image, roiRULER12, color2, 2);

    cv::Rect roiRULER13(400,150, 2, 20);
    cv::rectangle(image, roiRULER13, color2, 2);

    cv::Rect roiRULER14(450, 150, 2, 10);
    cv::rectangle(image, roiRULER14, color2, 2);

    cv::Rect roiRULER15(500, 150, 2, 20);
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

    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    vector<int>peak_location_avg[10];
    vector<int>peak_area_avg[10];

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


    //1.
    if(IS_STAGE_IN){

        for(int i=0;i<10;i++){

        peak_location_avg[i]=pixel_data(avg_x);
        peak_area_avg[i]=get_area(avg_x,peak_location_avg[i]);

       }

        for(int i=0;i<13;i++){
            int sum_for_pl=0,sum_for_pa=0;
            int avg_for_pl=0,avg_for_pa=0;

            for(int j=0;j<10;j++){
                sum_for_pl+=peak_location_avg[j][i];
                sum_for_pa+=peak_area_avg[j][i];
            }

            avg_for_pl=sum_for_pl/10;
            avg_for_pa=sum_for_pa/10;


            peak_location[i]=avg_for_pl;
            peak_area[i]=avg_for_pa;
        }


        int size =peak_location.size();

        qDebug()<<size;

        for(int i=0;i<size;i++){
            qDebug()<<13-i<<":" <<peak_location[i]<<peak_area[i];

        }


        //2.
        if(check_device_ok(peak_area,peak_location)){

            //2-1 : set panel////////////////////////////////////
            if(check_is_panel1()){
                ui->btn_panel->setText("PANEL 1");
            }else{
                ui->btn_panel->setText("PANEL 2");
            }///////////////////////////////////////////////////

            //2-2 : .
            if(check_is_positive(peak_area)){
                int most=0;
                vector<int> geno;
                QString msg="";
                //set the INFO
                ui->lineEdit->setText(QString::number(peak_area[12]));
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


                ui->HPV_view->setText("POSITIVE");
                for(int i=size-1;i>0;i--){
                    if(peak_area[i]>0){

                        geno.push_back(size-i);        // array saved data from 13 to 1
                    }
                }
                for(int i=0;i<geno.size();i++){
                    msg+=QString::number(geno[i]);
                    msg+=" ";
                }
                ui->geno_view->setText(msg);

//                if(geno==1 || geno==3 ||geno==4 ||geno==7){
//                    ui->Risk_view->setText("High");
//                }else if(geno==2 || geno ==5 || geno==6 || geno==8){
//                    ui->Risk_view->setText("Moderate");
//                }else if(geno==9 || geno ==10 || geno ==11 || geno ==12){
//                    ui->Risk_view->setText("Low");
//                }
                //set the INFO

                DB_connect(peak_area,1);


                stage_return();
                IS_STAGE_IN=false;
                tmrTimer->stop();
                ui->PlayCameraButton->setText("Insert");

            } //2-2 : if(check_is_positive?)///
            else{


                ui->HPV_view->setText("NEGATIVE");
                ui->geno_view->setText("None");
                ui->Risk_view->setText("None");

                DB_connect(peak_area,-1);



                stage_return();
                IS_STAGE_IN=false;
                tmrTimer->stop();
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
        ui->progressBar->setValue(0);
        IS_STAGE_IN=true;
        ui->PlayCameraButton->setText("Measure");

        ui->PlayCameraButton->setEnabled(true);

        QTimer *aa=new QTimer(this);
        ui->PlayCameraButton->setEnabled(false);
        QTimer::singleShot(3000,this,SLOT(enableMyButton()));

        tmrTimer->start(100);

     }




}

void MainWindow::enableMyButton(){
    ui->PlayCameraButton->setEnabled(true);
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

void MainWindow::on_progressBar_valueChanged(int value)
{
    value = ui->progressBar->value();
    ui->progressBar->setValue(value+1);

}

