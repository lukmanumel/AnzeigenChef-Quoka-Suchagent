#include "quokaagentplugin.h"
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QColor>

typedef QPair<QByteArray, QByteArray> RawHeaderPair;

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
QList<SearchResult> QuokaAgentPlugin::Search(const QUrl &url, int readpages)
{
    QList<SearchResult> resultList;

    int page = 1;
    QString currentUrl = url.toString();
    bool hasNext = true;
    QString hasNextUrl;
    QString lookFor = "q-ln hlisting";

    while(hasNext && page < readpages){

        if (qApp->property("AppDown").toInt() == 1)
            return resultList;

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
            if (qApp->property("AppDown").toInt() == 1)
                return resultList;

            QString currentLine = GetPartOfString(responseString, lookFor, "</li>");
            QString seoUrl = "https://www.quoka.de/" + GetPartOfString(currentLine, "href=\"/", "\"");
            QString adid = GetPartOfString(currentLine, "data-qng-submit=\"", "\"").replace("|","").trimmed();

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

            /* Quoka does not shown the Pricetype
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
            qInfo() << image;

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

                    if (startDate.length() == 8){
                        startDate = startDate.left(6)+"20"+startDate.right(2);
                    }

                    startDate = startDate + " 00:00:00";

                    MyDateTime = QDateTime::fromString(startDate , "dd.MM.yyyy HH:mm:ss");
                    if (!MyDateTime.isNull() && MyDateTime.isValid())
                        startDate = MyDateTime.toString("yyyy-MM-dd HH:mm:ss");
                    else {
                        qWarning() << "No valid StartTime: " << startDate;
                        startDate = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
                    }
                }
                catch (std::exception ex1)
                {
                    responseString = GetPartOfString(responseString, lookFor, "");
                    qWarning() << ex1.what();
                    continue;
                }
            }

            if (startDate.trimmed() == ""){
                responseString = GetPartOfString(responseString, lookFor, "");
                continue;
            }

            endDate = CalcEndTime(startDate);

            title = FixHtml(title);
            dist = FixHtml(dist);

            if ((!title.isEmpty() || !dist.isEmpty()) && title != "{{name}}")
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
QString QuokaAgentPlugin::GetPartOfString(const QString &sourceString, const QString &fromString, const QString &toString)
{
    QString src = sourceString;

    if (src.isEmpty())
        return "";

    int start = 0;
    if (fromString != "")
    {
        start = src.indexOf(fromString);
        if (start >= 0)
        {
            start += fromString.length();
            int end = src.length();
            if (toString != "")
            {
                end = src.indexOf(toString, start);
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
        if (toString != "")
        {
            end = src.indexOf(toString, start + fromString.length());
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
QString QuokaAgentPlugin::GetHtmlSourceCode(const QString &url, const QUrlQuery &post)
{
    QNetworkAccessManager manager;

    QUrl uri(url);
    QEventLoop eventLoop;

    QNetworkRequest request(uri);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);

    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    QNetworkReply *reply;
    if (post.isEmpty())
        reply = manager.get(request);
    else
        reply = manager.post(request,post.toString(QUrl::FullyEncoded).toUtf8());
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
QString QuokaAgentPlugin::FixDateTime(const QString &customDateString)
{
    if (!customDateString.contains("T") && !customDateString.contains("+"))
        return customDateString;

    QString dateTime = customDateString.left(23);
    dateTime = dateTime.replace("T"," ");
    int timezoneOffset = customDateString.right(5).left(3).toInt();

    QDateTime timeConvertor = QDateTime::fromString(dateTime, "yyyy-MM-dd HH:mm:ss.zzz");

    if (timeConvertor.isValid()){
        timeConvertor.setTimeSpec(Qt::OffsetFromUTC);
        timeConvertor.setUtcOffset(timezoneOffset * 3600);
        return timeConvertor.toString("yyyy-MM-dd HH:mm:ss");
    } else {
        qWarning() << "fixDateTime throws an exception with format " << customDateString << ", return today";
        return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    }
}

/**
 * @brief QuokaAgentPlugin::CalcEndTime
 * @param rFromString Calc endtime from starttime
 * @return Formatted EndTime String, yyyy-MM-dd HH:mm:ss
 */
QString QuokaAgentPlugin::CalcEndTime(const QString &fromString)
{
    QString fixdate = FixDateTime(fromString);

    QDateTime f = QDateTime::fromString(fixdate,"yyyy-MM-dd HH:mm:ss");

    if (f.isNull() || !f.isValid())
    {
        f = QDateTime::currentDateTime();
        qWarning() << "calcEbayEndTime throws an exception " << fromString << " fixdate " << fixdate;
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
QString QuokaAgentPlugin::FixHtml(const QString &fromString)
{
    QString returnString = fromString;
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

/**
 * @brief QuokaAgentPlugin::GetPlatformColor
 * @return Platformlabel color
 */
QColor QuokaAgentPlugin::GetPlatformColor()
{
    return QColor::fromRgb(0,0,255);
}

QString QuokaAgentPlugin::GetPlatformLetters()
{
    return "Q";
}

/**
 * @brief QuokaAgentPlugin::SendQuestionToAdOwner
 *
 * You must implement your own login and sending process
 *
 * @param accountUsername Username of the selected account at AnzeigenChef
 * @param accountPassword Password of the selected account at AnzeigenChef
 * @param myName Name of the sender
 * @param myPhone Phone of the sender
 * @param advertId Platform id
 * @return true if the message was sent successfully
 */
bool QuokaAgentPlugin::SendQuestionToAdOwner(const QString &accountUsername, const QString &accountPassword, const QString &myName, const QString &myPhone, const QString &advertId)
{
    QNetworkAccessManager manager(this);
    QNetworkCookieJar CookieJar(this);
    manager.setCookieJar(&CookieJar);

    if (Login(accountUsername,accountPassword,&manager))
    {
        // ToDo: 1. Get html source of ad site
        //       2. Read all hidden form fields, Quoka has security mask fields like name="5cf4ca3e637ecf3d1c6ce0049d14404e"
        //       3. Find correct textarea, name, phone
        //       4. Send
        QUrlQuery q;
        QString siteCode = GetHtmlSourceCode("https://www.quoka.de/index.php?controller=ajax&action=searchmailcontactform&adno="+advertId+"&catid=51_5850&entry_action=detail", q);
        qInfo() << siteCode;
        QMap<QString,QString> fieldList = GetListOfFields(siteCode);

        foreach(const QString &key, fieldList.keys())
        {
            // postData.addQueryItem(key,fieldList[key]);
            qInfo() << "add " << key << " with " << fieldList[key];
        }

        return true;
    }
    else
    {
        qWarning() << GetPlatformName() << " Plugin fails with:" << pLastError;
        return false; // pLastError
    }
}

/**
 * @brief QuokaAgentPlugin::GetListOfFields
 * @param fromString HTML source code
 * @return A map with all fields
 */
QMap<QString, QString> QuokaAgentPlugin::GetListOfFields(QString fromString)
{
    QMap<QString, QString> result;
    while (fromString.contains("<input "))
    {
       QString inputElement = GetPartOfString(fromString,"<input",">");
       fromString.remove(0,fromString.indexOf("<input ")+7);

       QString fieldname = GetPartOfString(inputElement,"name=\"","\"");
       QString value = GetPartOfString(inputElement,"value=\"","\"");

       if (!fieldname.isEmpty() && !value.isEmpty())
        result.insert(fieldname, value);
    }

    return result;
}


bool QuokaAgentPlugin::Login(const QString &username, const QString &password, QNetworkAccessManager *manager)
{
    pLastError = "";

    QEventLoop eventLoop2;
    QUrlQuery postData;
    postData.addQueryItem("loginname", username);
    postData.addQueryItem("password", QString(password).replace("+","%2B"));
    postData.addQueryItem("permanent","1");

    QNetworkRequest request2(QUrl("https://www.quoka.de/mein-konto/login.html"));
    request2.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request2.setRawHeader("Referer", "https://www.quoka.de");
    request2.setRawHeader("Accept","text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    request2.setRawHeader("Accept-Language","de-DE");
    request2.setRawHeader("Accept-Charset", "ISO-8859-1,utf-8;q=0.7,*;q=0.7" );
    request2.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QList<RawHeaderPair> responseHeaders;

    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), &eventLoop2, SLOT(quit()));
    QNetworkReply *reply2 = manager->post(request2,postData.toString(QUrl::FullyEncoded).toUtf8());
    eventLoop2.exec();

    QString responseString;
    auto error_type = reply2->error();
    if (error_type == QNetworkReply::NoError) {
        responseString = reply2->readAll();
        responseHeaders = reply2->rawHeaderPairs();
    } else {
        pLastError = tr("Login nicht möglich: %1").arg(reply2->errorString());
    }

     bool isOK = false;
     QStringList infoHeader;

     foreach(RawHeaderPair x, responseHeaders){
         infoHeader.append(x.first + ": " + x.second);
         if (x.first.trimmed() == "Location"){
             if (x.second.trimmed() == "/mein-konto/startseite.html" || "/mein-konto/online-anzeigen.html"){
                 isOK = true;
                 break;
             }
         }
     }

    reply2->deleteLater();

    if (!isOK)
        pLastError = tr("Der Login bei Quoka war nicht möglich, bitte Zugangsdaten prüfen");

    return isOK;
}

