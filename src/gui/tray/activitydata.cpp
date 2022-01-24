/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include <QtCore>

#include "activitydata.h"


namespace OCC {

bool operator<(const Activity &rhs, const Activity &lhs)
{
    return rhs._dateTime > lhs._dateTime;
}

bool operator==(const Activity &rhs, const Activity &lhs)
{
    return (rhs._type == lhs._type && rhs._id == lhs._id && rhs._accName == lhs._accName);
}

Activity::Identifier Activity::ident() const
{
    return Identifier(_id, _accName);
}

ActivityLink ActivityLink::createFomJsonObject(const QJsonObject &obj)
{
    ActivityLink activityLink;
    activityLink._label = QUrl::fromPercentEncoding(obj.value(QStringLiteral("label")).toString().toUtf8());
    activityLink._link = obj.value(QStringLiteral("link")).toString();
    activityLink._verb = obj.value(QStringLiteral("type")).toString().toUtf8();
    activityLink._primary = obj.value(QStringLiteral("primary")).toBool();

    return activityLink;
}

OCC::Activity Activity::fromActivityJson(QJsonObject json)
{
    Activity a;
    a._type = Activity::ActivityType;
    a._objectType = json.value(QStringLiteral("object_type")).toString();
    a._id = json.value(QStringLiteral("activity_id")).toInt();
    a._fileAction = json.value(QStringLiteral("type")).toString();
    a._subject = json.value(QStringLiteral("subject")).toString();
    a._message = json.value(QStringLiteral("message")).toString();
    a._file = json.value(QStringLiteral("object_name")).toString();
    a._link = QUrl(json.value(QStringLiteral("link")).toString());
    a._dateTime = QDateTime::fromString(json.value(QStringLiteral("datetime")).toString(), Qt::ISODate);
    a._icon = json.value(QStringLiteral("icon")).toString();

    auto richSubjectData = json.value(QStringLiteral("subject_rich")).toArray();

    if(richSubjectData.size() > 1) {
        a._subjectRich = richSubjectData[0].toString();
        auto parameters = richSubjectData[1].toObject();
        const QRegularExpression subjectRichParameterRe(QStringLiteral("({[a-zA-Z0-9]*})"));
        const QRegularExpression subjectRichParameterBracesRe(QStringLiteral("[{}]"));

        for (auto i = parameters.begin(); i != parameters.end(); ++i) {
            const auto parameterJsonObject = i.value().toObject();

            a._subjectRichParameters[i.key()] = Activity::RichSubjectParameter  {
                parameterJsonObject.value(QStringLiteral("type")).toString(),
                parameterJsonObject.value(QStringLiteral("id")).toString(),
                parameterJsonObject.value(QStringLiteral("name")).toString(),
                parameterJsonObject.contains(QStringLiteral("path")) ? parameterJsonObject.value(QStringLiteral("path")).toString() : QString(),
                parameterJsonObject.contains(QStringLiteral("link")) ? QUrl(parameterJsonObject.value(QStringLiteral("link")).toString()) : QUrl(),
            };
        }

        auto displayString = a._subjectRich;
        auto i = subjectRichParameterRe.globalMatch(displayString);

        while (i.hasNext()) {
            const auto match = i.next();
            auto word = match.captured(1);
            word.remove(subjectRichParameterBracesRe);

            Q_ASSERT(a._subjectRichParameters.contains(word));
            displayString = displayString.replace(match.captured(1), a._subjectRichParameters[word].name);
        }

        a._subjectDisplay = displayString;
    }

    const auto previewsData = json.value(QStringLiteral("previews")).toArray();

    for(const auto preview : previewsData) {
        const auto jsonPreviewData = preview.toObject();

        PreviewData data;
        data._source = jsonPreviewData.value(QStringLiteral("source")).toString();
        data._link = jsonPreviewData.value(QStringLiteral("link")).toString();
        data._mimeType = jsonPreviewData.value(QStringLiteral("mimeType")).toString();
        data._fileId = jsonPreviewData.value(QStringLiteral("fileId")).toInt();
        data._view = jsonPreviewData.value(QStringLiteral("view")).toString();
        data._isMimeTypeIcon = jsonPreviewData.value(QStringLiteral("isMimeTypeIcon")).toBool();
        data._filename = jsonPreviewData.value(QStringLiteral("filename")).toString();

        a._previews.append(data);
    }

    if(!previewsData.isEmpty()) {
        if(a._icon.contains(QStringLiteral("add-color.svg"))) {
            a._icon = "qrc:///client/theme/colored/add-bordered.svg";
        } else if(a._icon.contains(QStringLiteral("delete-color.svg"))) {
            a._icon = "qrc:///client/theme/colored/delete-bordered.svg";
        } else if(a._icon.contains(QStringLiteral("change.svg"))) {
            a._icon = "qrc:///client/theme/colored/change-bordered.svg";
        }
    }

    return a;
}

}
