#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTextCodec>





MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    //qDebug() << QCoreApplication::libraryPaths();
    //qDebug() << QSqlDatabase::drivers();
    a = new Account();
    a->setLoginPassword("this@@@","@@@");
    //a->apiCheck("4223456");
    //a->makeRequest();
    a->v_message.message="hello";
    //a->v_message.photos.append("");

    //exp = new exporter();


    //ai.setExporter(exp);

    //al = new AvitoList(ui->logger);
    //al = new AvitoList(this);



    /*timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(checkAds()));
    //timer->start(5000);

    proceedTimer = new QTimer(this);
    connect(proceedTimer, SIGNAL(timeout()), this, SLOT(proceedAds()));*/
    //proceedTimer->start(22333);

    core = new Core(ui->logger);



}

MainWindow::~MainWindow()
{
    delete core;
    delete ui;
}

void MainWindow::logMessage(QString &text)
{
    ui->logger->append(text);
}




void MainWindow::on_btnStartTask_clicked()
{
    //accounts.at(0)->setLoginPassword("this.say@hotmail.com","@qwerty2");
    //accounts.at(0)->makeRequest();

    checkAds();


}


void MainWindow::on_btnStopTask_clicked()
{




    QString source=this->ui->minusPlainTextEdit->toPlainText().trimmed();
    QStringList list = source.split("\n");
    for(int i = 0; i<list.size(); ++i) {

        AProxy p;

        QStringList ppl = list.at(i).trimmed().split(":");
        qDebug()<<ppl;
        p.proxy.setHostName(ppl.at(0));
        p.proxy.setPort(ppl.at(1).toInt());
        p.proxy.setUser(ppl.at(2));
        p.proxy.setPassword(ppl.at(3));

        //p.proxy.setType(QNetworkProxy::HttpProxy);
        p.proxy.setType(QNetworkProxy::Socks5Proxy);

        p.uses=0;
        p.isValid=true;

        /*switch (pl.at(4).toInt()) {
        case 1:
            p.setType(QNetworkProxy::Socks5Proxy);
            break;
        default:
            p.setType(QNetworkProxy::HttpProxy);
            break;
        }*/

        core->system_proxies.append(p);

    }

    qDebug()<<core->system_proxies.count();

    /*QString source=this->ui->minusPlainTextEdit->toPlainText();
    QStringList list = source.split("\n");
    BadWords::setMinusWords(list);*/

}

void MainWindow::checkAds()
{
    core->getTaskList();
    //core->updateTaskList();
    QTimer::singleShot(60000, this, SLOT(checkAds()));

}

void MainWindow::proceedAds()
{
    /* delay=10000;
    if(al->todo.isEmpty()) {
        QTimer::singleShot(delay, this, SLOT(proceedAds()));
        return;
    }

    AItemSimple item = al->todo.takeFirst();
    ai.proceedItem(item);

    delay=randInt(10000,20000);
    QTimer::singleShot(delay, this, SLOT(proceedAds()));*/
}

int MainWindow::randInt(int low, int high)
{
    // Random number between low and high
    return qrand() % ((high + 1) - low) + low;
}
