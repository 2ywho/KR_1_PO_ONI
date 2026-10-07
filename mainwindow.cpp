#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QTimer>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    scene = new QGraphicsScene(0, 0, 600, 400, this);
    ui->graphicsView->setScene(scene);

    base = QPointF(300, 200);
    waypoints = {QPointF(100, 100), QPointF(500, 100),
                 QPointF(500, 300), QPointF(100, 300)};

    // База — зелёный квадрат; четыре точки — маршрут миссии.
    scene->addRect(base.x() - 10, base.y() - 10, 20, 20,
                   QPen(Qt::green), QBrush(Qt::green));
    for (const QPointF &point : waypoints) {
        scene->addEllipse(point.x() - 4, point.y() - 4, 8, 8,
                          QPen(Qt::black), QBrush(Qt::black));
    }

    // Локальный центр круга совпадает с координатами БПЛА.
    uav = scene->addEllipse(-10, -10, 20, 20,
                            QPen(Qt::green), QBrush(Qt::green));
    rtlLine = scene->addLine(0, 0, 0, 0,
                             QPen(Qt::yellow, 1, Qt::DashLine));

    timer = new QTimer(this);
    timer->setInterval(50);
    connect(timer, &QTimer::timeout, this, &MainWindow::tick);
    connect(ui->btnReset, &QPushButton::clicked,
            this, &MainWindow::resetSimulation);
    connect(ui->btnStart, &QPushButton::clicked, this, [this]() {
        if (mode != Mode::IDLE) return;
        mode = Mode::MISSION;
        ui->textLog->append("Миссия начата: вылет с базы");
        updateUi();
        timer->start();
    });
    connect(ui->btnLostLink, &QPushButton::clicked, this, [this]() {
        if (mode != Mode::MISSION) return;
        linkActive = false;
        mode = Mode::RTL;
        uav->setBrush(Qt::yellow);
        uav->setPen(QPen(Qt::yellow));
        rtlLine->setLine(QLineF(uavPos, base));
        rtlLine->setVisible(true);
        ui->textLog->append("Потеря связи! Переход в режим RTL");
        updateUi();
    });

    resetSimulation();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resetSimulation()
{
    timer->stop();
    mode = Mode::IDLE;
    linkActive = true;
    wpIndex = 0;
    uavPos = base;
    uav->setPos(uavPos);
    uav->setBrush(Qt::green);
    uav->setPen(QPen(Qt::green));
    rtlLine->setVisible(false);
    ui->textLog->clear();
    updateUi();
}

void MainWindow::updateUi()
{
    switch (mode) {
    case Mode::IDLE:
        ui->labelStatus->setText("Статус: ОЖИДАНИЕ");
        break;
    case Mode::MISSION:
        ui->labelStatus->setText("Статус: ПОЛЁТ");
        break;
    case Mode::RTL:
        ui->labelStatus->setText("Статус: ВОЗВРАТ");
        break;
    case Mode::LANDED:
        ui->labelStatus->setText("Статус: ПОСАДКА");
        break;
    }
    ui->labelLink->setText(linkActive ? "Связь: Есть" : "Связь: Нет");
    ui->btnStart->setEnabled(mode == Mode::IDLE);
    ui->btnLostLink->setEnabled(mode == Mode::MISSION);
}

void MainWindow::tick()
{
    if (mode != Mode::MISSION && mode != Mode::RTL) return;

    const QPointF target = mode == Mode::MISSION ? waypoints[wpIndex] : base;
    const QPointF dir = target - uavPos;
    const double dist = std::hypot(dir.x(), dir.y());
    const double step = 4.0;

    if (dist > step) {
        uavPos += dir / dist * step;
    } else {
        // Точно достигаем точки, не перескакивая через неё.
        uavPos = target;
        if (mode == Mode::MISSION) {
            ui->textLog->append(QString("Достигнута точка %1").arg(wpIndex + 1));
            wpIndex = (wpIndex + 1) % waypoints.size();
        } else {
            mode = Mode::LANDED;
            uav->setBrush(Qt::red);
            uav->setPen(QPen(Qt::red));
            timer->stop();
            ui->textLog->append("Посадка завершена");
            updateUi();
        }
    }
    uav->setPos(uavPos);
}
