#include "RaceActionManager.h"

RaceActionManager::RaceActionManager(CANInterface *can, DataProcessor *data, Logger *log, UIRaceDataset *ui, GPSPositioningService *gps, NetworkInterface *net)
{
    dataProcessor = data;

    logger = log;

    uiInterface = ui;
    canInterface = can;
    // Until a better start/stop solution has been found,
    // we call startListening() here and at the start of races,
    // but we don't ever call stopListening().
    canConnected = canInterface->startListening();
    uiInterface->setCanStatus(canConnected);
    uiInterface->canStatusNotify();

    gpsService = gps;
    connect(gps, SIGNAL(lapIncremented()), this, SLOT(incrementCurrentLap()));

    network = net;

    currentLapTime = QTime();
    totalRaceTime = QTime();

    raceStarted = false;
    uiInterface->setRaceStatus(raceStarted);
    //For every UI change its corrisponding notify function must be called for redraw to occur.
    uiInterface->raceStatusNotify();

    raceTimer = new QTimer();
    connect(raceTimer, SIGNAL(timeout()), this, SLOT(updateCurrentTime()));
    sendToServerTimer = new QTimer();
    connect(sendToServerTimer, SIGNAL(timeout()), this, SLOT(sendInfoToServer()));
    indicatorUpdaterTimer = new QTimer();
    connect(indicatorUpdaterTimer, SIGNAL(timeout()), this, SLOT(updateIndicatorLights()));
    averageSpeedTimer = new QTimer();
    connect(averageSpeedTimer, SIGNAL(timeout()), this, SLOT(doSpeedAveraging()));
}

bool RaceActionManager::initConnections()
{
    //Canbus setup
//    if (!canConnected)
//    {
//        canConnected = canInterface->startListening();
//    }
//    uiInterface->setCanStatus(canConnected);
//    uiInterface->canStatusNotify();

    //GPS setup
    gpsService->startTracking();

    //Network setup.
    // connecToServer() does no harm if called when network is already connected
    // in fact, this call is necessary because it tells the network interface
    // that it should attempt to reconnect if it loses its connection.
    network->connectToServer(this);
    uiInterface->setNetworkStatus(network->isConnected());
    uiInterface->networkStatusNotify();

    return true; // This return value is meaningless, as of right now.
}

bool RaceActionManager::startRace()
{
    initConnections();

    //TODO: set approprate car here
    dataProcessor->setWheelCircumference(uiInterface->getCarName());

    // Should this be put into a slot that occurs when network is connected?
    /*
    QJsonObject startUp;
    startUp.insert("SharedKey", "k5t452dewa432");
    startUp.insert("CarType", "Sting");
    startUp.insert("LapNum", "1");


    if(networkConnected)
    {
        network->sendJASON(startUp);
    }
    */

    uiInterface->setCurrentLapNumber(1);
    uiInterface->setLastLapTime("00:00");

    //Set up pulses to check on things.
    indicatorUpdaterTimer->start(updateIndicatorPeriod);
    raceTimer->start(timerPeriod);
    sendToServerTimer->start(sendToServerTimerPeriod);
    averageSpeedTimer->start(callAveragingFunctionPeriod);

    //Start keeping track of time.
    totalRaceTime.start();
    currentLapTime.start();
    updateCurrentTime();

    //Show on the UI that the race has started.
    raceStarted = true;
    uiInterface->setRaceStatus(raceStarted);
    uiInterface->raceStatusNotify();

    //Initiate averaging
    dataProcessor->initiateAverageSpeed();

    logger->println((logPrefix + "Race started.").toStdString());

    return true;
}

