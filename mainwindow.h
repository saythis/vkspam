#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTCore>
//#include <QT>
#include <QMainWindow>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonObject>

#include <QFile>
#include <QTableWidget>

#include "account.h";
#include "core.h";



namespace Ui {
class MainWindow;
}



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    QVector<Account*> accounts;

    Account *a;

    Core *core;

    QTimer *timer;
    QTimer *proceedTimer;



public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    void logMessage(QString &text);
    int randInt(int low, int high);

/*signals:
    void readyToProceedItem();*/
private slots:

    void on_btnStartTask_clicked();

    void on_btnStopTask_clicked();

    void checkAds();
    void proceedAds();



private:
    Ui::MainWindow *ui;

private:


};

#endif // MAINWINDOW_H
