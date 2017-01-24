#include "core.h"
#include "task.h"

Core::Core(QObject *parent) : QObject(parent)
{
    db = QSqlDatabase::addDatabase("QMYSQL", "mydb");
    db.setHostName(Core::DB_HOST);
    db.setUserName(Core::DB_USER);
    db.setPassword(Core::DB_PSWD);
    db.setDatabaseName(Core::DB_NAME);
    bool ok = db.open();

    if(!ok) {
        QMessageBox::critical(0, QObject::tr("Database Error"),
        db.lastError().text());
    }
}

Core::Core(MTextEdit *log, QObject *parent): QObject(parent)
{
    db = QSqlDatabase::addDatabase("QMYSQL", "mydb");
    db.setHostName(Core::DB_HOST);
    db.setUserName(Core::DB_USER);
    db.setPassword(Core::DB_PSWD);
    db.setDatabaseName(Core::DB_NAME);
    bool ok = db.open();

    if(!ok) {
        QMessageBox::critical(0, QObject::tr("Database Error"),
        db.lastError().text());
    }

    this->log = log;


}

Core::~Core()
{
    for(int i = 0; i<task_list.count(); ++i) {
        //делать копию объекта
        saveTaskData(task_list[i]);
        saveTaskState(task_list[i]);
        writeLog(task_list[i]->id,0,"Аварийное завершение");
        //delete task_list[i];
    }
}

bool Core::getTaskList()
{
    QSqlQuery query(db);
    //QString query_str = "SELECT id, user_id, type, threads_num, user_accounts, user_proxy, user_template, spam_list, interval_from, interval_to, item_amount, online_status from task where state = "+QString::number(Task::READY)+" and start_at<NOW()";
    QString query_str = "SELECT id, user_id, type, threads_num, user_accounts, user_proxy, user_template, spam_list, interval_from, interval_to, item_amount, online_status,items_succeeded, state from task where state = 1 and start_at<NOW()";

    if(query.exec(query_str)) {
        if(query.size()==0) log->log("Нет новых");
        while (query.next()) {
qDebug()<<"NEW TASK: "<<query.value(0).toString();
            if(
                //query.value(4).toString().trimmed().isEmpty() ||
                //query.value(5).toString().trimmed().isEmpty() ||
                query.value(6).toString().trimmed().isEmpty() ||
                query.value(7).toString().trimmed().isEmpty()
            )
            {
                QString query_str1 = "update task set state = -1 where id = "+QString::number(query.value(0).toInt());
                QSqlQuery query1(db);
                query1.exec(query_str1);
                log->log("Проблемы с входным данными. Пропускаем");
                continue;
            }

            if(task_list.contains(query.value(0).toInt())) {


                task_list[query.value(0).toInt()]->start();
                writeLog(query.value(0).toInt(),0,"Возобновляем задачу");
                continue;
            }

            Task *task = new Task();
            task->id = query.value(0).toInt();
            task->user_id = query.value(1).toInt();
            task->type = query.value(2).toInt();
            task->threads = query.value(3).toInt();




            UData acs = this->getUserData(task->user_id,query.value(4).toString());
            task->setUserAccounts(acs);
            //использовать систмные
            if(query.value(5).toString()!="-1") {

                UData proxs = this->getUserData(task->user_id,query.value(5).toString());
                task->setUserProxies(proxs);
            }
            task->user_template = this->getUserData(task->user_id, query.value(6).toString());
            UData lists = this->getUserData(task->user_id, query.value(7).toString());            
            if(lists.id==0) log->log("Проблемная задача");

            task->setPrefix();
            task->setSpamList(lists);
            task->interval_from = query.value(8).toInt()*1000; // in msec
            task->interval_to = query.value(9).toInt()*1000; //in msec
            task->item_amount = query.value(10).toInt();
            task->items_succeeded = query.value(12).toInt();
            int online_only = query.value(11).toInt();
            if(online_only>=1) task->send_to_online_only=true;
            if(online_only>=2) task->send_to_mobile_only=true;

            task_list.insertMulti(task->id,task);

            //QString name = query.value(0).toString();
            //QString salary = query.value(1).toString();

//

            if(task->validate()) {
                qDebug()<<"ставим на исполнение";
                this->log->log("Задача добавлена");
                connect(task,SIGNAL(saveTaskState(Task*)),this,SLOT(saveTaskState(Task*)));
                connect(task,SIGNAL(saveTaskData(Task*)),this,SLOT(saveTaskData(Task*)));
                connect(task,SIGNAL(taskFinished(Task*)),this,SLOT(taskFinished(Task*)));
                connect(task,SIGNAL(accountsFinished(Task*)),this,SLOT(accountsFinished(Task*)));
                connect(task,SIGNAL(writeLog(int,int,QString)),this, SLOT(writeLog(int,int,QString)));
                connect(task,SIGNAL(needProxy(Account*, Task*)),this,SLOT(attachProxyAndRun(Account*, Task *)));

                task->start();

                createLogRecord(task->id,"Задача запущена");
            } else {
                qDebug()<<"ошибка задачи";
                this->log->log("Ошибка задачи");
                createLogRecord(task->id,"Ошибка задачи");
                delete task;
            }

          }
    } else {
        qDebug()<<"db error";
        qDebug()<<db.lastError().text();
    }
}

