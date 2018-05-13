#ifndef QUOKAAGENTPLUGIN_H
#define QUOKAAGENTPLUGIN_H

#include <QGenericPlugin>
#include <QNetworkAccessManager>
#include "searchagentinterface.h"


class QuokaAgentPlugin : public QObject, public SearchAgentInterface
{
    Q_OBJECT
    Q_INTERFACES(SearchAgentInterface)
    Q_PLUGIN_METADATA(IID "MG.AnzeigenChef.SearchAgentInterface" FILE "QuokaAgent.json")

private:
    QString userAgent = "Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US) AppleWebKit/534.20 (KHTML, like Gecko) Chrome/11.0.672.2 Safari/534.20";
    QString pLastError;
    QString GetPartOfString(const QString &sourceString, const QString &fromString, const QString &toString);
    QString GetHtmlSourceCode(const QString& url, const QUrlQuery& post);
    QString FixDateTime(const QString& customDateString);
    QString CalcEndTime(const QString& fromString);
    QString FixHtml(const QString& fromString);
    bool Login(const QString &username, const QString &password, QNetworkAccessManager *manager);
    QMap<QString, QString> GetListOfFields(QString fromString);

public:
    QuokaAgentPlugin(QObject *parent = 0);
    QList<SearchResult> Search(const QUrl& url, int readpages) override;
    QString GetPlatformName() override;
    QString GetPlatformHash() override;
    QString GetLastError() override;
    QString GetCustomerHelpMessage() override;
    QColor GetPlatformColor() override;
    QString GetPlatformLetters() override;
    bool SendQuestionToAdOwner(const QString &accountUsername, const QString &accountPassword, const QString &myName, const QString &myPhone, const QString &advertId) override;
};

#endif // QUOKAAGENTPLUGIN_H
