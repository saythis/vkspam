#ifndef AVITOITEM_H
#define AVITOITEM_H

//#include <Qt>
#include <QtCore>
#include <QObject>
#include <QtNetwork>
#include <QWebView>
#include <QWebElement>
#include <QWebFrame>
#include <QWebElementCollection>

#include "helpers.h"



class Account: public QObject
{ 
    Q_OBJECT

public:

    static const int LOGINPAGE=0;
    static const int BEFORE_AUTH=11;
    static const int AUTH=1;
    static const int AUTH_REDIRECT=2;
    static const int AUTH_CHECK=15;
    static const int PM_PAGE=3;//step1
    static const int PM_SEND=4;//step2
    static const int WALL_STEP1=5;//step1
    static const int WALL_STEP2=6;//step2
    static const int PM_API=7;
    static const int WALL_API=8;

    static const int GROUP_WALL=9;//step1
    static const int GROUP_WALL_STEP2=10;//step2
    //static const int VIDEO=3;
    //static const int GROUP=4;
    static const int PM_DELETE_STEP_1=20;
    static const int PM_DELETE_STEP_2=22;

    static const int CAPTCHA_LOAD=1000;
    static const int CAPTCHA_UPLOAD=1001;
    static const int CAPTCHA_CHECK=1002;

    static const int STATE_READY=0;
    static const int STATE_BUSY=1;
    static const int STATE_COOLDOWN=2;
    static const int STATE_BANNED=3;
    static const int STATE_BADPSWD=4;
    static const int WAITING_PROXY=5;

    int type;
public:

    Account(QObject *parent=0);
    ~Account();

private:
    void beforeAuth(); //открытие главной страницы вк
public:
    bool makeRequest();// запрос на автрозацию аккаунта
private:
    bool makeAuthRequest();//отправка запроса с авторизационными параметрами
public:
    bool sendPM(QString user);//отправка лс - параметр страница пользователя idХХХХХ
    bool sendGW(QString group);// отправка на стену - параметр - страница группы
    bool sendNotification(QString uid, QString post);//отправка уведомления - параметры: uid - для какого пользователя, post - от какой записи
    bool sendProfile(QString user);//отпрака на стену пользователя

    bool apiCheck(QString uid, int type);
    void setLoginPassword(QString login, QString password); // задать логин пароль аккаунта
    void setLoginPassword(VKAccount acc); // задать логин пароль аккаунта
    void setProxy(QNetworkProxy proxy); // задать прокси этому соединению
    QNetworkProxy getProxy();

    bool canPost();

    bool setMiniCooldown(int msec); //сколько пауза

    void unpause();

    QString message; //текстовое сообщение для отправки
    VMessage v_message; // сообщение с картинками и видео

    //не помню почему паблик, вообще должно быть приват
    QString login;
    QString last_send_to;

    //это норм
    int state=0;

    bool send_to_online_only = false;
    bool send_to_mobile_only = false;

public slots:
    void replyFinished(QNetworkReply *reply);
    void replyApiFinished(QNetworkReply *reply);
    void delayedReadyToSend();
    void afterApiLoaded(int);

    void captchaCkeck();


signals:
    void readyToSend(Account *acc);
    void blocked(Account *acc);
    void badPswd(Account *acc);
    void cantSend(Account *acc, bool ok);
    void cantSend(Account *acc);
    void apiLoaded(int);
    void cooldownStart(Account *acc);
    void networkError(Account *acc);



private:

    bool makeAuthRedirect(QUrl url);
    bool makeAuthCheck();
    bool sendPMStep1(QString user);
    bool proceedSendPm(QString url);
    bool sendWallStep1(QString id); //profile
    bool sendWallStep2(QString url); // profile
    bool sendGroupWallStep1(QString id);
    bool sendGroupWallStep2(QString url);

    bool captchaRequest();
    bool captchaUpload(QByteArray data);

    void open_page(QString url, int type);
    void post_data(QString url, int type, QByteArray data);




    QNetworkAccessManager *mngr;
    QNetworkAccessManager *api_mngr;
    QNetworkCookieJar *cookie;

public:
    bool isLoggedIn=false;
    bool isBanned=false;
private:

    VKProfile temp_send_to_profile;

    QString password;
    QDateTime lastaction_at;
    QDateTime login_at;
    QDateTime firstaction_at;
    int cooldown=600; //min
    int mini_cooldown=10000; //in msec
    int actions_count = 0;
    int max_actions = 50; //before cooldown



    QByteArray temp_post_data;

    QString ip_h;
    QString lg_h;

    QString captcha_action;
    QString captcha_src;
    QString captcha_sid;
    QString captcha_key;
    QString anti_captcha_id;
    QString anti_captcha_key;
    int captcha_check_attempt;
    int captcha_resume;

    int randInt(int low, int high);

};

#endif // AVITOITEM_H


