#include "task.h"

Task::Task(QObject *parent): QObject(parent)
{
    threads_running=0;
    items_succeeded=0;
}

Task::~Task()
{
    for(int i = 0; i<this->system_accounts.count();++i) {
        accounts.at(i)->disconnect();
    }

    for(int i = 0; i<this->accounts.count();++i) {
        accounts.at(i)->disconnect();
        delete accounts.at(i);
    }
}

bool Task::validate()
{
   /* qDebug()<<this->id;
    qDebug()<<this->user_id;
    qDebug()<<this->type;
    qDebug()<<this->threads;

    qDebug()<<this->user_accounts.count();
    qDebug()<<this->user_proxies.count();
    qDebug()<<this->spam_list.count();*/

    if(this->spam_list.count()==0) {
        qDebug()<<"некому слать";
        return false;
    }
    //if(this->user_accounts.count()==0)  return false;
    if(this->user_template.data.count()<1) {
        qDebug()<<"нечего слать";
        return false;
    }


    return true;
}



bool Task::startThread()
{

    if(this->state!=Task::PENDING) return false;

    if(threads_running>=threads) return false;

    //threads_running+=1;



    //qDebug()<<"not empty";
    for(int i = 0; i<accounts.count(); ++i) {
        if(accounts.at(i)->state==Account::STATE_BUSY) continue;
        if(accounts.at(i)->state==Account::STATE_COOLDOWN && accounts.at(i)->canPost())  {

            accounts.at(i)->unpause();
            attachSignals(accounts.at(i));
            return true;
        }
    }

    //если ничего не нашли запускаем новый аккаунт
    if(this->user_accounts.isEmpty()) {
        qDebug()<<"no user accs";
        //
        /*if(!system_accounts.isEmpty()) {
            for(int i = 0; i<system_accounts.count(); ++i) {
                if(system_accounts.at(i)->state==Account::STATE_BUSY) continue;
                if(system_accounts.at(i)->state==Account::STATE_COOLDOWN && system_accounts.at(i)->canPost())  {
                    qDebug()<<"цепляем системный аккаунт";
                    //!!!!
                    //system_accounts.at(i)->unpause();
                    attachSignals(system_accounts.at(i));
                    return true;
                }
            }
        }*/
        //--threads_running;
        emit accountsFinished(this);
        qDebug()<<"accounts finished";
        return false;
    } else {
        qDebug()<<"start thread";
        Account *a = new Account();
        a->type=this->type;

        this->accounts.append(a);
        attachSignals(a);
        a->setLoginPassword(this->user_accounts.takeFirst());

        /* раскомментировать при работе с прокси
         * это рабочий кусок
         *
        QNetworkProxy p;
        p=getProxy();
        if(p==QNetworkProxy::NoProxy) {
            //finish();
            a->state=Account::WAITING_PROXY;
            a->isLoggedIn=false;
            qDebug()<<"запрашиваем прокси";
            emit needProxy(a, this);
            return false;
        }
        a->setProxy(p);*/

        //login
        a->makeRequest();

        return true;
    }

qDebug()<<"empty";
return false;
}

void Task::setPrefix()
{
    switch(this->type) {
        case PM:
        {
            prefix="id";
            break;
        }

        case NOTIFICATION:
        {
            prefix="id";
            break;
        }

        case PROFILE:
        {
            prefix="id";
            break;
        }

        case GROUP:
        {
            prefix="club";
            break;
        }

        case VIDEO:
        {
            prefix="video";
            break;
        }

        case DISCUSSIONS:
        {
            prefix="topic";
            break;
        }
    }
}

