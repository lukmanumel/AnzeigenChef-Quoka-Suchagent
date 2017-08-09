#include "quokaagentplugin.h"
#include <QNetworkReply>

/**
 * @brief QuokaAgentPlugin::QuokaAgentPlugin
 * @param parent Parent is set by AnzeigenChef
 */
QuokaAgentPlugin::QuokaAgentPlugin(QObject *parent) { Q_UNUSED(parent); }

/**
 * @brief QuokaAgentPlugin::Search
 * @param rUrl: Input url for parsing target site
 *
 * Overwrite SearchAgentInterface Search function
 *
 * @return QList<SearchResult>: List of SearchResult
 */
QList<SearchResult> QuokaAgentPlugin::Search(const QUrl &rUrl, int rReadpages)
{
    QList<SearchResult> resultList;

    int page = 1;
    QString currentUrl = rUrl.toString();
    bool hasNext = true;
    QString hasNextUrl;
    QString lookFor = "q-ln hlisting";

    while(hasNext && page < rReadpages){

        QThread::msleep(1000);
        hasNext = false;
        QString responseString = GetHtmlSourceCode(currentUrl, QUrlQuery());

        if (responseString.contains("arr-rgt active\""))
        {
            hasNext = true;
            hasNextUrl = GetPartOfString(responseString, "arr-rgt active\"", "</li>");
            hasNextUrl = GetPartOfString(hasNextUrl, "href=\"", "\"");

            if (hasNextUrl.contains("http:") == false && hasNextUrl.contains("https:") == false)
            {
                hasNextUrl = "https://www.quoka.de" + hasNextUrl;
            }

            currentUrl = hasNextUrl;
        }

        while (responseString.contains(lookFor))
        {
            QString currentLine = GetPartOfString(responseString, lookFor, "</li>");
            QString seoUrl = "https://www.quoka.de/" + GetPartOfString(currentLine, "href=\"/", "\"");
            QString adid = GetPartOfString(currentLine, "data-qng-submit=\"", "\"");

            QString title = GetPartOfString(currentLine, "<h2", "</");
            title = GetPartOfString(title, ">", "");

            if (title.trimmed() == "")
            {
                responseString = GetPartOfString(responseString, lookFor, "");
                continue;
            }

            QString price = GetPartOfString(currentLine, "price\"", "<");
            price = GetPartOfString(price, ">", "").trimmed();
            price = price.replace(".-","");

            PriceType priceType = PriceType::Negotiating;

            /*
            if (price.contains("VB") && price.contains("€"))
            {
                priceType = PriceType::Negotiating;
            }
            else if (price.contains("€"))
            {
                priceType = PriceType::FixPrice;
            }
            else if (price.contains("VB"))
            {
                priceType = PriceType::Negotiating;
            }
            else
            {
                priceType = PriceType::GiveAway;
                price = "0";
            }
            */


            QString priceFix = price;
            QString newFixPrice = "";
            foreach (QChar c, priceFix)
            {
                if (c == "0" || c == "1" || c == "2" || c == "3" || c == "4" || c == "5" || c == "6" || c == "7" || c == "8" || c == "9" || c == "0")
                {
                    newFixPrice += c;
                }
            }
            if (newFixPrice == "")
            {
                newFixPrice = "0";
            }

            QString desc = GetPartOfString(currentLine, "description", "<");
            desc = GetPartOfString(desc,">","").trimmed();

            QString dist = GetPartOfString(currentLine, "postal-code", "<");
            dist = GetPartOfString(dist,">","").trimmed() + " " + GetPartOfString(currentLine,"<span class=\"locality\">","<").trimmed();

            QString image = GetPartOfString(currentLine, "data-src=\"", "\"");

            bool datedone = false;
            QString endDate = "";
            QString startDate = GetPartOfString(currentLine, "date\">", "</");
            startDate = startDate.replace("Uhr","").trimmed();

            if (startDate.contains("Heute"))
            {
                startDate = startDate.replace("Heute,", "").trimmed();
                QDateTime xMyDateTime = QDateTime::currentDateTime();
                startDate = xMyDateTime.toString("yyyy-MM-dd") + " " + startDate + ":00";
                datedone = true;
            }

            if (startDate.contains("Gestern"))
            {
                QDateTime xMyDateTime = QDateTime::currentDateTime().addDays(-1);
                startDate = xMyDateTime.toString("yyyy-MM-dd HH:mm:ss");
                datedone = true;
            }

            if (datedone == false)
            {
                QDateTime MyDateTime;
                try
                {
                    startDate = startDate.replace(".17",".2017");
                    startDate = startDate.replace(".18",".2018");
                    startDate = startDate.replace(".19",".2019");
                    startDate = startDate.replace(".20",".2020");
                    MyDateTime = QDateTime::fromString(startDate, "dd.MM.yyyy");
                    startDate = MyDateTime.toString("yyyy-MM-dd HH:mm:ss");
                }
                catch (std::exception ex1)
                {
                    responseString = GetPartOfString(responseString, lookFor, "");
                    qWarning() << ex1.what();
                    continue;
                }
            }

            if (startDate.trimmed() == "")
                continue;

            endDate = CalcEndTime(startDate);

            title = FixHtml(title);
            dist = FixHtml(dist);

            if ((title != "" || dist != "") && title != "{{name}}")
            {
                SearchResult newSearchResult;
                newSearchResult.AdDescription = FixHtml(desc).trimmed();
                newSearchResult.AdEnd = QDateTime::fromString(endDate,"yyyy-MM-dd HH:mm:ss");
                newSearchResult.AdId = adid.trimmed();
                newSearchResult.AdImageUrl = image.trimmed();
                newSearchResult.AdPrice = newFixPrice.toInt();
                newSearchResult.AdPriceType = priceType;
                newSearchResult.AdSeoUrl = seoUrl.trimmed();
                newSearchResult.AdStart = QDateTime::fromString(startDate,"yyyy-MM-dd HH:mm:ss");
                newSearchResult.AdTitle = FixHtml(title).trimmed();
                newSearchResult.AdDistance = FixHtml(dist).trimmed();
                resultList.append(newSearchResult);
            }

            responseString = GetPartOfString(responseString, lookFor, "");
        }
        page++;
    }

    qDebug() << "Return Resultlist with " << resultList.count() << " items";
    return resultList;
}

