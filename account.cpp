#include "account.h"

Account::Account(QObject *parent): QObject(parent)
{


    mngr = new QNetworkAccessManager(this);
    api_mngr = new QNetworkAccessManager(this);

    cookie = new QNetworkCookieJar();

    mngr->setCookieJar(cookie);
    //mngr->setProxy(QNetworkProxy::NoProxy);

    actions_count = 0;

    connect(mngr,SIGNAL(finished(QNetworkReply*)),this,SLOT(replyFinished(QNetworkReply*)));
    connect(api_mngr,SIGNAL(finished(QNetworkReply*)),this,SLOT(replyApiFinished(QNetworkReply*)));

    connect(this,SIGNAL(apiLoaded(int)),this,SLOT(afterApiLoaded(int)));

    lastaction_at=QDateTime::currentDateTime();

    //anti_captcha_key="3a9fd7b690857d323d8ae725f4b80781";
    anti_captcha_key="f6eaf0c21bd8c87da20744f0adfecc2e";

}

Account::~Account()
{
    qDebug()<<"free account mem";
    disconnect(mngr,SIGNAL(finished(QNetworkReply*)),this,SLOT(replyFinished(QNetworkReply*)));
    disconnect(api_mngr,SIGNAL(finished(QNetworkReply*)),this,SLOT(replyApiFinished(QNetworkReply*)));
    this->disconnect();
    delete cookie;

    delete mngr;
    delete api_mngr;
}

void Account::setLoginPassword(QString login, QString password)
{
    this->login=login;
    this->password=password;
}

void Account::setLoginPassword(VKAccount acc)
{
    this->login=acc.login;
    this->password=acc.password;
}

void Account::setProxy(QNetworkProxy proxy)
{
    mngr->setProxy(proxy);
}

QNetworkProxy Account::getProxy()
{
    return mngr->proxy();
}

bool Account::canPost()
{
    qDebug()<<QDateTime::currentDateTime();
    qDebug()<<this->firstaction_at.addSecs(this->cooldown*60);
    if(this->isBanned) {
        qDebug()<<"забанен";
        return false;
    }
    if(this->actions_count>=this->max_actions&&QDateTime::currentDateTime()<this->firstaction_at.addSecs(this->cooldown*60)) return false;
    else if(this->actions_count>=this->max_actions) this->actions_count=0;
    return true;
}

bool Account::setMiniCooldown(int msec)
{
    this->mini_cooldown = msec;
}

void Account::unpause()
{
    this->state=Account::STATE_READY;
    if(this->isLoggedIn) {
        qDebug()<<"continue with account";
        QTimer::singleShot(this->mini_cooldown,this,SLOT(delayedReadyToSend()));
    } else {
        //log in
        qDebug()<<"continue with account NO";
        this->makeRequest();
    }
}

bool Account::sendPM(QString user)
{
    if(!this->isLoggedIn||!canPost()) return false;
    //if(!this->canPost()) return false;
    this->last_send_to=user;
    apiCheck(user, Account::PM_API);
    return true;
}

bool Account::sendProfile(QString user)
{
    if(!this->isLoggedIn||!canPost()) return false;
    //if(!this->canPost()) return false;
    this->last_send_to=user;
    apiCheck(user, Account::WALL_API);
    return true;
}

bool Account::sendGW(QString group)
{
    qDebug()<<"proxy: "<<mngr->proxy().hostName();

    if(!this->isLoggedIn||!canPost()) return false;
    //if(!this->canPost()) return false;
    this->last_send_to=group;
    this->sendGroupWallStep1(group);
    return true;
}

bool Account::sendNotification(QString uid, QString post)
{
    if(!this->isLoggedIn||!canPost()) return false;
    //if(!this->canPost()) return false;
    this->last_send_to=uid;
    this->v_message.message="@"+uid+" (Хэй) "+this->v_message.message;
    this->sendGroupWallStep1(post);
    return true;
}

void Account::afterApiLoaded(int type)
{
    switch(type)
    {
        case Account::PM_API:
        {
            if(!temp_send_to_profile.can_pm || (send_to_online_only&&!temp_send_to_profile.is_online) || (!temp_send_to_profile.is_mobile&&send_to_mobile_only)) {
                qDebug()<<!temp_send_to_profile.can_pm<<(send_to_online_only&&!temp_send_to_profile.is_online)<<(!temp_send_to_profile.is_mobile&&send_to_mobile_only);
                qDebug()<<"cant pm";
                emit cantSend(this, temp_send_to_profile.can_pm);
                //emit readyToSend(this);
            } else {

                sendPMStep1(temp_send_to_profile.id);
            }
        }
        break;
        case Account::WALL_API:
        {
            if(!temp_send_to_profile.can_wall) {
                qDebug()<<"cant wall";
                emit cantSend(this,temp_send_to_profile.can_wall);
            } else {
                sendWallStep1(temp_send_to_profile.id);
            }

        }
        break;
    }
}