void Task::attachSignals(Account *a)
{
    connect(a,SIGNAL(readyToSend(Account*)),this,SLOT(makeTick(Account*)));
    connect(a,SIGNAL(blocked(Account*)),this,SLOT(accountBlocked(Account*)));
    connect(a,SIGNAL(badPswd(Account*)),this,SLOT(accountBadpswd(Account*)));
    connect(a,SIGNAL(cantSend(Account*)),this, SLOT(cantSend(Account*)));
    connect(a,SIGNAL(cantSend(Account*,bool)),this, SLOT(cantSend(Account*,bool)));
    connect(a,SIGNAL(cooldownStart(Account*)),this,SLOT(accountCooldown(Account*)));
    connect(a,SIGNAL(networkError(Account*)),this,SLOT(accountNetworkError(Account*)));
}

bool Task::start()
{
    if(this->state==Task::PENDING) return false;

    this->state=Task::PENDING;
    while(threads_running<threads) {
        qDebug()<<"start?"<<threads_running<<"--"<<threads;
        if(startThread()) {
            threads_running+=1;
        } else {
            qDebug()<<"не удалось запустить поток пользовательскими данными. Ждем ответ ядра";
            break;
        }
    }

    if(threads_running!=0) emit saveTaskState(this);
}

bool Task::pause()
{
    this->state=Task::PAUSED;
    emit saveTaskState(this);
}

bool Task::stop()
{
    this->state=Task::CANCELED;
    //emit saveTaskState(this);
    emit taskFinished(this);
}



Account &Task::getAccount()
{
    /*if(this->user_accounts.isEmpty()) {

    }
    //берем аккаунт
    if(this->accounts.isEmpty()) {
        Account *a = new Account();

        a->setLoginPassword();
        this->accounts.append(a);
    } else {

    }*/
}


QNetworkProxy Task::getProxy()
{
    if(this->user_proxies.isEmpty()) {
        qDebug()<<"no user proxies";
        return QNetworkProxy::NoProxy;
    } else {

        int uses_limit=0;
        bool has_alive=false;
        do {

            for(int i = 0; i<user_proxies.count();++i) {
                qDebug()<<"check"<<user_proxies[i].proxy;
                if(user_proxies[i].isValid==true && user_proxies[i].proxy!=QNetworkProxy::NoProxy) {

                    has_alive=true;
                    if(user_proxies[i].uses<=uses_limit) {
                        user_proxies[i].uses+=1;

                        return user_proxies[i].proxy;
                    }
                }
            }

            ++uses_limit;
        } while(has_alive);
    }

    return QNetworkProxy::NoProxy;
}

void Task::toQueue(Account *a)
{
    int time;
    time = this->randInt(this->interval_from, this->interval_to);

    queue.enqueue(a);
    QTimer::singleShot(time*1000, this, SLOT(makeTick()));
    qDebug()<<"to q wait "<<time;

}

void Task::accountBlocked(Account *a)
{
    while(1) {
        if(!lock) {
            lock=true;
            for(int i = 0; i<this->accounts.count();++i) {
                if(a->login==accounts.at(i)->login) {
                    user_accounts_banned.append(a->login);
                    a->disconnect();

                    emit writeLog(this->id,1,"Аккаунт заблокирован: "+a->login);

                    delete accounts.at(i);
                    accounts.remove(i);
                    --threads_running;
                    startThread();
                }
            }
            lock=false;


            return;
        }
        sleep(50);
    }

}

void Task::accountBadpswd(Account *a)
{
    while(1) {
        if(!lock) {
            lock=true;
            for(int i = 0; i<this->accounts.count();++i) {
                if(a->login==accounts.at(i)->login) {
                    user_accounts_badpswd.append(a->login);
                    a->disconnect();

                    emit writeLog(this->id,1,"Аккаунт неправильный пароль: "+a->login);

                    delete accounts.at(i);
                    accounts.remove(i);
                    --threads_running;
                    startThread();
                    break;
                }
            }
            lock=false;
            return;
        }
        sleep(50);
    }

}

void Task::accountCooldown(Account *a)
{
    for(int i = 0; i<this->accounts.count();++i) {
        if(a->login==accounts.at(i)->login) {
            a->disconnect();

            --threads_running;
            startThread();
        }
    }
}

