#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QPointF>
#include <QPainterPath>

class QGraphicsScene;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QTimer;
class QSoundEffect;
class QGraphicsPathItem;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    enum class Mode { IDLE, MISSION, RTL, LANDING, LANDED };
    Mode mode = Mode::IDLE;
    QList<QPointF> waypoints;
    int wpIndex = 0;
    QPointF base;
    QPointF uavPos;
    bool linkActive = true;

    QGraphicsScene *scene;
    QGraphicsEllipseItem *uav;
    QGraphicsLineItem *rtlLine;
    QTimer *timer;

    bool paused = false;
    int landingTicks = 0;
    double elapsed = 0.0;
    QList<QPointF> missionTrail, rtlTrail;
    QList<double> distances;
    QGraphicsScene *chartScene;
    QGraphicsPathItem *missionTrailItem, *rtlTrailItem, *chartItem;
    QSoundEffect *lostSound, *landedSound;
    void logEvent(const QString &message);
    void updateTelemetry();
    void resetSimulation();
    void updateUi();

private slots:
    void tick();
};

#endif // MAINWINDOW_H
