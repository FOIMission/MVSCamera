// DeviceMonitorWorker.h
#include <QObject>
#include <QTimer>
#include "mvscamera.h"

class DeviceMonitorWorker : public QObject
{
    Q_OBJECT
public:
    explicit DeviceMonitorWorker(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void startMonitoring() {
        if (!m_timer) {
            m_timer = new QTimer(this);
            connect(m_timer, &QTimer::timeout, this, &DeviceMonitorWorker::checkDevices);
            m_timer->start(4999);
        }
        // 立即执行一次
        checkDevices();
    }

    void stopMonitoring() {
        if (m_timer) {
            m_timer->stop();
            m_timer->deleteLater();
            m_timer = nullptr;
        }
    }

signals:
    void deviceMessage(const QMap<QString, QString>& scannedDevices, const QSet<QString>& currentIPs, QMap<QString, MV_CC_DEVICE_INFO> deviceInfoMap);

private slots:
    void checkDevices() {
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        int nRet=MV_OK;
        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE, &stDeviceList);
        if(MV_OK != nRet)
        {
            qDebug() << "Enum Devices fail!";
            return;
        }
        QSet<QString> scannedIPs;
        QMap<QString, QString> scannedDevices;
        QMap<QString, MV_CC_DEVICE_INFO> deviceInfoMap;

        if(stDeviceList.nDeviceNum > 0)
        {
            for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++)
            {
                MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
                if (!pDeviceInfo) continue;

                int nIp1 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
                int nIp2 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
                int nIp3 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
                int nIp4 = (pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

                QString deviceIP = QString("%1.%2.%3.%4").arg(nIp1).arg(nIp2).arg(nIp3).arg(nIp4);
                QString deviceName = QString::fromLocal8Bit((char*)pDeviceInfo->SpecialInfo.stGigEInfo.chModelName);

                if (deviceName.isEmpty()) {
                    deviceName = QString("设备%1").arg(i + 1);
                }
                scannedIPs.insert(deviceIP);
                scannedDevices[deviceIP] = deviceName;
                deviceInfoMap[deviceIP] = *stDeviceList.pDeviceInfo[i];
            }
        }

        if (scannedIPs != m_lastScannedIPs)
            {
                m_lastScannedIPs = scannedIPs;
                m_lastScannedDevices = scannedDevices;
                m_lastDeviceInfoMap = deviceInfoMap;

                // 发送信号
                emit deviceMessage(scannedDevices, scannedIPs, deviceInfoMap);
            }

    }

private:
    QSet<QString> m_lastScannedIPs;
    QMap<QString, QString> m_lastScannedDevices;
    QMap<QString, MV_CC_DEVICE_INFO> m_lastDeviceInfoMap;

    QTimer *m_timer = nullptr;
};