void Task::accountNetworkError(Account *a)
{
    a->isLoggedIn=false;
    emit needProxy(a, this);
    emit writeLog(this->id,2,"прокси не доступен: "+a->getProxy().hostName());
}

void Task::cantSend(Account *a, bool ok)
{
    qDebug()<<"smth wrong at "<<a->last_send_to<<" with "<<ok;
    spam_bad.append(a->last_send_to);
    --items_succeeded;

    emit writeLog(this->id,0,"Не отправить сообщение для: "+a->last_send_to);
    makeTick(a);
}

void Task::cantSend(Account *a)
{
    qDebug()<<"smth wrong at "<<a->last_send_to;
    spam_bad.append(a->last_send_to);
    --items_succeeded;

    emit writeLog(this->id,0,"Не отправить сообщение для: "+a->last_send_to);
    makeTick(a);
}

void Task::makeTick(Account *a)
{
    if(this->state!=Task::PENDING) {
        qDebug()<<"проект стоит";
        return;
    }

    a->setMiniCooldown(this->randInt(this->interval_from, this->interval_to));

    QString id;

    switch(this->type) {
        case Task::PM: {            
            //
            qDebug()<<"Делаем отправку ЛС |"<<spam_list.count();

            bool isNum=false;


            //if(spam_list.isEmpty()) {
            if(items_succeeded>=item_amount || items_worked>=spam_list.count()){
                finish();
                qDebug()<<"СПИСОК ЗАКОНЧИЛСЯ";
                return;
            }
            id = spam_list.takeFirst();
            qDebug()<<"отправка на "<<id;
            ++items_succeeded;
            ++items_worked;
            a->v_message=this->getMessage();
            a->send_to_mobile_only=this->send_to_mobile_only;
            a->send_to_online_only=this->send_to_online_only;
            a->sendPM(id);
        }
        break;
        case Task::GROUP:
        case Task::DISCUSSIONS:
        case Task::VIDEO:
        {
            qDebug()<<"Делаем отправку в группу";



            if(items_succeeded>=item_amount || items_worked>=spam_list.count()){
                finish();
                qDebug()<<"СПИСОК ЗАКОНЧИЛСЯ";
                return;
            }

            id = spam_list.takeFirst();

            qDebug()<<"отправка на "<<id;

            emit writeLog(this->id,0,"Отправляем по адресу: vk.com/"+id);

            ++items_succeeded;
            ++items_worked;
            a->v_message=this->getMessage();
            if(!a->sendGW(id)) {
                qDebug()<<"не получается начать отправку";
                emit writeLog(this->id,0,"Не удалось отправить с аккаунта: "+a->login);
            }
        }
        break;
        case Task::PROFILE: {
            qDebug()<<"Делаем отправку в ?";

            if(items_succeeded>=item_amount || items_worked>=spam_list.count()){
                finish();
                qDebug()<<"СПИСОК ЗАКОНЧИЛСЯ";
                return;
            }

            id = spam_list.takeFirst();

            qDebug()<<"отправка на "<<id;
            ++items_succeeded;
            ++items_worked;
            a->v_message=this->getMessage();
            a->sendProfile(id);
        }
        break;
        case Task::NOTIFICATION: {


            if(items_succeeded>=item_amount || items_worked>=spam_list.count()){
                finish();
                qDebug()<<"СПИСОК ЗАКОНЧИЛСЯ";
                return;
            }


            id = spam_list.takeFirst();
            qDebug()<<"Делаем уведомление для"<<id;

            ++items_succeeded;
            ++items_worked;
            a->v_message=this->getMessage();
            a->sendNotification(id,a->v_message.landing);
        }
        break;

    }

    spam_list.append(id);
}