/**
 * @brief QuokaAgentPlugin::GetPartOfString
 * @param rSourceString String to parse
 * @param rFromString Begin of part
 * @param rToString End of part
 * @return SubString of begin and end, return empty String if not found
 */
QString QuokaAgentPlugin::GetPartOfString(const QString &rSourceString, const QString &rFromString, const QString &rToString)
{
    QString src = rSourceString;

    if (src.isEmpty())
        return "";

    int start = 0;
    if (rFromString != "")
    {
        start = src.indexOf(rFromString);
        if (start >= 0)
        {
            start += rFromString.length();
            int end = src.length();
            if (rToString != "")
            {
                end = src.indexOf(rToString, start);
                if (end < 0) return "";
            }
            return src.mid(start, end - start);
        }
        else
        {
            return "";
        }
    }
    else
    {
        int end = src.length();
        if (rToString != "")
        {
            end = src.indexOf(rToString, start + rFromString.length());
            if (end < 0) return "";
        }
        if (end - start < 0)
        {
            return "";
        }
        return src.mid(start, end - start);
    }
}

/**
 * @brief QuokaAgentPlugin::GetHtmlSourceCode
 * @param rUrl Source URL
 * @param rPost Any Postfields if needed, else use GET
 * @return HTML Source Code
 */
QString QuokaAgentPlugin::GetHtmlSourceCode(const QString &rUrl, const QUrlQuery &rPost)
{
    QNetworkAccessManager manager;

    QUrl uri(rUrl);
    QEventLoop eventLoop;

    QNetworkRequest request(uri);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US) AppleWebKit/534.20 (KHTML, like Gecko) Chrome/11.0.672.2 Safari/534.20");

    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    QNetworkReply *reply;
    if (rPost.isEmpty())
        reply = manager.get(request);
    else
        reply = manager.post(request,rPost.toString(QUrl::FullyEncoded).toUtf8());
    eventLoop.exec();

    QString responseString = "";
    auto error_type = reply->error();
    if (error_type == QNetworkReply::NoError) {
        responseString = reply->readAll();
    } else {
        pLastError = reply->errorString();
    }

    delete reply;
    manager.deleteLater();

    return responseString;
}

