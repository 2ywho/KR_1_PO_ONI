#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QTimer>
#include <cmath>
#include <QDateTime>
#include <QSoundEffect>
#include <QUrl>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QSlider>
#include <QCheckBox>
#include <algorithm>

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
    auto *baseLabel = scene->addText("База");
    baseLabel->setPos(base + QPointF(12, -12));
    int pointNumber = 1;
    for (const QPointF &point : waypoints) {
        scene->addEllipse(point.x() - 4, point.y() - 4, 8, 8,
                          QPen(Qt::black), QBrush(Qt::black));
        auto *label = scene->addText(QString::number(pointNumber++));
        label->setPos(point + QPointF(8, -12));
    }

    // Локальный центр круга совпадает с координатами БПЛА.
    uav = scene->addEllipse(-10, -10, 20, 20,
                            QPen(Qt::green), QBrush(Qt::green));
    rtlLine = scene->addLine(0, 0, 0, 0,
                             QPen(Qt::yellow, 1, Qt::DashLine));

    missionTrailItem = scene->addPath(QPainterPath(), QPen(Qt::darkGreen, 2));
    rtlTrailItem = scene->addPath(QPainterPath(), QPen(Qt::darkYellow, 2));
    missionTrailItem->setZValue(-1);
    rtlTrailItem->setZValue(-1);
    chartScene = new QGraphicsScene(0, 0, 600, 150, this);
    ui->graphicsViewChart->setScene(chartScene);
    chartScene->addLine(35, 10, 35, 125, QPen(Qt::gray));
    chartScene->addLine(35, 125, 590, 125, QPen(Qt::gray));
    auto *chartLabel = chartScene->addText("Расстояние до базы • последние 12 с • единицы сцены");
    chartLabel->setPos(40, 125);
    chartItem = chartScene->addPath(QPainterPath(), QPen(Qt::blue, 2));
    lostSound = new QSoundEffect(this);
    landedSound = new QSoundEffect(this);
    lostSound->setSource(QUrl("qrc:/sounds/lost.wav"));
    landedSound->setSource(QUrl("qrc:/sounds/landed.wav"));
    lostSound->setVolume(0.35);
    landedSound->setVolume(0.35);
    timer = new QTimer(this);
    timer->setInterval(50);
    connect(timer, &QTimer::timeout, this, &MainWindow::tick);
    connect(ui->btnReset, &QPushButton::clicked,
            this, &MainWindow::resetSimulation);
    connect(ui->btnStart, &QPushButton::clicked, this, [this]() {
        if (mode != Mode::IDLE) return;
        mode = Mode::MISSION;
        logEvent("Миссия начата: вылет с базы");
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
        rtlTrail = {uavPos};
        if (ui->checkSound->isChecked()) lostSound->play();
        logEvent("Потеря связи! Переход в режим RTL");
        updateUi();
    });

    connect(ui->btnPause, &QPushButton::clicked, this, [this]() {
        if (mode == Mode::IDLE || mode == Mode::LANDED) return;
        paused = !paused;
        if (paused) timer->stop(); else timer->start();
        logEvent(paused ? "Пауза" : "Продолжение симуляции");
        updateUi();
    });
    connect(ui->sliderSpeed, &QSlider::valueChanged, this, [this]() {
        ui->labelSpeed->setText(QString("Скорость: %1 ед./с").arg(ui->sliderSpeed->value()));
    });
    connect(ui->checkSound, &QCheckBox::toggled, this, [this](bool enabled) {
        if (!enabled) { lostSound->stop(); landedSound->stop(); }
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
    paused = false;
    landingTicks = 0;
    elapsed = 0;
    lostSound->stop();
    landedSound->stop();
    missionTrail = {base};
    rtlTrail.clear();
    distances.clear();
    missionTrailItem->setPath(QPainterPath());
    rtlTrailItem->setPath(QPainterPath());
    uav->setScale(1.0);
    ui->sliderSpeed->setValue(80);
    linkActive = true;
    wpIndex = 0;
    uavPos = base;
    uav->setPos(uavPos);
    uav->setBrush(Qt::green);
    uav->setPen(QPen(Qt::green));
    rtlLine->setVisible(false);
    ui->textLog->clear();
    updateUi();
    updateTelemetry();
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
    case Mode::LANDING:
        ui->labelStatus->setText("Статус: ПОСАДКА — СНИЖЕНИЕ");
        break;
    case Mode::LANDED:
        ui->labelStatus->setText("Статус: ПОСАДКА");
        break;
    }
    if (paused) ui->labelStatus->setText(ui->labelStatus->text() + " (ПАУЗА)");
    ui->btnPause->setEnabled(mode != Mode::IDLE && mode != Mode::LANDED);
    ui->btnPause->setText(paused ? "Продолжить" : "Пауза");
    ui->labelSpeed->setText(QString("Скорость: %1 ед./с").arg(ui->sliderSpeed->value()));
    ui->labelLink->setText(linkActive ? "Связь: Есть" : "Связь: Нет");
    ui->btnStart->setEnabled(mode == Mode::IDLE);
    ui->btnLostLink->setEnabled(mode == Mode::MISSION);
}

