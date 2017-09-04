#include <QCanBus>
#include <QTextCodec>
#include <QDebug>
#include <QCanBusFrame>
#include <QObject>
#include <QProcess>
#include "DataProcessor.h"

#ifdef Q_OS_WIN
#include "LinuxCANTypes.h"
#include <windows.h>
#endif


#ifndef CANINTERFACE_H
#define CANINTERFACE_H


class CANInterface : public QObject
{
    Q_OBJECT
public:
    explicit CANInterface(DataProcessor *dataProcessor);
    ~CANInterface();
    bool startListening();
    void stopListening();
    bool writeCANFrame(int ID, QByteArray payload);

private Q_SLOTS:
    void readFrame();

private:
    bool slcandActive;
    QCanBusDevice *device;
    DataProcessor *dataProcessor;
    bool activateSlcand();
    bool disableSlcand();
};

#endif // CANINTERFACE_H
