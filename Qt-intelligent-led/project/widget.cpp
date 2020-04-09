
#include "widget.h"
#include "ui_widget.h"
#include <qdatetime.h>


#define MAX_LIGHT_LEVEL 100
#define MAX_COLOR_LEVEL 100
#define MIN_LIGHT_LEVEL 0
#define MIN_COLOR_LEVEL 0
#define TEMP_RANGE 40
#define SLEEP_TIME 10


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("LED 灯光控制"));
    //初始化变量
    mainSwitch = false;
    sleepSwitch = false;
    temp = 25;
    hum = 30;
    realTime = "-";
    lightLevel = 0;
    colorLevel = 0;
    selectMode = 0;
    customLight = 0;
    customColor = 0;
    sleepMsc = 0;
    distance = 0;
    bestdistance = 0;
    alert = false;
    alertcount = 0;
    alertflag = 0;
    //初始化串口
    //查找可用的串口
    foreach (const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        QSerialPort serial;
        serial.setPort(info);
        if(serial.open(QIODevice::ReadWrite))
        {
            ui->PortBox->addItem(serial.portName());
            serial.close();
        }
    }
    openSerial();
    initled();
    mainSwitch_off();
    //定时器
    QObject::connect(timer,SIGNAL(timeout()),this,SLOT(start_counting()));
    timer->start(1000);
    QObject::connect(serial,&QSerialPort::readyRead,this,&Widget::recieve_data);
}

Widget::~Widget()
{
    delete ui;
    serial->clear();
    serial->close();
    serial->deleteLater();
}
void Widget::openSerial()
{
    serial = new QSerialPort;
    serial->setPortName(ui->PortBox->currentText());//设置串口名
    serial->open(QIODevice::ReadWrite);//打开串口
    serial->setBaudRate(QSerialPort::Baud9600);//设置波特率为9600
    serial->setDataBits(QSerialPort::Data8);//设置数据位8
    serial->setParity(QSerialPort::NoParity);//设置校验位
    serial->setStopBits(QSerialPort::OneStop);//设置停止位为1
    serial->setFlowControl(QSerialPort::NoFlowControl);//设置为无流控制


}

void Widget::initled()
{
    //初始化滑动条
    ui->lightlevelslider->setMinimum(MIN_LIGHT_LEVEL);
    ui->lightlevelslider->setMaximum(MAX_LIGHT_LEVEL);
    ui->colorlevelslider->setMinimum(MIN_COLOR_LEVEL);
    ui->colorlevelslider->setMaximum(MAX_COLOR_LEVEL);
    ui->colorlevelslider->setValue(MIN_COLOR_LEVEL);
    on_lightlevelslider_valueChanged(0);
    ui->colorlevel->setText("0");
    ui->colorlevelset->setText("0");
    ui->lightlevelslider->setValue(MIN_LIGHT_LEVEL);
    ui->lightlevel->setText("0");
    ui->lightlevelset->setText("0");
    on_colorlevelslider_valueChanged(0);

    //初始化温湿度
    ui->temp->setText(QString::number(temp));
    ui->hum->setText(QString::number(hum));
    //初始化距离
    ui->distance->setText(QString::number(distance));
    ui->bestdistance->setText(QString::number(bestdistance));
    ui->alert->setText("OFF");

    //初始化MODE
    ui->sleepmode->setChecked(false);
    ui->aimode->setChecked(false);
    ui->custommode->setChecked(false);



}
void Widget::mainSwitch_off()
{
    ui->group2->setDisabled(true);
    ui->group3->setDisabled(true);
}

void Widget::mainSwitch_on()
{
    ui->group2->setDisabled(false);
    ui->group3->setDisabled(false);
    ui->aimode->setChecked(true);
    on_aimode_clicked(true);

}
void Widget::start_counting()
{
    QDateTime time = QDateTime::currentDateTime();
    realTime = time.toString("yyyy-MM-dd hh:mm:ss");
    ui->time->setText(realTime);

    if(sleepSwitch == false)    sleepMsc = 0;
    else
    {
        ++sleepMsc;
        if(sleepMsc == SLEEP_TIME)
        {
            sleepMsc = 0;
            sleepSwitch = false;
            on_mainswitch_clicked();
        }
    }
    if(ui->aimode->isChecked())
        on_aimode_clicked(true);
    //提示灯触发
    if(alert)
    {
        alertcount++;
        if(alertcount >= 10)
        {
            ui->alert->setText("ON");
            alertflag = 1;
            send_data(lightLevel,colorLevel,alertflag);
        }
    }
    else
    {
        ui->alert->setText("OFF");
        alertcount = 0;
        alertflag = 0;
    }


}