bool Account::sendPMStep1(QString user)
{
    if(user.isEmpty()) {
        qDebug()<<"USER ID ERROR";
        return false;
    }
    QUrl full_url = QUrl("https://m.vk.com/write"+user);
    QNetworkRequest req(full_url);
qDebug()<<full_url.toString();
    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );
    req.setRawHeader("Host","m.vk.com");
    req.setRawHeader( "Upgrade-Insecure-Requests" , "1" );

    req.setAttribute(QNetworkRequest::User, Account::PM_PAGE);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    mngr->get(req);
}


bool Account::proceedSendPm(QString url)
{
    if(!this->isLoggedIn) return false;

    QUrl full_url = QUrl("https://m.vk.com"+url);
    QNetworkRequest req(full_url);

    req.setRawHeader(":host:" , "m.vk.com" );
    req.setRawHeader(":method:","POST");
    req.setRawHeader(":path:",url.toUtf8());
    req.setRawHeader(":scheme:","https");
    req.setRawHeader(":version:","HTTP/1.1");
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    req.setRawHeader( "user-agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "accept" , "*/*" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );    
    req.setRawHeader("origin", "https://m.vk.com");
    req.setRawHeader("referer", "https://m.vk.com");
    req.setRawHeader("x-requested-with","XMLHttpRequest");
    req.setAttribute(QNetworkRequest::User, Account::PM_SEND);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    QByteArray data;
    //_ajax=1&message={-Variable.text-}&attach1_type=photo&attach1={-Variable.photo_attach1-}&attach2_type=photo&attach2={-Variable.photo_attach2-}&attach3_type=photo&attach3={-Variable.photo_attach3-}&attach4_type=photo&attach4={-Variable.photo_attach4-}&attach5_type=photo&attach5={-Variable.photo_attach5-}&attach6_type=photo&attach6={-Variable.photo_attach6-}&attach7_type=photo&attach7={-Variable.photo_attach7-}&attach8_type=photo&attach8={-Variable.photo_attach8-}&attach9_type=photo&attach9={-Variable.photo_attach9-}&attach10_type=photo&attach10={-Variable.photo_attach10-}&attach11_type=video&attach11={-Variable.video_attach1-}

    this->v_message.message=this->v_message.message.replace("%firstname%",temp_send_to_profile.firstname);
    this->v_message.message=this->v_message.message.replace("%lastname%",temp_send_to_profile.lastname);

    data.append("_ajax=1&message="+this->v_message.message);

    for(int i = 0; i<this->v_message.photos.count(); ++i)
    {
        data.append("&attach"+QString::number((i+1))+"_type=photo&attach"+QString::number((i+1))+"="+QUrl::toPercentEncoding(this->v_message.photos.at(i)));
    }
    //qDebug()<<data;

    mngr->post(req,data);
}

bool Account::sendGroupWallStep1(QString id)
{
    QUrl full_url = QUrl("https://m.vk.com/"+id);
    QNetworkRequest req(full_url);

    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );
    req.setRawHeader("Host","m.vk.com");
    req.setRawHeader( "Upgrade-Insecure-Requests" , "1" );

    req.setAttribute(QNetworkRequest::User, Account::GROUP_WALL);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    mngr->get(req);
}

bool Account::sendGroupWallStep2(QString url)
{
    if(!this->isLoggedIn) return false;

    QUrl full_url = QUrl("https://m.vk.com"+url);
    QNetworkRequest req(full_url);

    req.setRawHeader(":host:" , "m.vk.com" );
    req.setRawHeader(":method:","POST");
    req.setRawHeader(":path:",url.toUtf8());
    req.setRawHeader(":scheme:","https");
    req.setRawHeader(":version:","HTTP/1.1");
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    req.setRawHeader( "user-agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "accept" , "*/*" );
    //!!!
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader("origin", "https://m.vk.com");
    req.setRawHeader("referer", "https://m.vk.com");
    req.setRawHeader("x-requested-with","XMLHttpRequest");
    req.setAttribute(QNetworkRequest::User, Account::GROUP_WALL_STEP2);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    QByteArray data;
    //_ajax=1&message={-Variable.text-}&attach1_type=photo&attach1={-Variable.photo_attach1-}&attach2_type=photo&attach2={-Variable.photo_attach2-}&attach3_type=photo&attach3={-Variable.photo_attach3-}&attach4_type=photo&attach4={-Variable.photo_attach4-}&attach5_type=photo&attach5={-Variable.photo_attach5-}&attach6_type=photo&attach6={-Variable.photo_attach6-}&attach7_type=photo&attach7={-Variable.photo_attach7-}&attach8_type=photo&attach8={-Variable.photo_attach8-}&attach9_type=photo&attach9={-Variable.photo_attach9-}&attach10_type=photo&attach10={-Variable.photo_attach10-}&attach11_type=video&attach11={-Variable.video_attach1-}

    data.append("message="+QUrl::toPercentEncoding(this->v_message.message));

    for(int i = 0; i<this->v_message.photos.count(); ++i)
    {
        data.append("&attach"+QString::number((i+1))+"_type=photo&attach"+QString::number((i+1))+"="+this->v_message.photos.at(i));
    }
qDebug()<<data;

    mngr->post(req,data);
}

//выкачиваем капчу
bool Account::captchaRequest()
{
    QNetworkRequest req(QUrl("http://m.vk.com"+captcha_src));


    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );

    req.setRawHeader( "Upgrade-Insecure-Requests" , "1" );


    req.setAttribute(QNetworkRequest::User, CAPTCHA_LOAD);

    mngr->get(req);
}

