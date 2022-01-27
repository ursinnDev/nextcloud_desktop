/*
 * Copyright (C) by Claudio Cambra <claudio.cambra@nextcloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "gui/tray/activitydata.h"
#include "account.h"
#include "accountstate.h"
#include "configfile.h"
#include "syncenginetestutils.h"
#include "syncfileitem.h"
#include "folder.h"
#include "folderman.h"
#include "testhelper.h"

#include <QTest>

class TestActivityData : public QObject
{
    Q_OBJECT

public:
    TestActivityData() = default;

    void jsonSpecificFormatTest(QString fileFormat, QString mimeType)
    {
        const auto objectType = QStringLiteral("files");
        const auto subject = QStringLiteral("You created path/test.").append(fileFormat);
        const auto path = QStringLiteral("path/test.").append(fileFormat);
        const auto fileName = QStringLiteral("test.").append(fileFormat);
        const auto activityType = QStringLiteral("file");
        const auto activityId = 90000;
        const auto message = QStringLiteral();
        const auto objectName = QStringLiteral("test.").append(fileFormat);
        const auto link = account->url().toString().append(QStringLiteral("/f/")).append(activityId);
        const auto datetime = QDateTime::currentDateTime().toString(Qt::ISODate);
        const auto icon = account->url().toString().append(QStringLiteral("/apps/files/img/add-color.svg"));

        const QJsonObject richStringData({
            {QStringLiteral("type"), activityType},
            {QStringLiteral("id"), activityId},
            {QStringLiteral("link"),  link},
            {QStringLiteral("name"), fileName},
            {QStringLiteral("path"), objectName}
        });

        const auto subjectRichString = QStringLiteral("You created {file1}");
        const auto subjectRichObj = QJsonObject({{QStringLiteral("file1"), richStringData}});
        const auto subjectRichData = QJsonArray({subjectRichString, subjectRichObj});

        const auto previewUrl = account->url().toString().append(QStringLiteral("/index.php/core/preview.png?file=/")).append(path);

        // Text file previews should be replaced by mimetype icon
        const QJsonObject previewData({
            {QStringLiteral("link"), link},
            {QStringLiteral("mimeType"), mimeType},
            {QStringLiteral("fileId"), activityId},
            {QStringLiteral("filename"), fileName},
            {QStringLiteral("view"), QStringLiteral("files")},
            {QStringLiteral("source"), previewUrl},
            {QStringLiteral("isMimeTypeIcon"), false},
        });

        QJsonObject testData({
            {QStringLiteral("object_type"), objectType},
            {QStringLiteral("activity_id"), activityId},
            {QStringLiteral("type"), activityType},
            {QStringLiteral("subject"), subject},
            {QStringLiteral("message"), message},
            {QStringLiteral("object_name"), objectName},
            {QStringLiteral("link"), link},
            {QStringLiteral("datetime"), datetime},
            {QStringLiteral("icon"), icon},
            {QStringLiteral("subject_rich"), subjectRichData},
            {QStringLiteral("previews"), QJsonArray({previewData})},
        });

        OCC::Activity activity = OCC::Activity::fromActivityJson(testData, account);
        QCOMPARE(activity._type, OCC::Activity::ActivityType);
        QCOMPARE(activity._objectType, objectType);
        QCOMPARE(activity._id, activityId);
        QCOMPARE(activity._fileAction, activityType);
        QCOMPARE(activity._accName, account->displayName());
        QCOMPARE(activity._subject, subject);
        QCOMPARE(activity._message, message);
        QCOMPARE(activity._file, objectName);
        QCOMPARE(activity._link, link);
        QCOMPARE(activity._dateTime, QDateTime::fromString(datetime, Qt::ISODate));

        QCOMPARE(activity._subjectRichParameters.count(), 1);
        QCOMPARE(activity._subjectDisplay, QStringLiteral("You created ").append(fileName));

        QCOMPARE(activity._previews.count(), 1);
        // We want the different icon when we have a preview
        QCOMPARE(activity._icon, QStringLiteral("qrc:///client/theme/colored/add-bordered.svg"));

        if(fileFormat == "txt") {
            QCOMPARE(activity._previews[0]._source, account->url().toString().append(QStringLiteral("/index.php/apps/theming/img/core/filetypes/text.svg")));
            QCOMPARE(activity._previews[0]._isMimeTypeIcon, true);
            QCOMPARE(activity._previews[0]._mimeType, mimeType);
        } else if(fileFormat == "pdf") {
            QCOMPARE(activity._previews[0]._source, account->url().toString().append(QStringLiteral("/index.php/apps/theming/img/core/filetypes/application-pdf.svg")));
            QCOMPARE(activity._previews[0]._isMimeTypeIcon, true);
        } else {
            QCOMPARE(activity._previews[0]._source, previewUrl);
            QCOMPARE(activity._previews[0]._isMimeTypeIcon, false);
        }

        QCOMPARE(activity._previews[0]._mimeType, mimeType);
    }

    QScopedPointer<FakeQNAM> fakeQnam;
    OCC::AccountPtr account;

private slots:
    void initTestCase()
    {
        account = OCC::Account::create();
        account->setCredentials(new FakeCredentials{fakeQnam.data()});
        account->setUrl(QUrl(("http://example.de")));
        auto *cred = new HttpCredentialsTest("testuser", "secret");
        account->setCredentials(cred);
    }

    void testFromJson()
    {
        jsonSpecificFormatTest(QStringLiteral("jpg"), QStringLiteral("image/jpg"));
        jsonSpecificFormatTest(QStringLiteral("txt"), QStringLiteral("text/plain"));
        jsonSpecificFormatTest(QStringLiteral("pdf"), QStringLiteral("application/pdf"));
    }
};

QTEST_MAIN(TestActivityData)
#include "testactivitydata.moc"
