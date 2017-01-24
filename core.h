#ifndef CORE_H
#define CORE_H

#include <QtCore>
#include <QObject>
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QMap>
#include <QtNetwork>
#include "task.h"
#include "helpers.h"



class Core : public QObject
{
    Q_OBJECT

    const QString DB_NAME = "sap_lc";
    const QString DB_HOST = "127.0.0.1";
    const QString DB_USER = "root";
    const QString DB_PSWD = "root";

public:
    explicit Core(QObject *parent = 0);
    Core(MTextEdit *log, QObject *parent=0);
    ~Core();




    QSqlDatabase db;

    bool getTaskList();
    bool updateTaskList();
    UData getUserData(int user_id, QString where_in);

public:

    QVector <AProxy> system_proxies;
    //QVector <AProxyAccount> proxies_history;
    QVector <SystemVKAccount> system_accounts_data;
    QVector <Account *> system_accounts;
    QVector <Account *> system_accounts_free;
    QVector <Account *> system_accounts_busy;

    QVector<User> users; // сколько проксей юзер уже запрашивал

private:

    QMap<int, Task*> task_list;

    MTextEdit *log;

    int findUserById(int user_id);

    void addToFile();
    void readFromFile();


    void createLogRecord(int task_id, QString text);

signals:

public slots:
    void saveTaskState(Task * task);
    void saveTaskData(Task * task);
    void taskFinished(Task *task);
    void accountsFinished(Task *task);
    void writeLog(int task_id, int type, QString text);
    void attachProxyAndRun(Account *a, Task *task);

};

#endif // CORE_H