bool Account::captchaUpload(QByteArray captcha)
{
    QNetworkRequest req(QUrl("http://anti-captcha.com/in.php"));


    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );

    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    //req.setHeader(QNetworkRequest::ContentTypeHeader,"multipart/form-data");

    req.setRawHeader( "Upgrade-Insecure-Requests" , "1" );


    req.setAttribute(QNetworkRequest::User, CAPTCHA_UPLOAD);

    QByteArray data;
    data.append("method=base64&");
    data.append("key="+anti_captcha_key+"&");
    data.append("body="+QUrl::toPercentEncoding(captcha.toBase64()));

    mngr->post(req,data);
}


void Account::captchaCkeck()
{

    QUrl url = QUrl("http://antigate.com/res.php?key="+anti_captcha_key+"&action=get&id="+anti_captcha_id);
    qDebug()<<"capthc_check URL:"<<url;
    QNetworkRequest req(url);


    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );

    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    //req.setHeader(QNetworkRequest::ContentTypeHeader,"multipart/form-data");

    req.setRawHeader( "Upgrade-Insecure-Requests" , "1" );


    req.setAttribute(QNetworkRequest::User, CAPTCHA_CHECK);

    mngr->get(req);
}

void Account::open_page(QString url, int type)
{
    if(url.isEmpty()) {
        return;
    }
    QUrl full_url = QUrl("https://m.vk.com"+url);
    QNetworkRequest req(full_url);

    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );
    req.setRawHeader("Host","m.vk.com");
    req.setRawHeader( "Upgrade-Insecure-Requests" , "1" );

    req.setAttribute(QNetworkRequest::User, type);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    mngr->get(req);
}

void Account::post_data(QString url, int type, QByteArray data)
{
    if(!this->isLoggedIn) return;

    QUrl full_url = QUrl("https://m.vk.com"+url);

    qDebug()<<"переотправляем на:"<<full_url;
    qDebug()<<data;

    QNetworkRequest req(full_url);

    req.setRawHeader(":host:" , "m.vk.com" );
    req.setRawHeader(":method:","POST");
    req.setRawHeader(":path:",url.toUtf8());
    req.setRawHeader(":scheme:","https");
    req.setRawHeader(":version:","HTTP/1.1");
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    req.setRawHeader( "user-agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "accept" , "*/*" );
    req.setRawHeader( "Accept-Encoding" , "gzip, deflate" );
    //req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader("origin", "https://m.vk.com");
    req.setRawHeader("referer", "https://m.vk.com");
    req.setRawHeader("x-requested-with","XMLHttpRequest");
    req.setAttribute(QNetworkRequest::User, type);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);


    mngr->post(req,data);
}

bool Account::sendWallStep1(QString id)
{

    //if start with "topic"
    if(id.mid(0,5)=="topic") {
        id=id+"?offset=last";
    }

    QUrl full_url = QUrl("https://m.vk.com/"+id);

    QNetworkRequest req(full_url);

    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );
    req.setRawHeader("Host","m.vk.com");
    req.setRawHeader( "Upgrade-Insecure-Requests" , "1" );

    req.setAttribute(QNetworkRequest::User, Account::WALL_STEP1);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    mngr->get(req);
}

bool Account::sendWallStep2(QString url)
{
    if(!this->isLoggedIn) return false;

    QUrl full_url = QUrl("https://m.vk.com"+url);
    QNetworkRequest req(full_url);

    req.setRawHeader(":host:" , "m.vk.com" );
    req.setRawHeader(":method:","POST");
    req.setRawHeader(":path:",url.toUtf8());
    req.setRawHeader(":scheme:","https");
    req.setRawHeader(":version:","HTTP/1.1");
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    req.setRawHeader( "user-agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "accept" , "*/*" );
    req.setRawHeader( "Accept-Encoding" , "gzip, deflate" );
    //req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader("origin", "https://m.vk.com");
    req.setRawHeader("referer", "https://m.vk.com");
    req.setRawHeader("x-requested-with","XMLHttpRequest");
    req.setAttribute(QNetworkRequest::User, Account::WALL_STEP2);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    QByteArray data;
    //_ajax=1&message={-Variable.text-}&attach1_type=photo&attach1={-Variable.photo_attach1-}&attach2_type=photo&attach2={-Variable.photo_attach2-}&attach3_type=photo&attach3={-Variable.photo_attach3-}&attach4_type=photo&attach4={-Variable.photo_attach4-}&attach5_type=photo&attach5={-Variable.photo_attach5-}&attach6_type=photo&attach6={-Variable.photo_attach6-}&attach7_type=photo&attach7={-Variable.photo_attach7-}&attach8_type=photo&attach8={-Variable.photo_attach8-}&attach9_type=photo&attach9={-Variable.photo_attach9-}&attach10_type=photo&attach10={-Variable.photo_attach10-}&attach11_type=video&attach11={-Variable.video_attach1-}

    data.append("message="+this->v_message.message);

    for(int i = 0; i<this->v_message.photos.count(); ++i)
    {
        data.append("&attach"+QString::number((i+1))+"_type=photo&attach"+QString::number((i+1))+"="+this->v_message.photos.at(i));
    }


    mngr->post(req,data);
}

bool Account::makeRequest()//QString url, int id
{
qDebug()<<"opening..."<<this->login<<this->getProxy().hostName();
    //QUrl full_url = QUrl("https://vk.com"+url);
    QUrl full_url = QUrl("https://vk.com/");
    QNetworkRequest req(full_url);


    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );
    req.setRawHeader("Host","vk.com");
    req.setRawHeader( "Upgrade-Insecure-Requests" , "1" );
    req.setRawHeader("Referer", full_url.toString().toUtf8());


    req.setRawHeader(":host:" , "vk.com" );
    req.setRawHeader(":method:","GET");
    req.setRawHeader(":path:",full_url.toString().toUtf8());
    req.setRawHeader(":scheme:","https");
    req.setRawHeader(":version:","HTTP/1.1");

    //req.setRawHeader("Cache-Control","max-age=0");

    if(!isLoggedIn) {
        //if(isBanned) return false;

        req.setAttribute(QNetworkRequest::User, Account::LOGINPAGE);

    }



//    req.setAttribute(QNetworkRequest::User, Account::PAGE);
//    req.setAttribute(QNetworkRequest::UserMax, id);


    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    mngr->get(req);
}