void RaceActionManager::updateCurrentTime()
{
    int totalTimeMS = totalRaceTime.elapsed();
    int currentLapTimeMS = currentLapTime.elapsed();
    QString totalText = QString("%1:%2").arg( totalTimeMS / 60000        , 2, 10, QChar('0'))
                                        .arg((totalTimeMS % 60000) / 1000, 2, 10, QChar('0'));
    QString currentLapText = QString("%1:%2").arg( currentLapTimeMS / 60000        , 2, 10, QChar('0'))
                                             .arg((currentLapTimeMS % 60000) / 1000, 2, 10, QChar('0'));
    uiInterface->setCurrentLapTime(currentLapText);
    uiInterface->currentLapTimeNotify();
    uiInterface->setTotalTime(totalText);
    uiInterface->totalTimeNotify();
}

void RaceActionManager::doSpeedAveraging()
{
    dataProcessor->updateAverageSpeed();
}

void RaceActionManager::incrementCurrentLap()
{
    logger->println((QString)"Time to increment the lap counter!");

    uiInterface->setLastLapTime(uiInterface->getCurrentLapTime());
    uiInterface->lastLapTimeNotify();

    currentLapTime.restart();

    uiInterface->setCurrentLapNumber(uiInterface->getCurrentLapNumber() + 1);
    uiInterface->currentLapNumberNotify();
}

// Checks the status of the can and network interfaces then sets the UI
// indicator lights accordingly
void RaceActionManager::updateIndicatorLights()
{
    uiInterface->setNetworkStatus(network->isConnected());
    uiInterface->networkStatusNotify();

    uiInterface->setCanStatus(canConnected);
    uiInterface->canStatusNotify();

}

bool RaceActionManager::stopRace()
{
    // Need to re-evaluate this entire function once startRace() and initConnections() have been figured out
    if(raceStarted)
    {
        //Stop the update timer.
        raceTimer->stop();
        sendToServerTimer->stop();
        indicatorUpdaterTimer->stop();
        averageSpeedTimer->stop();

//        if (canConnected)
//        {
//            canInterface->stopListening();
//            canConnected = false;
//        }

        //Stop GPS updates.
        gpsService->stopTracking();

        //Deal with network
        // disconnect() does no harm if called when the network interface is
        // already disconnected. In fact, this call is necessary because it
        // tells the network interface to not attempt to reconnect should it
        // encounter a connection error.
        network->disconnect();

        //Notify the UI that the race has ended.
        updateIndicatorLights();
        uiInterface->setRaceStatus(false);
        uiInterface->raceStatusNotify();

        raceStarted = false;
    }
    logger->println((logPrefix + "Race stopped.").toStdString());

    return true;
}

/**
 * @brief Builds a JSON message containing information such as gps location and groundspeed
 * and sends the message to the web server
 */
void RaceActionManager::sendInfoToServer()
{
    if (network->isConnected())
    {
        QGeoCoordinate currentCoordinate = uiInterface->getGPSInfo().coordinate();

        // I'm guessing we don't actually need to know altitude
        //gpsMessage.insert("altitude", QJsonValue(currentCoordinate.altitude()));

        QJsonObject mainMessage;
        mainMessage.insert("AndroidId", "placeholder");
        mainMessage.insert("BatteryVoltage", 1.0);
        mainMessage.insert("carId", uiInterface->getCarName());
        mainMessage.insert("GroundSpeed", uiInterface->getGroundSpeed());
        mainMessage.insert("Id", 1);
        mainMessage.insert("IntakeTemperature", 30);
        mainMessage.insert("LKillSwitch", "placeholder");
        mainMessage.insert("Latitude", QJsonValue(currentCoordinate.latitude()));
        mainMessage.insert("LogTime", uiInterface->getTotalTime());
        mainMessage.insert("Longitude", QJsonValue(currentCoordinate.longitude()));
        mainMessage.insert("MKillSwitch", "placeholder");
        mainMessage.insert("RKillSwitch", "placeholder");
        mainMessage.insert("SecondaryBatteryVoltage", "placeholder");
        mainMessage.insert("WheelRpm", 30);
        mainMessage.insert("WindSpeed", uiInterface->getWindSpeed());
        network->sendJSON(mainMessage);
    }
}

RaceActionManager::~RaceActionManager()
{
    if(raceTimer != nullptr)
    {
        delete raceTimer;
    }
}
