#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);

    bool mainSwitch;    //总开关
    bool sleepSwitch;   //睡眠模式开关
    int temp;          //温度
    int hum;           //湿度
    QString realTime;      //实时时间
    int lightLevel;    //亮度
    int colorLevel;    //色温
    int selectMode;    //模式
    int customLight;   //自定义亮度
    int customColor;   //自定义色温
    int sleepMsc;       //睡眠模式秒数
    int distance;       //距离
    int bestdistance;   //最佳坐姿距离
    bool alert;
    int alertcount;
    int alertflag;

    QTimer *timer=new QTimer();
    QTimer *sleeptimer=new QTimer();
    void openSerial();
    void mainSwitch_off();
    void mainSwitch_on();
    void send_data(int,int,int);
    bool recieve_data();
    void initled();
    ~Widget();

private slots:
    void on_mainswitch_clicked();

    void start_counting();

    void on_lightlevelslider_valueChanged(int value);

    void on_colorlevelslider_valueChanged(int value);

    void on_lightlevelsub_pressed();

    void on_lightleveladd_pressed();

    void on_colorlevelsub_pressed();

    void on_colorleveladd_pressed();

    void on_save_clicked();

    void on_custommode_clicked();

    void on_aimode_clicked(bool checked);

    void on_sleepmode_clicked();

    void on_bestdistanceset_clicked();

private:
    Ui::Widget *ui;
    QSerialPort *serial;
};

#endif // WIDGET_H