void MainWindow::tick()
{
    if (paused || mode == Mode::IDLE || mode == Mode::LANDED) return;
    elapsed += 0.05;
    if (mode == Mode::LANDING) {
        uav->setScale(1.0 - 0.025 * ++landingTicks);
        if (landingTicks >= 20) {
            mode = Mode::LANDED;
            timer->stop();
            logEvent("Посадка завершена");
            if (ui->checkSound->isChecked()) landedSound->play();
        }
        updateUi();
        updateTelemetry();
        return;
    }

    const QPointF target = mode == Mode::MISSION ? waypoints[wpIndex] : base;
    const QPointF dir = target - uavPos;
    const double dist = std::hypot(dir.x(), dir.y());
    const double step = ui->sliderSpeed->value() * 0.05;

    if (dist > step) {
        uavPos += dir / dist * step;
    } else {
        // Точно достигаем точки, не перескакивая через неё.
        uavPos = target;
        if (mode == Mode::MISSION) {
            logEvent(QString("Достигнута точка %1").arg(wpIndex + 1));
            wpIndex = (wpIndex + 1) % waypoints.size();
        } else {
            mode = Mode::LANDING;
            landingTicks = 0;
            uav->setBrush(Qt::red);
            uav->setPen(QPen(Qt::red));
            logEvent("На базе. Начало посадки");
            updateUi();
        }
    }
    uav->setPos(uavPos);
    updateTelemetry();
}

void MainWindow::logEvent(const QString &message)
{
    ui->textLog->append(QDateTime::currentDateTime().toString("[HH:mm:ss] ") + message);
}

void MainWindow::updateTelemetry()
{
    const QPointF delta = uavPos - base;
    const double distance = std::hypot(delta.x(), delta.y());
    ui->labelDistance->setText(QString("До базы: %1 ед.").arg(distance, 0, 'f', 1));
    ui->labelTime->setText(QString("Время симуляции: %1 с").arg(elapsed, 0, 'f', 1));
    if (mode == Mode::MISSION || mode == Mode::RTL) {
        auto &trail = mode == Mode::MISSION ? missionTrail : rtlTrail;
        if (trail.isEmpty() || trail.last() != uavPos) trail.append(uavPos);
        if (trail.size() > 2000) trail.removeFirst();
        QPainterPath path;
        if (!trail.isEmpty()) path.moveTo(trail.first());
        for (int i = 1; i < trail.size(); ++i) path.lineTo(trail[i]);
        (mode == Mode::MISSION ? missionTrailItem : rtlTrailItem)->setPath(path);
    }
    distances.append(distance);
    if (distances.size() > 240) distances.removeFirst();
    const double maximum = std::max(1.0, *std::max_element(distances.begin(), distances.end()));
    QPainterPath chart;
    for (int i = 0; i < distances.size(); ++i) {
        const QPointF point(35 + i * 555.0 / 239, 125 - distances[i] / maximum * 110);
        if (i == 0) chart.moveTo(point); else chart.lineTo(point);
    }
    chartItem->setPath(chart);
}
