#include "helpers.h"



MTextEdit::MTextEdit(QWidget *parent): QTextEdit(parent)
{
    PlatformTypes<<"system"<<"avito";
}

MTextEdit::MTextEdit(const QString &text, QWidget *parent): QTextEdit(text,parent)
{

}

void MTextEdit::log(QString message)
{
    message=QTime::currentTime().toString()+":" + message;
    this->append(message);
}


BadWords::BadWords()
{

}

//QList<QString> CYourClass::m_strList = QList<QString>() << QString("First") << QString("Second"); //<<<<<<<<<<<<<<<
QStringList BadWords::minusWordsList = QStringList()<<"example";

bool BadWords::hasMinusWords(QString &text)
{
qDebug()<<"- test: "<<text;

    foreach(QString minusWord, minusWordsList) {
        if(text.contains(minusWord, Qt::CaseInsensitive)) {
            qDebug()<<"- test: detected";
            return true;
        }
    }

    return false;
}

void BadWords::setMinusWords(QStringList &text)
{
    minusWordsList.clear();
    minusWordsList=text;

    qDebug()<<minusWordsList;
}

QString BadWords::parseRnd(QString str)
{

    int level = 0;
    int start_index=0;
    int end_index=0;

    QString res="";

    for(int i = 0; i<str.length();++i) {
        if(str.at(i)==QChar('{')) {

            if(i>0) {
                if(str.at(i-1)==QChar('\\')) {
                    if(i<=1) continue;
                    else {
                        if(str.at(i-2)!=QChar('\\')) {
                            continue;
                        }
                    }
                }
            }

            if(level==0) {
                QString temp2=str.mid(start_index,(i-start_index));

                res+=temp2;
                start_index=i;
            }

            ++level;
        }

        if(str.at(i)==QChar('}')) {

            if(i>0) {
                if(str.at(i-1)==QChar('\\')) {
                    if(i<=1) continue;
                    else {
                        if(str.at(i-2)!=QChar('\\')) {
                            continue;
                        }
                    }
                }
            }


            --level;
            if(level==0) {
                end_index=i;
                QString tornd = str.mid((start_index+1),(end_index-start_index-1));


                QString temp=parseRnd(tornd);

                temp = mixStr(temp);
                //
                QStringList sl = temp.split("|");

                QString rnd_item = sl.at(BadWords::randInt(0,sl.length()-1));


                res+=rnd_item;

                start_index=end_index+1;
            }

        }
    }

    res+=str.mid(start_index,str.length()-end_index);

    return res;
}

QString BadWords::mixStr(QString str)
{
    int level = 0;
    int start_index=0;
    int end_index=0;

    QString res="";

    for(int i = 0; i<str.length();++i) {
        if(str.at(i)==QChar('[')) {

            if(i>0) {
                if(str.at(i-1)==QChar('\\')) {
                    if(i<=1) continue;
                    else {
                        if(str.at(i-2)!=QChar('\\')) {
                            continue;
                        }
                    }
                }
            }

            if(level==0) {
                QString temp2=str.mid(start_index,(i-start_index));

                res+=temp2;
                start_index=i;
            }

            ++level;
        }

        if(str.at(i)==QChar(']')) {

            if(i>0) {
                if(str.at(i-1)==QChar('\\')) {
                    if(i<=1) continue;
                    else {
                        if(str.at(i-2)!=QChar('\\')) {
                            continue;
                        }
                    }
                }
            }


            --level;
            if(level==0) {
                end_index=i;
                QString tornd = str.mid((start_index+1),(end_index-start_index-1));


                QString temp=parseRnd(tornd);

                QStringList sl = temp.split("|");

                QString rnd_item ;

                qsrand( QTime(0,0,0).secsTo(QTime::currentTime()) );

                for( int i = sl.count() - 1 ; i > 0 ; --i )
                {
                    int random = qrand() % sl.count();
                    QString str = sl[i];
                    sl[i] = sl[random];
                    sl[random] = str;
                }

                rnd_item = sl.join(" ");

                res+=rnd_item;

                start_index=end_index+1;
            }

        }
    }

    res+=str.mid(start_index,str.length()-end_index);

    return res;
}