bool Account::makeAuthRedirect(QUrl url)//QString url, int id
{

    //QUrl full_url = QUrl("https://vk.com"+url);
    //QUrl full_url = QUrl("https://login.vk.com/?act=login");
    QNetworkRequest req(url);


    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );
    req.setRawHeader("Host","vk.com");
    req.setRawHeader( "upgrade-Insecure-Requests" , "1" );
    req.setRawHeader("referer", "https://vk.com/login.php?&to=&s=1&m=1&email="+QUrl::toPercentEncoding(this->login));


    req.setRawHeader(":host:" , "vk.com" );
    req.setRawHeader(":method:","GET");
    req.setRawHeader(":path:",url.toString().mid(14).toUtf8());
    qDebug()<<"MID::::"<<url.toString().mid(14);
    req.setRawHeader(":scheme:","https");
    req.setRawHeader(":version:","HTTP/1.1");

    //req.setRawHeader("Cache-Control","max-age=0");

    req.setAttribute(QNetworkRequest::User, Account::AUTH_REDIRECT);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    mngr->get(req);
}

bool Account::makeAuthCheck()//QString url, int id
{
    QNetworkRequest req(QUrl("https://vk.com/feed"));


    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );
    req.setRawHeader("Host","vk.com");
    req.setRawHeader( "upgrade-Insecure-Requests" , "1" );
    //req.setRawHeader("referer", "https://vk.com/login.php?&to=&s=1&m=1&email="+QUrl::toPercentEncoding(this->login));


    req.setRawHeader(":host:" , "vk.com" );
    req.setRawHeader(":method:","GET");
    req.setRawHeader(":path:","/feed");
    req.setRawHeader(":scheme:","https");
    req.setRawHeader(":version:","HTTP/1.1");

    //req.setRawHeader("Cache-Control","max-age=0");

    req.setAttribute(QNetworkRequest::User, Account::AUTH_CHECK);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);

    mngr->get(req);
}

