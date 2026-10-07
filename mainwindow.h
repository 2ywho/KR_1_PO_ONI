#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QPointF>

class QGraphicsScene;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QTimer;

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

    enum class Mode { IDLE, MISSION, RTL, LANDED };
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

    void resetSimulation();
    void updateUi();

private slots:
    void tick();
};

#endif // MAINWINDOW_H
