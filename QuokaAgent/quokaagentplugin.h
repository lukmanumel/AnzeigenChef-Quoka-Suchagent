#ifndef QUOKAAGENTPLUGIN_H
#define QUOKAAGENTPLUGIN_H

#include <QGenericPlugin>
#include "searchagentinterface.h"


class QuokaAgentPlugin : public QObject, public SearchAgentInterface
{
    Q_OBJECT
    Q_INTERFACES(SearchAgentInterface)
    Q_PLUGIN_METADATA(IID "MG.AnzeigenChef.SearchAgentInterface" FILE "QuokaAgent.json")

private:
    QString pLastError;
    QString GetPartOfString(const QString &rSourceString, const QString &rFromString, const QString &rToString);
    QString GetHtmlSourceCode(const QString& rUrl, const QUrlQuery& rPost);
    QString FixDateTime(const QString& rCustomDateString);
    QString CalcEndTime(const QString& rFromString);
    QString FixHtml(const QString& rFromString);

public:
    QuokaAgentPlugin(QObject *parent = 0);
    QList<SearchResult> Search(const QUrl& rUrl, int rReadpages) override;
    QString GetPlatformName() override;
    QString GetPlatformHash() override;
    QString GetLastError() override;
    QString GetCustomerHelpMessage() override;
};

#endif // QUOKAAGENTPLUGIN_H
