#ifndef UIRACEDATASET_H
#define UIRACEDATASET_H

#include <QObject>

class UIRaceDataset : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal projectedProgress READ getProjectedProgress NOTIFY projectedProgressNotify)
    Q_PROPERTY(qreal groundSpeed READ getGroundSpeed WRITE setGroundSpeed NOTIFY groundSpeedNotify)
    Q_PROPERTY(bool speedSensorStatus READ getSpeedSensorStatus NOTIFY speedSensorStatusNotify)
    Q_PROPERTY(bool canStatus READ getCanStatus WRITE setCanStatus NOTIFY canStatusNotify)
    Q_PROPERTY(QString totalTime READ getTotalTime WRITE setTotalTime NOTIFY totalTimeNotify)
    Q_PROPERTY(QString currentLapTime READ getCurrentLapTime WRITE setCurrentLapTime NOTIFY currentLapTimeNotify)
public:
    explicit UIRaceDataset(QObject *parent = 0);

    void setProjectedProgress(double projectedProgress);
    qreal getProjectedProgress();

    void setGroundSpeed(qreal speed);
    qreal getGroundSpeed();

    void setSpeedSensorStatus(bool status);
    bool getSpeedSensorStatus();

    void setCanStatus(bool status);
    bool getCanStatus();

    void setTotalTime(QString time);
    QString getTotalTime();

    void setCurrentLapTime(QString time);
    QString getCurrentLapTime();

private:
    qreal projectedProgress;
    qreal groundSpeed;
    bool speedSensorStatus;
    bool canStatus;
    QString totalTime;
    QString currentLapTime;

signals:
    void projectedProgressNotify();
    void groundSpeedNotify();
    void speedSensorStatusNotify();
    void canStatusNotify();
    void totalTimeNotify();
    void currentLapTimeNotify();
public slots:
};

#endif // UIRACEDATASET_H
