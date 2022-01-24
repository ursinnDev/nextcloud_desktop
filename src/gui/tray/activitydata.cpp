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

    OCC::Activity Activity::fromActivityJson(const QJsonObject json, const QString accName)
    {
        Activity activity;
        activity._type = Activity::ActivityType;
        activity._objectType = json.value(QStringLiteral("object_type")).toString();
        activity._id = json.value(QStringLiteral("activity_id")).toInt();
        activity._fileAction = json.value(QStringLiteral("type")).toString();
        activity._accName = accName;
        activity._subject = json.value(QStringLiteral("subject")).toString();
        activity._message = json.value(QStringLiteral("message")).toString();
        activity._file = json.value(QStringLiteral("object_name")).toString();
        activity._link = QUrl(json.value(QStringLiteral("link")).toString());
        activity._dateTime = QDateTime::fromString(json.value(QStringLiteral("datetime")).toString(), Qt::ISODate);
        activity._icon = json.value(QStringLiteral("icon")).toString();

        auto richSubjectData = json.value(QStringLiteral("subject_rich")).toArray();

        if(richSubjectData.size() > 1) {
            activity._subjectRich = richSubjectData[0].toString();
            auto parameters = richSubjectData[1].toObject();
            const QRegularExpression subjectRichParameterRe(QStringLiteral("({[a-zA-Z0-9]*})"));
            const QRegularExpression subjectRichParameterBracesRe(QStringLiteral("[{}]"));

            for (auto i = parameters.begin(); i != parameters.end(); ++i) {
                const auto parameterJsonObject = i.value().toObject();

                activity._subjectRichParameters[i.key()] = Activity::RichSubjectParameter  {
                    parameterJsonObject.value(QStringLiteral("type")).toString(),
                    parameterJsonObject.value(QStringLiteral("id")).toString(),
                    parameterJsonObject.value(QStringLiteral("name")).toString(),
                    parameterJsonObject.contains(QStringLiteral("path")) ? parameterJsonObject.value(QStringLiteral("path")).toString() : QString(),
                    parameterJsonObject.contains(QStringLiteral("link")) ? QUrl(parameterJsonObject.value(QStringLiteral("link")).toString()) : QUrl(),
                };
            }

            auto displayString = activity._subjectRich;
            auto i = subjectRichParameterRe.globalMatch(displayString);

            while (i.hasNext()) {
                const auto match = i.next();
                auto word = match.captured(1);
                word.remove(subjectRichParameterBracesRe);

                Q_ASSERT(activity._subjectRichParameters.contains(word));
                displayString = displayString.replace(match.captured(1), activity._subjectRichParameters[word].name);
            }

            activity._subjectDisplay = displayString;
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

            activity._previews.append(data);
        }

        if(!previewsData.isEmpty()) {
            if(activity._icon.contains(QStringLiteral("add-color.svg"))) {
                activity._icon = "qrc:///client/theme/colored/add-bordered.svg";
            } else if(activity._icon.contains(QStringLiteral("delete-color.svg"))) {
                activity._icon = "qrc:///client/theme/colored/delete-bordered.svg";
            } else if(activity._icon.contains(QStringLiteral("change.svg"))) {
                activity._icon = "qrc:///client/theme/colored/change-bordered.svg";
            }
        }

        return activity;
    }

    OCC::Activity Activity::fromSyncFileItemPtr(const SyncFileItemPtr item, const AccountPtr account, const Folder *folder)
    {
        Activity activity;
        activity._type = Activity::SyncFileItemType; //client activity
        activity._status = item->_status;
        activity._dateTime = QDateTime::currentDateTime();
        activity._message = item->_originalFile;
        activity._link = account->url();
        activity._accName = account->displayName();
        activity._file = item->_file;
        activity._folder = folder->alias();
        activity._fileAction = "";

        const auto fileName = QFileInfo(item->_originalFile).fileName();

        if (item->_instruction == CSYNC_INSTRUCTION_REMOVE) {
            activity._fileAction = "file_deleted";
        } else if (item->_instruction == CSYNC_INSTRUCTION_NEW) {
            activity._fileAction = "file_created";
        } else if (item->_instruction == CSYNC_INSTRUCTION_RENAME) {
            activity._fileAction = "file_renamed";
        } else {
            activity._fileAction = "file_changed";
        }

        if (item->_status == SyncFileItem::NoStatus || item->_status == SyncFileItem::Success) {

            if (item->_direction != SyncFileItem::Up) {
                activity._message = QObject::tr("Synced %1").arg(fileName);
            } else if (activity._fileAction == "file_renamed") {
                activity._message = QObject::tr("You renamed %1").arg(fileName);
            } else if (activity._fileAction == "file_deleted") {
                activity._message =QObject:: tr("You deleted %1").arg(fileName);
            } else if (activity._fileAction == "file_created") {
                activity._message = QObject::tr("You created %1").arg(fileName);
            } else {
                activity._message = QObject::tr("You changed %1").arg(fileName);
            }

            if(activity._fileAction != "file_deleted") {
                auto remotePath = folder->remotePath();
                remotePath.append(activity._fileAction == "file_renamed" ? item->_renameTarget : activity._file);
                PreviewData preview;
                preview._source = account->url().toString() + QLatin1String("/index.php/apps/files/api/v1/thumbnail/150/150/") + remotePath;
                activity._previews.append(preview);
            }

        } else {
            activity._subject = item->_errorString;
        }

        return activity;
    }
}
