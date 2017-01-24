#ifndef HELPERS
#define HELPERS

//#include <QT>
#include <QTCore>
#include <QTextEdit>
#include <QWidget>
#include <QString>
#include <QtNetwork>

struct AItemSimple
{
    int id;
    QString href;
    int city;
};


struct AItem
{
    int id;
    QString title;
    QString phone;
    QString description;
    QString price;
    QString href;
    QString phone_href;
    int city;

};

struct UData
{
    int id;
    int user_id;
    QString data;
    int type;
    QString attach;
    int typization;
    QString topic;
};

struct VMessage
{
    QString message;
    QStringList photos;
    QStringList videos;
    QString landing;
};

struct AProxy
{
    int uses;
    QNetworkProxy proxy;
    bool isValid;
};

struct VKAccount
{
    QString login;
    QString password;
    //QNetworkProxy& proxy;
};

struct SystemVKAccount
{
    VKAccount account;
    AProxy proxy;
};



struct VKProfile
{
    QString id;
    bool can_pm;
    bool can_wall;
    bool is_online;
    bool is_mobile;
    QString firstname;
    QString lastname;

};

struct User
{
    int user_id;
    int membership;
    int proxy_available;
    int proxy_used;
    int accounts_available;
    int accounts_used;
    int tasks_running;
};

enum ECity {SPB=1, MSK, NOV, EKB, NN, KAZ, SAM, UFA};
enum EPlatform {System, Avito};



class MTextEdit : public QTextEdit
{
public:
    QStringList PlatformTypes;

public:
    MTextEdit(QWidget *parent = 0);
    MTextEdit(const QString &text, QWidget *parent = 0);


public:
    void log(QString message);
};


class BadWords
{
public:
    BadWords();
    static bool hasMinusWords(QString &text);
    static void setMinusWords(QStringList &text);

    static QString parseRnd(QString str);
    static QString mixStr(QString str);

    static int randInt(int low, int high)
    {
        // Random number between low and high
        return qrand() % ((high + 1) - low) + low;
    }

public:
    static QStringList minusWordsList;
};



#endif // HELPERS

