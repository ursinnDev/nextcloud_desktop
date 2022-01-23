#ifndef NOTIFICATIONHANDLER_H
#define NOTIFICATIONHANDLER_H

#include <QtCore>

#include "usermodel.h"

class QJsonDocument;

namespace OCC {

class TalkReply;

class ServerNotificationHandler : public QObject
{
    Q_OBJECT
public:
    explicit ServerNotificationHandler(AccountState *accountState, QObject *parent = nullptr);

signals:
    void newNotificationList(ActivityList);

public slots:
    void slotFetchNotifications();

private slots:
    void slotNotificationsReceived(const QJsonDocument &json, int statusCode);
    void slotEtagResponseHeaderReceived(const QByteArray &value, int statusCode);
    void slotAllowDesktopNotificationsChanged(bool isAllowed);

private:
    QPointer<JsonApiJob> _notificationJob;
    AccountState *_accountState;
    QPointer<TalkReply> _talkReply;
};
}

#endif // NOTIFICATIONHANDLER_H