bool Task::setUserProxies(UData &aProxies)
{
    QString x = aProxies.data;

    QStringList list = x.split("\n");


    if(list.isEmpty()) return false;
    for(int i = 0; i<list.count(); ++i) {
        QStringList s_list = list.at(i).trimmed().split(":");
        qDebug()<<"p:"<<s_list;
        if(s_list.count()<2) continue;
        AProxy proxy;
        proxy.uses=0;
        proxy.proxy.setHostName(s_list.at(0));
        proxy.proxy.setPort(s_list.at(1).toInt());
        if(s_list.count()>2) {
            proxy.proxy.setUser(s_list.at(2));
            proxy.proxy.setPassword(s_list.at(3));
        }

        proxy.isValid=true;

        this->user_proxies.append(proxy);
        s_list.clear();
    }
    qDebug()<<"добавлено проксей"<<user_proxies.count();
    return this->user_proxies.isEmpty();
}

bool Task::setUserAccounts(UData &aAccounts)
{
    QString x = aAccounts.data;
    QStringList list = x.split("\n");
    if(list.isEmpty()) return false;
    for(int i = 0; i<list.count(); ++i) {
        QStringList s_list = list.at(i).split(":");
        if(s_list.count()<2) continue;
        VKAccount account;
        account.login=s_list.at(0);
        account.password=s_list.at(1);
        this->user_accounts.append(account);
    }
    qDebug()<<user_accounts.count();
    return this->user_accounts.isEmpty();
}

bool Task::setSpamList(UData &aSpamList)
{
    QString x = aSpamList.data;
    QStringList t = x.split("\n");
    QStringList tt = x.split("\r");
    if(t.length()<tt.length()) {
        t=tt;
    }
    for(int i = 0; i<t.count(); ++i ) {
        if(t.at(i).length()==0) continue;
        QString xx = t.at(i).trimmed();
        if(xx.at(0).isNumber()) {
            xx=prefix+xx;
        }

        this->spam_list.append(xx.trimmed());
    }

    spam_list_id=aSpamList.id;

    return this->spam_list.isEmpty();
}

VMessage Task::getMessage()
{
    //должен быть рандомизатор
    QStringList v = user_template.attach.split("\n");
    VMessage m;
    for(int i = 0; i<v.count();++i)
    {
       //int xx = randInt(1,10);
       //if(xx>5) continue; //рандомный элемент
       QString t;
       if(v.at(i).mid(0,5)=="video") {
           t = v.at(i);
           t = t.remove(0,5);
           m.videos.append(t);
       }
       else if(v.at(i).mid(0,5)=="photo") {
           t = v.at(i);
           t = t.remove(0,5);
           m.photos.append(t);
       } else {
           m.photos.append(v.at(i));
       }
    }

    //RANDOM TEXT

    QString str = user_template.data;

    if(this->type==Task::NOTIFICATION) {
        if(user_template.topic.isEmpty()) {
            qDebug()<<"ERROR IN TEMPLATE";
            return m;
        } else {
            m.landing=user_template.topic;
        }

    }

/*
    QRegExp re("\\{(.+)\\}");
    re.setMinimal(true);
    //QRegExp re("(?:\\{(?:\\\\\"|[^\\}])*\\})");
    int pos = 0;
    while ((pos = re.indexIn(str, pos)) != -1) {
        QString t = re.cap(0).mid(1,re.cap(0).length()-2);
        QStringList tt = t.split("|");
        t = tt.at(randInt(0,tt.length()-1));
        str.replace(pos, re.cap(0).length(), t);
        pos += t.length();
    }
    //

    m.message=str;
*/
    str = BadWords::parseRnd(str);
    str = BadWords::mixStr(str);

    m.message=str;
    //m.photos=v;
    return m;
}

int Task::randInt(int low, int high)
{
    // Random number between low and high
    return qrand() % ((high + 1) - low) + low;
}

void Task::finish()
{
    this->state=Task::FINISHED;
    emit(taskFinished(this));
    qDebug()<<"FINISHED";
}