void Account::beforeAuth()
{

    QUrl full_url = QUrl("https://vk.com/login.php");
    QNetworkRequest req(full_url);


    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    req.setRawHeader( "user-agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "accept" , "*/*" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );

    req.setRawHeader("X-Requested-With","XMLHttpRequest");

    req.setRawHeader( "Connection" , "keep-alive" );
    req.setRawHeader("Host","vk.com");

    req.setRawHeader( "Origin" , "https://vk.com" );
    req.setRawHeader("Referer", "https://vk.com/login.php?&to=&s=1&m=1&email="+this->login.toUtf8());


    req.setAttribute(QNetworkRequest::User, Account::BEFORE_AUTH);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);



    QByteArray data;


    data.append("op=a_login_attempt&login="+this->login);
    if(!captcha_key.isEmpty()) {
        qDebug()<<"with captcha answer!";
        data.append("&captcha_sid="+captcha_sid+"&captcha_key="+captcha_key);
    }

    mngr->post(req,data);

qDebug()<<"check before login"<<data;
}

bool Account::makeAuthRequest()
{

    QUrl full_url = QUrl("https://login.vk.com/");
    QNetworkRequest req(full_url);

    req.setRawHeader(":host:" , "login.vk.com" );
    req.setRawHeader(":method:","POST");
    req.setRawHeader(":path:","/");
    req.setRawHeader(":scheme:","https");
    req.setRawHeader(":version:","HTTP/1.1");

    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );
    //req.setRawHeader("Host","vk.com");

    req.setRawHeader( "origin" , "https://vk.com" );
    req.setRawHeader("referer", "https://vk.com/login.php?act=login");
    req.setRawHeader( "upgrade-insecure-requests" , "1" );




    //req.setRawHeader("Cache-Control","max-age=0");

    req.setAttribute(QNetworkRequest::User, Account::AUTH);



//    req.setAttribute(QNetworkRequest::User, Account::PAGE);
//    req.setAttribute(QNetworkRequest::UserMax, id);

    QSslConfiguration conf = req.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(conf);



    QByteArray data;


    data.append("act=login&to=&expire=&_origin=https://vk.com&ip_h="+this->ip_h+"&lg_h="+this->lg_h+"&email="+this->login+"&pass="+QUrl::toPercentEncoding(this->password));

    mngr->post(req,data);

qDebug()<<"TRY TO LOGIN"<<data;
}

bool Account::apiCheck(QString uid, int type)
{

    QNetworkRequest req(QUrl("http://api.vk.com/method/users.get?user_ids="+uid+"&fields=can_write_private_message,online,online_mobile,can_post,wall_comments"));


    req.setRawHeader( "User-Agent" , "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36" );
    req.setRawHeader( "Accept" , "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" );
    //req.setRawHeader( "Accept-Encoding" , "gzip, deflate, sdch" );
    req.setRawHeader( "Accept-Encoding" , "identity" );
    req.setRawHeader( "Accept-Language" , "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4" );
    req.setRawHeader( "Connection" , "keep-alive" );
    req.setRawHeader("Host","api.vk.com");
    req.setRawHeader( "Upgrade-Insecure-Requests" , "1" );
    //req.setRawHeader("referer", "https://vk.com/login.php?&to=&s=1&m=1&email="+QUrl::toPercentEncoding(this->login));

    req.setAttribute(QNetworkRequest::User, type);

    api_mngr->get(req);
}


void Account::replyApiFinished(QNetworkReply *reply)
{
    QByteArray data;
    data = reply->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(data);
    QJsonObject jsonObj = jsonResponse.object();
    qDebug()<<jsonObj;

    jsonObj = jsonObj["response"].toArray().at(0).toObject();

    temp_send_to_profile.firstname=jsonObj["first_name"].toString();
    temp_send_to_profile.lastname=jsonObj["last_name"].toString();
    temp_send_to_profile.id=QString::number(jsonObj["uid"].toInt());
    temp_send_to_profile.is_online=jsonObj["online"].toInt();
    temp_send_to_profile.is_mobile=jsonObj["online_mobile"].toInt();
    temp_send_to_profile.can_pm=jsonObj["can_write_private_message"].toInt();
    temp_send_to_profile.can_wall=jsonObj["can_post"].toInt();

    int type = reply->request().attribute(QNetworkRequest::User).toInt();


    emit apiLoaded(type);

    reply->deleteLater();
}

void Account::replyFinished(QNetworkReply *reply)
{
    QByteArray data;
    data = reply->readAll();


    //qDebug() << "Headers:"<<  reply->rawHeaderPairs()<< "content:" << reply->readAll();
    //qDebug() << "Headers:"<<  reply->manager()->cookieJar()<< "content:" << reply->readAll();

    /*qDebug() <<"ErrorNo: "<< reply->error() << "for url: " << reply->url().toString();
    qDebug() << "Request failed, " << reply->errorString();
    qDebug() << "Headers:"<<  reply->rawHeaderList();//<< "content:" << data;
    qDebug()<<reply->rawHeaderPairs();
    qDebug()<<"content:" << data;*/

    /*QVariant cookieVar = reply->header(QNetworkRequest::SetCookieHeader);
    if (cookieVar.isValid()) {
        QList<QNetworkCookie> cookies = cookieVar.value<QList<QNetworkCookie> >();
        foreach (QNetworkCookie cookie, cookies) {
            if(QString(cookie.name())=="csrftoken") {
                csrf = cookie.value();
                break;
            }
        }
    }*/

    if(reply->error()!=QNetworkReply::NoError) {
        qDebug() <<"ErrorNo: "<< reply->error();
        qDebug()<<"NETWORK ERROR";
        qDebug() << "Request failed, " << reply->errorString();
        qDebug() << "Headers:"<<  reply->rawHeaderList();
        qDebug()<<"content:" << data;
        emit networkError(this);
        return;
    }

    lastaction_at=QDateTime::currentDateTime();

    //int id = reply->request().attribute(QNetworkRequest::UserMax).toInt();
    int type = reply->request().attribute(QNetworkRequest::User).toInt();

    QWebView m_view;

    switch(type){

        case Account::LOGINPAGE:
        {

            m_view.setHtml(data);

            QWebElement form = m_view.page()->mainFrame()->findFirstElement("#quick_login_form");
            QWebElement we_ip_h = form.findFirst("input[name=ip_h]");
            QWebElement we_lg_h = form.findFirst("input[name=lg_h]");
            ip_h = we_ip_h.attribute("value");
            lg_h = we_lg_h.attribute("value");
            //qDebug()<<m_view->page()->mainFrame()->toPlainText();
            //qDebug()<<we_lg_h.toO();
            //qDebug()<<we_ip_h.toPlainText();
            qDebug()<<"BEFORE AUTH";
            qDebug()<<ip_h<<"----"<<lg_h;
            //makeAuthRequest();
            beforeAuth();
            return;
            break;
        }
        case Account::BEFORE_AUTH:
        {

            QJsonDocument jsonResponse = QJsonDocument::fromJson(data);
            qDebug()<<this->login<<"before auth answer: "<<data;
            QJsonObject jsonObject = jsonResponse.object();


            QJsonValue value = jsonObject["ok"];
            qDebug()<<value;
            if(value.toVariant()=="-2") {
                captcha_sid = jsonObject["captcha_sid"].toString();
                captcha_src = "/captcha.php?sid="+captcha_sid;

                captcha_resume = Account::AUTH;
                captchaRequest();
            } else {
                qDebug()<<this->login<<"ok. try login";
                makeAuthRequest();
            }
            return;
            break;
        }
        case Account::AUTH:
        {
            /*qDebug()<<"AUTH";
            qDebug() << "Headers:"<<  reply->rawHeaderPairs();
            qDebug() << "Headers:"<<  reply->rawHeaderList();
            qDebug()<<"data:"<<data;*/
        qDebug()<<data;
            qDebug()<<"-REDIRECT?--";


            QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

            qDebug()<<possibleRedirectUrl.toString();

            QUrl url = possibleRedirectUrl.toUrl();

            QStringList url_data = url.query().split("&");
            bool login_succeed=false;
            for(int i = 0; i<url_data.count(); ++i) {


                if(url_data.at(i).startsWith("act")) {
                    QString t = url_data.at(i).split("=").at(1);

                    if(t =="block" || t == "blocked") {
                        qDebug()<<"ACC BAN";
                        this->state=STATE_BANNED;
                        emit blocked(this);
                        return;
                    }

                    if(t == "slogin") {
                        login_succeed=true;
                        break;
                    }
                }
            }

            if(login_succeed) {
                qDebug()<<"ACC LOGGED";
                //this->state=STATE_BUSY;
                makeAuthRedirect(possibleRedirectUrl.toUrl());
            } else {
                qDebug()<<"ACC BAD PSWD or CAPTCHA STOPPING!"<<this->login;
                qDebug()<<"data:"<<data;
                //makeAuthRedirect(possibleRedirectUrl.toUrl());
                emit badPswd(this);
                return;
            }


            break;
        }
        case Account::AUTH_REDIRECT:
        {
            qDebug()<<"AUTH_REDIRECT";
            QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

            qDebug()<<possibleRedirectUrl.toString();
            /*qDebug() << "Headers:"<<  reply->rawHeaderPairs();
            qDebug() << "Headers:"<<  reply->rawHeaderList();
            qDebug()<<"data:"<<data;*/
            //if all OK

            //emit readyToSend(this);

            makeAuthCheck();
            break;
        }
        case Account::AUTH_CHECK:
        {

        qDebug()<<"AUTH CHECK!!";
            QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

            qDebug()<<possibleRedirectUrl.toString();

            if(possibleRedirectUrl.isNull()) {
                this->isLoggedIn=true;
                this->state=STATE_BUSY;
//emit readyToSend(this);
                int delay = randInt(10000,20000);
                qDebug()<<"wait "<<delay<<" before next mail";
                QTimer::singleShot(delay,this,SLOT(delayedReadyToSend()));
                return;
            }
            QUrl url = possibleRedirectUrl.toUrl();

            QStringList url_data = url.query().split("&");

            for(int i = 0; i<url_data.count(); ++i) {


                if(url_data.at(i).startsWith("act")) {
                    QString t = url_data.at(i).split("=").at(1);

                    if(t =="block" || t == "blocked") {
                        qDebug()<<"ACC BAN";
                        this->state=STATE_BANNED;
                        emit blocked(this);
                        return;
                    }


                }
            }



            break;
        }
        case Account::PM_PAGE:
        {
            m_view.setHtml(data);
            QWebElement form = m_view.page()->mainFrame()->findFirstElement("#write_form");
            QString action = form.attribute("action");

            qDebug()<<"BEFORE SEND";
            //qDebug()<<data;
            //qDebug()<<form.toOuterXml();
            proceedSendPm(action);
            break;
        }
        case Account::PM_SEND:
        {
            //if ok

            QJsonDocument jsonResponse = QJsonDocument::fromJson(data);
            QJsonArray jsonArray = jsonResponse.array();
            QJsonValue value = jsonArray[2];
            qDebug()<<jsonResponse;

            qDebug()<<jsonArray;
            qDebug()<<value;
            if(value==3) {
                qDebug()<<"капча?";
                qDebug()<<jsonArray[4];

                m_view.setHtml(jsonArray[4].toString());
                QWebElement form = m_view.page()->mainFrame()->findFirstElement("form");
                captcha_action = form.attribute("action");
                QWebElement captcha = m_view.page()->mainFrame()->findFirstElement("img.captcha_img");
                captcha_src = captcha.attribute("src");
                qDebug()<<"capthca src: "<<captcha_src;
                if(captcha_src.isEmpty()) {
                    qDebug()<<"не понятная ошибка";
                    emit cantSend(this);
                    emit blocked(this);
                    return;
                }
                captchaRequest();
                //return;
            }
            if(value==0) {
                //удаляем сообщение
                qDebug()<<"удаляем сообщение";
                qDebug()<<jsonArray[3].toArray().at(0);
                m_view.setHtml(jsonArray[3].toArray().at(0).toString());

                QWebElement el = m_view.page()->mainFrame()->findFirstElement("a.mi_date");
                QString href = el.attribute("href");
                qDebug()<<href;
                open_page(href,PM_DELETE_STEP_1);
            }

            //jsonObj = jsonObj["response"].toArray().at(0).toObject();

            qDebug()<<"PM SEND";
            //qDebug()<<data;
            //emit readyToSend(this);

            break;
        }

        case Account::PM_DELETE_STEP_1:
        {
            m_view.setHtml(data);
            QWebElement el = m_view.page()->mainFrame()->findFirstElement(".mi_links a");
            QString href = el.attribute("href");
            qDebug()<<"удалем далее";
            qDebug()<<href;
            open_page(href,PM_DELETE_STEP_2);
            break;
        }

        case Account::GROUP_WALL:
        {
            m_view.setHtml(data);
            QWebElement form = m_view.page()->mainFrame()->findFirstElement(".create_post form");
            if(form.isNull()) {
                qDebug()<<"сюда нельзя написать";

                emit cantSend(this);
                //delayedReadyToSend();
                return;
            }

            QString action = form.attribute("action");

            //ДЛЯ ОВТЕТА НА КОММЕНТ
            /*QWebElementCollection pi = m_view.page()->mainFrame()->findAllElements(".wall_replies>.post_item");
            QWebElement pie = pi.last();
            QWebElement name = pie.findFirst(".pi_author");
            QWebElement post = pie.findFirst("a");
            if(post.attribute("name")=="last") {
                post=post.nextSibling();
            }

            QString post_str = post.attribute("name");
            QString name_str = name.toPlainText();
qDebug()<<pie.toOuterXml();*/
            //this->v_message.message=this->v_message.message;/*"["+post_str+"|"+name_str+"], "+*/

            /*qDebug()<<"BEFORE SEND";
            qDebug()<<data;
            qDebug()<<form.toOuterXml();*/
            sendGroupWallStep2(action);
            break;
        }
        case Account::GROUP_WALL_STEP2:
        {
            QJsonDocument jsonResponse = QJsonDocument::fromJson(data);
            QJsonArray jsonArray = jsonResponse.array();
            //qDebug()<<"after step2:"<<data;
            QJsonValue value = jsonArray[2];
            if(value==3) {

                qDebug()<<"вылезла капча?";
                qDebug()<<jsonArray;
                //qDebug()<<jsonArray[4];

                m_view.setHtml(jsonArray[4].toString());
                QWebElement form = m_view.page()->mainFrame()->findFirstElement("form");
                captcha_action = form.attribute("action");
                QWebElement captcha = m_view.page()->mainFrame()->findFirstElement("img.captcha_img");
                captcha_src = captcha.attribute("src");
                //qDebug()<<"capthca src: "<<captcha_src;
                //qDebug()<<"capthca action: "<<captcha_action;


                //запоминаем данные формы
                temp_post_data.clear();
                QWebElementCollection idata = form.findAll("input");
                foreach(QWebElement t_el, idata) {
                    if("captcha_key"==t_el.attribute("name")) continue;
                    temp_post_data.append(t_el.attribute("name")+"="+t_el.attribute("value")+"&");
                }
                captcha_resume = type;

                captchaRequest();
                return;
            }


            //emit readyToSend(this);

            break;
        }
        case Account::WALL_STEP1:
        {
            m_view.setHtml(data);
            QWebElement form = m_view.page()->mainFrame()->findFirstElement(".create_post form");
            if(form.isNull()) {
                qDebug()<<this->last_send_to<<"сюда нельзя написать";
                emit cantSend(this);
                //delayedReadyToSend();
                return;
            }

            QString action = form.attribute("action");

            /*qDebug()<<"BEFORE SEND";
            qDebug()<<data;
            qDebug()<<form.toOuterXml();*/
            sendWallStep2(action);
            break;
        }
        case Account::WALL_STEP2:
        {
            //if ok
        //проверяем ответ
            QJsonDocument jsonResponse = QJsonDocument::fromJson(data);
            QJsonArray jsonArray = jsonResponse.array();
            QJsonValue value = jsonArray[2];
            if(value==3) {
                qDebug()<<"капча?";
                //qDebug()<<jsonArray[4];

                m_view.setHtml(jsonArray[4].toString());
                QWebElement form = m_view.page()->mainFrame()->findFirstElement("form");
                captcha_action = form.attribute("action");
                QWebElement captcha = m_view.page()->mainFrame()->findFirstElement("img.captcha_img");
                captcha_src = captcha.attribute("src");
                //qDebug()<<"capthca src: "<<captcha_src;

                //запоминаем данные формы
                temp_post_data.clear();
                QWebElementCollection idata = form.findAll("input");
                foreach(QWebElement t_el, idata) {
                    if("captcha_key"==t_el.attribute("name")) continue;
                    temp_post_data.append(t_el.attribute("name")+"="+t_el.attribute("value")+"&");
                }
                captcha_resume = type;

                captchaRequest();
                return;
            }

            //qDebug()<<"SEND";
            //qDebug()<<data;
            //emit readyToSend(this);

            break;
        }

        case CAPTCHA_LOAD:
        {
            //data.toBase64();
            captcha_key="";

            captchaUpload(data);
            break;
        }


        case CAPTCHA_UPLOAD:
        {
            //data.toBase64();
            //captchaUpload(data);
            //qDebug()<<"captcha upload answer"<<data;
            anti_captcha_id=QString(data).mid(3);
            //qDebug()<<"anti_captcha_id="<<anti_captcha_id;

            captcha_check_attempt=0;
            QTimer::singleShot(10000,this,SLOT(captchaCkeck()));
            break;
        }

        case CAPTCHA_CHECK:
        {
            if(captcha_check_attempt>5) {
                qDebug()<<"не удается разгадать капчу";
                emit blocked(this);
                return;
            }
            captcha_check_attempt+=1;

            //data.toBase64();
            //captchaUpload(data);
            //qDebug()<<"captcha check answer"<<data;

            if (QString(data).indexOf("NOT_READY")!=-1)
            {
                qDebug()<<"captcha is not ready yet\n";
                QTimer::singleShot(10000,this,SLOT(captchaCkeck()));
            }
            if (QString(data).indexOf("ERROR")!=-1)
            {
                //found error
                qDebug()<<"error: "<<data;

            }
            if (QString(data).indexOf("OK")!=-1)
            {
                //captcha is ready
                QString c_res;
                c_res=QString(data).mid(3);
                captcha_key = c_res;
                qDebug()<<"captcha result: "<<data;
                temp_post_data.append("captcha_key="+c_res);
                if(captcha_resume==Account::AUTH) {
                    //makeAuthRequest();
                    beforeAuth();
                } else {
                    post_data(captcha_action,captcha_resume,temp_post_data);
                }

                break;
            }

            break;
        }
        //default:

    }

    if(type==PM_SEND || type == GROUP_WALL_STEP2 || type == Account::WALL_STEP2) {
        if(actions_count==0) {
            this->firstaction_at=QDateTime::currentDateTime();
        }
        actions_count+=1;
        qDebug()<<this->login;
qDebug()<<"actions_count="<<actions_count;
        if(actions_count>=max_actions) {
            qDebug()<<"COOLDOWN BEGIN";
            this->state=STATE_COOLDOWN;
            emit cooldownStart(this);
        } else {
            qDebug()<<"wait "<<this->mini_cooldown<<" before next mail";
            QTimer::singleShot(this->mini_cooldown,this,SLOT(delayedReadyToSend()));
        }
    }
    /*case Account::VIDEO:
        m_view->setHtml(data);

        QJsonDocument j = QJsonDocument::fromJson(data);
        QJsonObject jo = j.object();
        items[id].phone = jo.value("phone").toString();
        break;
    }*/


    //lastAction=QDateTime::currentDateTime();

    /*if(isLoggedIn==false and !csrf.isEmpty() and reply->request().attribute(QNetworkRequest::User).toInt()!=InstaModel::AUTH) {
        emit(readyToLogin());
    }*/

    //cookie=reply->manager()->cookieJar();
    //qDebug()<<cookie->allCookies();

    //emit(requestComplete(reply));

    reply->deleteLater();
}

void Account::delayedReadyToSend()
{
    emit(readyToSend(this));
}


int Account::randInt(int low, int high)
{
    // Random number between low and high
    return qrand() % ((high + 1) - low) + low;
}
