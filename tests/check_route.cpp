#include "mainwindow.h"
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <QSlider>
#include <QCheckBox>
#include <QFile>
#include <QGraphicsPathItem>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    MainWindow window;
    window.findChild<QCheckBox*>("checkSound")->setChecked(false);
    if (!QFile::exists(":/sounds/lost.wav") || !QFile::exists(":/sounds/landed.wav")) return 30;
    auto view=window.findChild<QGraphicsView*>("graphicsView");
    auto start=window.findChild<QPushButton*>("btnStart");
    auto reset=window.findChild<QPushButton*>("btnReset");
    auto log=window.findChild<QTextEdit*>("textLog");
    auto status=window.findChild<QLabel*>("labelStatus");
    auto timer=window.findChild<QTimer*>();
    QGraphicsEllipseItem *uav=nullptr;
    for (auto item:view->scene()->items()) {
        auto ellipse=qgraphicsitem_cast<QGraphicsEllipseItem*>(item);
        if (ellipse && ellipse->rect()==QRectF(-10,-10,20,20)) uav=ellipse;
    }
    if (!uav || uav->pos()!=QPointF(300,200) || timer->isActive()) return 1;
    QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
    if (uav->pos()!=QPointF(300,200)) return 2;
    start->click();
    if (!timer->isActive() || timer->interval()!=50 || start->isEnabled() || status->text()!=QString::fromUtf8("Статус: ПОЛЁТ")) return 3;
    start->click();
    if (log->toPlainText().count(QString::fromUtf8("Миссия начата"))!=1 || window.findChildren<QTimer*>().size()!=1) return 19;
    const QList<QPointF> expected={{100,100},{500,100},{500,300},{100,300},{100,100}};
    for (const auto &target:expected) {
        bool reached=false;
        for (int i=0;i<200;++i) {
            const QPointF before=uav->pos();
            QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
            const QPointF delta=uav->pos()-before;
            if (std::hypot(delta.x(),delta.y())>4.000001) return 4;
            if (!std::isfinite(uav->pos().x()) || !std::isfinite(uav->pos().y())) return 5;
            if (uav->pos()==target) { reached=true; break; }
        }
        if (!reached) return 6;
    }
    if (log->toPlainText().count(QString::fromUtf8("Достигнута точка"))!=5) return 7;
    reset->click();
    if (uav->pos()!=QPointF(300,200) || timer->isActive() || !log->toPlainText().isEmpty() || !start->isEnabled()) return 8;
    start->click();
    QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
    if (uav->pos()==QPointF(300,200)) return 9;
    auto lost=window.findChild<QPushButton*>("btnLostLink");
    auto link=window.findChild<QLabel*>("labelLink");
    QGraphicsLineItem *rtlLine=nullptr;
    for (auto item:view->scene()->items()) {
        if (auto line=qgraphicsitem_cast<QGraphicsLineItem*>(item)) rtlLine=line;
    }
    if (!rtlLine) return 10;
    for (int delay : {0,25,100,300}) {
        reset->click();
        start->click();
        for (int i=0;i<delay;++i) QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
        const auto departure=uav->pos();
        lost->click();
        start->click();
        lost->click();
        if (log->toPlainText().count(QString::fromUtf8("Потеря связи!"))!=1) return 20;
        if (uav->brush().color()!=QColor(Qt::yellow) || uav->pen().color()!=QColor(Qt::yellow) || link->text()!=QString::fromUtf8("Связь: Нет") || lost->isEnabled()) return 11;
        if (!rtlLine->isVisible() || rtlLine->line()!=QLineF(departure,QPointF(300,200)) || rtlLine->pen().style()!=Qt::DashLine) return 12;
        for (int i=0;i<250 && timer->isActive();++i) {
            const auto before=uav->pos();
            QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
            const auto delta=uav->pos()-before;
            const auto initialDir=QPointF(300,200)-before;
            const auto remainingDir=QPointF(300,200)-uav->pos();
            const double initialDistance=std::hypot(initialDir.x(),initialDir.y());
            const double remainingDistance=std::hypot(remainingDir.x(),remainingDir.y());
            if (std::abs(remainingDistance-std::max(0.0,initialDistance-4.0))>0.000001) return 21;
            if (std::hypot(delta.x(),delta.y())>4.000001) return 13;
        }
        if (timer->isActive() || uav->pos()!=QPointF(300,200) || uav->brush().color()!=QColor(Qt::red) || uav->pen().color()!=QColor(Qt::red)) return 14;
        if (status->text()!=QString::fromUtf8("Статус: ПОСАДКА") || log->toPlainText().count(QString::fromUtf8("Посадка завершена"))!=1) return 15;
        QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
        if (log->toPlainText().count(QString::fromUtf8("Посадка завершена"))!=1) return 16;
        start->click();
        lost->click();
        for (int i=0;i<10;++i) QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
        if (uav->pos()!=QPointF(300,200) || timer->isActive()) return 22;
    }
    reset->click();
    if (rtlLine->isVisible() || link->text()!=QString::fromUtf8("Связь: Есть") || uav->brush().color()!=QColor(Qt::green) || uav->pen().color()!=QColor(Qt::green)) return 17;
    start->click();
    for (int i=0;i<20;++i) QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
    lost->click();
    reset->click();
    if (timer->isActive() || rtlLine->isVisible() || uav->pos()!=QPointF(300,200)) return 18;
    reset->click();
    auto speed=window.findChild<QSlider*>("sliderSpeed");
    auto pause=window.findChild<QPushButton*>("btnPause");
    start->click();
    speed->setValue(160);
    const auto initial=uav->pos();
    QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
    const auto motion=uav->pos()-initial;
    if (std::abs(std::hypot(motion.x(),motion.y())-8.0)>0.000001) return 31;
    pause->click();
    const auto held=uav->pos();
    QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
    if (timer->isActive() || uav->pos()!=held) return 32;
    lost->click();
    if (timer->isActive()) return 33;
    pause->click();
    for(int i=0;i<250 && timer->isActive();++i) QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
    if(timer->isActive() || std::abs(uav->scale()-0.5)>0.000001) return 34;
    reset->click();
    if(uav->scale()!=1.0 || speed->value()!=80 || pause->isEnabled()) return 35;
    start->click(); lost->click();
    QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
    QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
    pause->click();
    const double heldScale=uav->scale();
    QMetaObject::invokeMethod(&window,"tick",Qt::DirectConnection);
    if(uav->scale()!=heldScale || timer->isActive()) return 36;
    reset->click();
    auto chart=window.findChild<QGraphicsView*>("graphicsViewChart");
    if(!chart || !chart->scene() || !window.findChild<QLabel*>("labelDistance")->text().contains("0.0")) return 37;
    std::cout << "PASS: extras (speed, pause, RTL while paused, landing animation, resources, telemetry); ";
    std::cout << "PASS: RTL, landing, reset during RTL; ";
    std::cout << "PASS: base departure, four waypoints, route loop, bounded step, reset, restart\n";
}



