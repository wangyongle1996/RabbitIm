#include "LbsTrack.h"
#include "ui_LbsTrack.h"
#include <QDebug>
#include <math.h>
#include "Nmea.h"
#include <fstream>
#include <QStandardPaths>
#include <QDir>
#include <QNmeaPositionInfoSource>
#include "../Global/Log.h"

CLbsTrack::CLbsTrack(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CLbsTrack)
{
    ui->setupUi(this);
    
    m_bStart = false;
    /*默认 nmea 保存文件  
    m_NmeaSaveFile = 
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
            + QDir::separator() + "gps.nmea"; //*/
    /*默认 opents gprmc 上传   
    m_szUrl = "http://182.254.185.29:8080/gprmc/Data";
    m_szUser = "root";
    m_szDevice = "123456";//*/

#ifdef MOBILE
    m_Source = QGeoPositionInfoSource::createDefaultSource(this);
#else
    QNmeaPositionInfoSource* p = new QNmeaPositionInfoSource(
                QNmeaPositionInfoSource::SimulationMode, this);
    m_Source = p;
    if(m_Source)
    {
        m_NmeaFile.setFileName(":/file/gps.nmea");
        if(m_NmeaFile.open(QIODevice::ReadOnly))
        {   
            p->setDevice(&m_NmeaFile);
            //p->setUpdateInterval(1000);
        }
        else
            LOG_MODEL_ERROR("CLbsTrack", "don't open file:%s",
                            m_NmeaFile.fileName().toStdString().c_str());
    }
#endif
    
    if (m_Source) {
        bool check = connect(m_Source,
                             SIGNAL(positionUpdated(QGeoPositionInfo)),
                             this, SLOT(positionUpdated(QGeoPositionInfo)));
        Q_ASSERT(check);
    }
}

CLbsTrack::~CLbsTrack()
{
#ifndef MOBILE
    if(m_NmeaFile.isOpen())
    {
        m_NmeaFile.close();
    }
    if(m_Source)
        delete m_Source;
#endif    
    delete ui;
}

void CLbsTrack::positionUpdated(const QGeoPositionInfo &info)
{
    qDebug() << "Position updated:" << info;
    QString szMsg;
    szMsg = "latitude:" + QString::number(info.coordinate().latitude())
          + "\nlongitude:" + QString::number(info.coordinate().longitude())
          + "\naltitude:" + QString::number(info.coordinate().altitude())
          + "\ntime:" + info.timestamp().toString()
          + "\nspeed:" + QString::number(info.attribute(QGeoPositionInfo::GroundSpeed))
          + "\nDirection:" + QString::number(info.attribute(QGeoPositionInfo::Direction))
          + "\nHorizontalAccuracy" + QString::number(info.attribute(QGeoPositionInfo::HorizontalAccuracy));
    szMsg += "\n" + QString(CNmea::EncodeGMC(info).c_str());
    szMsg += "\n";
    ui->textBrowser->append(szMsg);

    //保存到本地文件中  
    std::ofstream out(m_NmeaSaveFile.toStdString().c_str(), std::ios::app);
    if (out.is_open()) 
        out << CNmea::EncodeGMC(info).c_str() << std::endl;
    out.close();
    
    //上传到opengts gprmce服务器  
    if(!(m_szUrl.isEmpty() || m_szUser.isEmpty() || m_szDevice.isEmpty()))
        CNmea::SendHttpOpenGts(m_szUrl.toStdString().c_str(),
                           m_szUser.toStdString().c_str(),
                           m_szDevice.toStdString().c_str(),
                           info);
}

void CLbsTrack::on_pbStart_clicked()
{
    if(m_Source)
    {
        if(m_bStart)
        {   
            m_Source->stopUpdates();
            ui->pbStart->setText(tr("Start"));
        }
        else
        {
            m_Source->startUpdates();
            ui->pbStart->setText(tr("Stop"));
        }
        m_bStart = !m_bStart;
    }
}