bool Core::updateTaskList()
{
    QSqlQuery query(db);
    QString query_str = "SELECT id, state from task where state in (4,6)";

    if(query.exec(query_str)) {
        //if(query.size()==0) log->log("Нет новых");
        while (query.next()) {
            if(!task_list.contains(query.value(0).toInt())) continue;

            if(query.value(1).toInt()==Task::PAUSED_PEND) {
                task_list[query.value(0).toInt()]->pause();
            }
            if(query.value(1).toInt()==Task::CANCELED_PEND) {
                task_list[query.value(0).toInt()]->stop();
            }
        }
    } else {
        qDebug()<<"db error";
        qDebug()<<db.lastError().text();
    }


    //
    QMap<int, Task*>::const_iterator it = task_list.constBegin();
    while (it != task_list.constEnd()) {
        if(it.value()->state==Task::PENDING)
            saveTaskData(it.value());

        ++it;
    }

}


UData Core::getUserData(int user_id, QString where_in)
{
    where_in=where_in.replace( " ", "" );
    if(where_in.length()==0) return {0};
    if(where_in[where_in.length()-1]==',') {
        where_in=where_in.remove(where_in.length()-1,1);
    }

    qDebug()<<where_in;

    // integers 1 to 9999
    QRegExp rx("([,\\d\\s]+)");
    // the validator treats the regexp as "^[1-9]\\d{0,3}$"
    QRegExpValidator v(rx, 0);

    int pos = 0;



    if(v.validate(where_in, pos) == QValidator::Invalid) {
        qDebug()<<"BAD WHERE_IN!!!";
        return {0};
    }
    //check match
    if (true) {

        QString query_str = "SELECT id, user_id, data,attach, typization, type, topic from user_data where user_id = "+QString::number(user_id)+" and id in ("+where_in+")";

        QSqlQuery query(db);


        if(query.exec(query_str)) {
            if(query.size()<=0) {
                qDebug()<<"NO DATA";
                return {0};
            }
            query.first();
            UData t;
            t.id=query.value(0).toInt();
            t.user_id=query.value(1).toInt();
            t.data=query.value(2).toString().trimmed();
            t.attach=query.value(3).toString().trimmed();
            t.typization=query.value(4).toInt();
            t.type=query.value(5).toInt();
            t.topic=query.value(6).toString().trimmed();

            return t;
        } else {

            qDebug()<<"err"<<db.lastError().text();

        }
    }

    return {0};
}

int Core::findUserById(int user_id)
{

    for(int i = 0; i<users.count();++i) {
        if(users.at(i).user_id==user_id) {
            return i;
        }
    }

    User user;
    user.user_id=user_id;
    user.accounts_available=0;
    user.accounts_used=0;
    user.proxy_available=0;
    user.proxy_used=0;
    user.membership=0;
    users.append(user);
    return (users.count()-1);
}

void Core::saveTaskState(Task *task)
{
    QString query_str = "update task set state = "+QString::number(task->state) + " where id = "+QString::number(task->id);
    QSqlQuery query(db);
    if(query.exec(query_str)) {
        log->log("Состояние задания сохранено");
        writeLog(task->id,0,"Статус изменен");
    } else {
        qDebug()<<"ошибка сохранения состояния"<<query_str;
    }
}

void Core::saveTaskData(Task *task)
{
    QString query_str = "update task set items_succeeded="+QString::number(task->items_succeeded)+" where id = "+QString::number(task->id);
    QSqlQuery query(db);
    if(query.exec(query_str)) {
        log->log("Состояние задания сохранено");
    } else {
        qDebug()<<"ошибка сохранения состояния"<<query_str;
    }


    query_str = "update user_data set data = '"+task->spam_list.join(QChar(10)) + "' where id = "+QString::number(task->spam_list_id);

    if(query.exec(query_str)) {
        log->log("Состояние данных рассылки сохранено");
    } else {
        qDebug()<<"ошибка сохранения данных "<<query_str;
    }
}