/**
 * @brief QuokaAgentPlugin::FixDateTime
 * @param rCustomDateString Any DateTime String
 * @return Formatted DateTime yyyy-MM-dd HH:mm:ss
 */
QString QuokaAgentPlugin::FixDateTime(const QString &rCustomDateString)
{
    if (!rCustomDateString.contains("T") && !rCustomDateString.contains("+"))
        return rCustomDateString;

    QString dateTime = rCustomDateString.left(23);
    dateTime = dateTime.replace("T"," ");
    int timezoneOffset = rCustomDateString.right(5).left(3).toInt();

    QDateTime timeConvertor = QDateTime::fromString(dateTime, "yyyy-MM-dd HH:mm:ss.zzz");

    if (timeConvertor.isValid()){
        timeConvertor.setTimeSpec(Qt::OffsetFromUTC);
        timeConvertor.setUtcOffset(timezoneOffset * 3600);
        return timeConvertor.toString("yyyy-MM-dd HH:mm:ss");
    } else {
        qWarning() << "fixDateTime throws an exception with format " << rCustomDateString << ", return today";
        return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    }
}

/**
 * @brief QuokaAgentPlugin::CalcEndTime
 * @param rFromString Calc endtime from starttime
 * @return Formatted EndTime String, yyyy-MM-dd HH:mm:ss
 */
QString QuokaAgentPlugin::CalcEndTime(const QString &rFromString)
{
    QString fixdate = FixDateTime(rFromString);

    QDateTime f = QDateTime::fromString(fixdate,"yyyy-MM-dd HH:mm:ss");

    if (f.isNull() || !f.isValid())
    {
        f = QDateTime::currentDateTime();
        qWarning() << "calcEbayEndTime throws an exception " << rFromString << " fixdate " << fixdate;
    }

    while(f < QDateTime::currentDateTime())
        f = f.addDays(30);

    return f.toString("yyyy-MM-dd HH:mm:ss");
}

/**
 * @brief QuokaAgentPlugin::FixHtml
 * @param rFromString String to clean HTML Tags
 * @return Clean String
 */
QString QuokaAgentPlugin::FixHtml(const QString &rFromString)
{
    QString returnString = rFromString;
    returnString = returnString.replace("&euro;","€");
    returnString = returnString.replace("&uuml;","ü");
    returnString = returnString.replace("&Uuml;","Ü");
    returnString = returnString.replace("&auml;","ä");
    returnString = returnString.replace("&Auml;","Ä");
    returnString = returnString.replace("&ouml;","ö");
    returnString = returnString.replace("&Ouml;","Ö");
    returnString = returnString.replace("&amp;","&");
    returnString = returnString.replace("&quot;","\"");
    returnString = returnString.replace("&nbsp;"," ");
    returnString = returnString.replace("&#x27;","'");
    returnString = returnString.replace("&#x2F;","/");
    returnString = returnString.replace("&#034;","\"");
    return returnString;
}

/**
 * @brief QuokaAgentPlugin::GetPlatformName
 * @return Platformname for UI
 */
QString QuokaAgentPlugin::GetPlatformName()
{
    return "Quoka";
}

/**
 * @brief QuokaAgentPlugin::GetPlatformHash
 * @return Unique plugin identifier, use a hash value max. 50 chars
 */
QString QuokaAgentPlugin::GetPlatformHash()
{
    return "QuokaSearchAgentPlugin";
}

/**
 * @brief QuokaAgentPlugin::GetLastError
 * @return Error message, only if an error happened
 */
QString QuokaAgentPlugin::GetLastError()
{
    return pLastError;
}

/**
 * @brief QuokaAgentPlugin::GetCustomerHelpMessage
 * @return Helptext for UI
 */
QString QuokaAgentPlugin::GetCustomerHelpMessage()
{
    QStringList infoText;
    infoText.append("Bei Quoka muss die Suche wie folgt aussehen:");
    infoText.append("https://www.quoka.de/alle-rubriken/kleinanzeigen.html?search1=<b>TEST</b>&city=<b>PLZ</b>&radius=<b>50</b>");
    infoText.append("TEST ist der Suchbegriff, PLZ die Postleitzahl, 50 der Radius");
    return infoText.join("<br>");
}


