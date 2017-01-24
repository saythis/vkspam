#ifndef TASK_H
#define TASK_H

#include <QtCore>
#include <QObject>
#include <QtNetwork>
#include <QWebView>
#include <QWebElement>
#include <QWebFrame>
#include <QWebElementCollection>
#include "account.h"
#include "helpers.h"





class Task: public QObject
{
    Q_OBJECT


public:
    //enum TASK_TYPES {PM,WALL,VIDEO};
    //enum TASK_STATE {CREATED, READY, RUNNING, FINISHED};
    //Тип задачи

    static const int PM=0; //тип - рассылка по личкам
    static const int NOTIFICATION=4; // тип - рассылка по уведомлениям
    static const int PROFILE=5; // тип - рассылка по профилям
    static const int GROUP=1; // тип - рассылка по группам
    static const int VIDEO=2; // тип - расслка по комментам видео
    static const int DISCUSSIONS=3; // рассылка по обсужденями

    //Статус задачи
    static const int CREATED=0;
    static const int READY=1;
    static const int PENDING=2;
    static const int FINISHED=3;
    static const int PAUSED_PEND=4;
    static const int PAUSED=5;
    static const int CANCELED_PEND=6;
    static const int CANCELED=7;

public:
    explicit Task(QObject *parent = 0);
    ~Task();

    int id=0; // порядковый номер из базы
    int user_id=0; //чья задача
    int state; // статус
    int type; // тип
    int threads; // сколько поток можно
    int threads_running; // сколько потоков работает сейчас

    int interval_from; // пауза от
    int interval_to;// до

    int items_succeeded=0; //успешно отработано
    int items_worked=0; //всего отработано
    int item_amount=0; // в списке

    bool send_to_mobile_only=false; // отправлять только пользователям кто с мобильного телефона
    bool send_to_online_only=false; // отправлять только пользователям кто онлайн

    bool lock = false; //



    QVector<Account*> accounts; //список объектов Accounts через которые будет слать и шлем (пользовательские)
    QVector<Account*> system_accounts; // список объектов аккаунтов вк через которые будет слать (системные)
    QVector<QNetworkProxy> proxies; //

    QVector<VKAccount> user_accounts;// список логинов и паролей аккаунтов вк через которые будет слать (системные)
    QVector<AProxy> user_proxies;// список проксей (пользовательские)

    QVector<QString> user_accounts_banned; //забаненые аккаунты
    QVector<QString> user_accounts_badpswd; // аккаунты с неверным паролем

    UData user_template; //шаблон сообщения для рассылки
    QStringList spam_list; //список id страниц(или профилей пользователей) на которые будет слаться
    QStringList spam_bad; // список не разослано
    QStringList spam_ok; // список разослано
    int spam_list_id;
private:
    QString prefix; //
public:
    QQueue<Account*> queue; //

    bool validate();
    bool start();
    bool pause();
    bool stop();
    void finish();

    Account& getAccount();
    bool setUserProxies(UData &aProxies);
    bool setUserAccounts(UData &aAccounts);
    bool setSpamList(UData &aSpamList);
    VMessage getMessage();
    QNetworkProxy getProxy();

private:
    void attachSignals(Account *a);

    int randInt(int low, int high);
    void setPrefix();

public slots:
    void makeTick(Account *a); // делаем манипуляции с аккаунтом (шлем по списку)
    void toQueue(Account *a);//ставим аккаунт в ожидание в очередь
    void accountBlocked(Account *a);//аккаунт бракованный
    void accountBadpswd(Account *a);//аккаунт бракованный
    void accountCooldown(Account *a);//аккаунт заморозился, нужно что то сделать
    void accountNetworkError(Account *a);//аккаунт отвалился
    void cantSend(Account *a, bool ok);//неудача при отправке
    void cantSend(Account *a);//неудача при отправке
    bool startThread();//запуск потока (инициализация Account)

signals:
    void proxyFailed();
    void accountFailed(Account *acc);
    void taskFinished(Task *task); //задача завершена
    void saveTaskState(Task *task);
    void saveTaskData(Task *task);
    void proxyFinished();//прокси закончились
    void accountsFinished(Task *task);//аккаунты закончились
    void writeLog(int task_id, int type, QString text);
    void needProxy(Account *a, Task *task);//нужны еще прокси

private:


};

#endif // TASK_H