void Core::taskFinished(Task *task)
{
    //task->state=Task::FINISHED;
    saveTaskState(task);
    saveTaskData(task);
    task_list.remove(task->id);
    system_accounts_free+=task->system_accounts;
    task->disconnect();
    //delete task;
    task->deleteLater();
}

void Core::accountsFinished(Task *task)
{/*
    if(system_accounts_free.isEmpty()) {
        qDebug()<<"системных нет, ждем 60 секунд и стартуем поток еще раз";
        QTimer::singleShot(20000,task,SLOT(startThread()));
    } else {
        qDebug()<<"добавляем системый акк";
        Account *a = system_accounts_free.takeFirst();
        task->system_accounts.append(a);
        task->startThread();
    }*/
    if(task->threads_running==0) {
        writeLog(task->id,0,"Нет доступных аккаунтов. Останавливаем задачу");
        task->finish();
    }
}

void Core::readFromFile()
{
    //загружаем старое


    QFile file;
    file.setFileName(QCoreApplication::applicationDirPath()+"/data.bin");

    file.open(QIODevice::ReadOnly);
    QTextStream input(&file);
    QString line;
    while(!input.atEnd()) {
            line = input.readLine();
            QStringList l = line.split("|");
            SystemVKAccount data;
            data.account.login=l.at(0);
            data.account.password=l.at(1);
            data.proxy.proxy.setHostName(l.at(2));
            data.proxy.proxy.setPort(l.at(3).toInt());
            data.proxy.proxy.setUser(l.at(4));
            data.proxy.proxy.setPassword(l.at(5));
            system_accounts_data.append(data);
        }
    file.close();
}

void Core::writeLog(int task_id, int type, QString text)
{
    return;
    QString field;
    if(type==0) field = "send";
    if(type==1) field = "account";
    if(type==2) field = "proxy";

    text=QDateTime::currentDateTime().toString()+": "+text+"\r\n";

    QString query_str = "update task_log set "+field+" = CONCAT(COALESCE("+field+",''),'"+text + "') where task_id = "+QString::number(task_id);
    QSqlQuery query(db);
    if(query.exec(query_str)) {
        log->log("Лог сохранен");

    } else {
        qDebug()<<"ошибка сохранения лога"<<query_str;
    }

}

void Core::attachProxyAndRun(Account *a, Task *task)
{
    if(system_proxies.isEmpty()) {
        qDebug()<<"Нет системных проксей. Останавливаем?"<<task->threads_running;
        if(task->threads_running==0) {
            writeLog(task->id,2,"Нет доступных системных проксей");
            task->finish();
        }
        return;
    }

    //search in "used" list


    if(a->getProxy()!=QNetworkProxy::NoProxy) {
        for(int i = 0; i<system_proxies.count();++i) {
            if(system_proxies[i].proxy.hostName()==a->getProxy().hostName()) {
                system_proxies[i].isValid=false;
                qDebug()<<"проблема с системным прокис";
            }
        }
    } else {
        qDebug()<<"прокси не ставился";

    }

    int uses_limit=0;
    bool has_alive=false;
    do {
qDebug()<<"ищем прокси. использований"<<uses_limit;
        for(int i = 0; i<system_proxies.count();++i) {
            if(system_proxies[i].isValid==true  && system_proxies[i].proxy!=QNetworkProxy::NoProxy) {
                has_alive=true;
                if(system_proxies[i].uses<=uses_limit) {
                    system_proxies[i].uses+=1;
                    qDebug()<<"прокси подобран"<<system_proxies[i].proxy.hostName();
                    a->setProxy(system_proxies[i].proxy);
                    a->unpause();

                    task->threads_running+=1;
                    task->startThread();

                    return;
                }
            }
        }

        ++uses_limit;
    } while(has_alive);
}

void Core::createLogRecord(int task_id, QString text)
{
    text=QDateTime::currentDateTime().toString()+": "+text+"\r\n";

    QString query_str = "insert  into task_log (task_id, account) values ("+QString::number(task_id)+",'"+text+"')";
    QSqlQuery query(db);
    if(query.exec(query_str)) {
        log->log("Лог создан");
    } else {
        qDebug()<<"ошибка сохранения лога "<<query_str;
    }
}

void Core::addToFile()
{
    QFile file;
    file.setFileName(QCoreApplication::applicationDirPath()+"/data.bin");

    file.open(QIODevice::Append|QIODevice::Text);
    QTextStream out(&file);
    for(int i = 0; i<system_accounts_data.count(); ++i) {
        SystemVKAccount a = system_accounts_data.at(i);
        QString str = a.account.login+"|"+a.account.password+"|"+a.proxy.proxy.hostName()+"|"+a.proxy.proxy.port()+"|"+a.proxy.proxy.user()+"|"+a.proxy.proxy.password();
        out<<str<<endl;
    }

    file.close();
}


