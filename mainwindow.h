#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>
#include <raspicam/raspicam_cv.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/shape.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc.hpp>
#include <wiringPi.h>
#include <softPwm.h>
#include <QThread>



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();


    void processFrameAndUpdateGUI();


    void on_PlayCameraButton_clicked();


    void on_btn_panel_pressed();

    void on_pushButton_Left_clicked();

    void on_pushButton_up_clicked();

    void on_pushButton_Right_clicked();

    void on_pushButton_Down_clicked();

    void on_progressBar_valueChanged(int value);

    void enableMyButton();

    void data_write();

    void stage_insert();
    void on_btn_exit_clicked();

    void on_btn_mode_pressed();

    void set_area();

public:
    Ui::MainWindow *ui;
    raspicam::RaspiCam_Cv Camera;
    cv::Mat image;
    QImage qimgOriginal;
    QTimer*tmrTimer;
    QTimer*processTimer;


};

class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep(unsigned long secs){QThread::sleep(secs);}
};

#endif // MAINWINDOW_H