void Widget::send_data(int lightlevel,int colorlevel,int alertflag)
{

    QString light_ge = QString::number(lightlevel/10);
    QString light_shi = QString::number(lightlevel%10);
    QString color_ge = QString::number(colorlevel/10);
    QString color_shi = QString::number(colorlevel%10);
    QString aflag = QString::number(alertflag);

    serial->write("D");
    serial->write(light_shi.toLatin1());
    serial->write(light_ge.toLatin1());
    serial->write(color_shi.toLatin1());
    serial->write(color_ge.toLatin1());
    serial->write(aflag.toLatin1());
}

bool Widget::recieve_data()
{
    QByteArray buf =  serial->readAll();
    if(!buf.isEmpty())
    {
        int wendu_shi = int(buf[1] - '0');
        int wendu_ge = int(buf[2] - '0');
        temp = wendu_shi*10 + wendu_ge;

        int hum_shi = int(buf[3] - '0');
        int hum_ge = int(buf[4] - '0');
        hum = hum_shi *10 + hum_ge;
        if(temp == 0 || hum == 0)
        {
            temp = 29;
            hum = 52;
        }

        ui->temp->setText(QString::number(temp));
        ui->hum->setText(QString::number(hum));


        distance = int(buf[5] - '0')*100 + int(buf[6] - '0')*10 + int(buf[7] - '0');
        ui->distance->setText(QString::number(distance));
        if(distance < bestdistance - 10)
        {
            alert = true;

        }
        else    alert = false;

        buf.clear();
        return true;
    }
    else
    {
        buf.clear();
        return false;
    }
}


void Widget::on_mainswitch_clicked()
{
    if(mainSwitch)
    {
        mainSwitch = false;
        initled();
        mainSwitch_off();
        ui->mainswitch->setChecked(false);
    }
    else
    {
        mainSwitch = true;
        initled();
        mainSwitch_on();
        ui->mainswitch->setChecked(true);
    }
}


void Widget::on_lightlevelslider_valueChanged(int value)
{
    lightLevel = value;
    QString qcurrentLight = QString::number(value);
    ui->lightlevel->setText(qcurrentLight);
    ui->lightlevelset->setText(qcurrentLight);
    ui->lightlevelslider->setValue(qcurrentLight.toInt());
    send_data(lightLevel,colorLevel,alertflag);
}

void Widget::on_colorlevelslider_valueChanged(int value)
{
    colorLevel = value;
    QString qcurrentColor = QString::number(value);
    ui->colorlevel->setText(qcurrentColor);
    ui->colorlevelset->setText(qcurrentColor);
    ui->colorlevelslider->setValue(qcurrentColor.toInt());
    send_data(lightLevel,colorLevel,alertflag);
}

void Widget::on_lightlevelsub_pressed()
{
    --lightLevel;
    if(lightLevel < MIN_LIGHT_LEVEL)    lightLevel = MIN_LIGHT_LEVEL;
    on_lightlevelslider_valueChanged(lightLevel);
}


void Widget::on_lightleveladd_pressed()
{
    ++lightLevel;
    if(lightLevel > MAX_LIGHT_LEVEL)    lightLevel = MAX_LIGHT_LEVEL;
    on_lightlevelslider_valueChanged(lightLevel);
}


void Widget::on_colorlevelsub_pressed()
{
    --colorLevel;
    if(colorLevel < MIN_COLOR_LEVEL)    colorLevel = MIN_COLOR_LEVEL;
    on_colorlevelslider_valueChanged(colorLevel);
}

void Widget::on_colorleveladd_pressed()
{
    ++colorLevel;
    if(colorLevel > MAX_COLOR_LEVEL)    colorLevel = MAX_COLOR_LEVEL;
    on_colorlevelslider_valueChanged(colorLevel);
}

void Widget::on_save_clicked()
{
    customLight = ui->lightlevelset->text().toInt();
    customColor = ui->colorlevelset->text().toInt();
}

void Widget::on_custommode_clicked()
{
    if(ui->custommode->isChecked())
    {
        sleepSwitch = false;
        on_colorlevelslider_valueChanged(customColor);
        on_lightlevelslider_valueChanged(customLight);
    }
}


void Widget::on_aimode_clicked(bool checked)
{
    if(checked)
    {
        sleepSwitch = false;
        lightLevel = 80;
        colorLevel = (TEMP_RANGE - temp) * (MAX_LIGHT_LEVEL - MIN_LIGHT_LEVEL) / TEMP_RANGE;
        on_lightlevelslider_valueChanged(lightLevel);
        on_colorlevelslider_valueChanged(colorLevel);
    }
}


void Widget::on_sleepmode_clicked()
{
    sleepSwitch = true;
}



void Widget::on_bestdistanceset_clicked()
{
    bestdistance = distance;
    ui->bestdistance->setText(QString::number(bestdistance));
}
