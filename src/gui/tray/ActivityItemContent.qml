import QtQml 2.15
import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.2
import Style 1.0
import com.nextcloud.desktopclient 1.0

RowLayout {
    id: root

    property variant activityData: {{}}

    property color activityTextTitleColor: Style.ncTextColor

    property bool showDismissButton: false

    property bool childHovered: shareButton.hovered || dismissActionButton.hovered

    signal dismissButtonClicked()
    signal shareButtonClicked()

    spacing: 10

    Item {
        Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
        Layout.leftMargin: 5
        Layout.preferredWidth: shareButton.icon.width * 2
        Layout.preferredHeight: shareButton.icon.height * 2

        Image {
            id: thumbnailImage
            width: parent.width * 0.66
            height: parent.height * 0.66
            anchors.centerIn: parent
            cache: true
            source: model.thumbnail.source
            visible: model.thumbnail !== undefined
            sourceSize.height: 64
            sourceSize.width: 64
        }

        Image {
            id: activityIcon
            width: thumbnailImage.visible ? parent.width * 0.5 : parent.width * 0.8
            height: thumbnailImage.visible ? parent.height * 0.5 : parent.height * 0.8
            anchors.centerIn: if(!thumbnailImage.visible) parent
            anchors.right: if(thumbnailImage.visible) parent.right
            anchors.bottom: if(thumbnailImage.visible) parent.bottom
            cache: true
            source: icon
            sourceSize.height: 64
            sourceSize.width: 64
        }
    }

    Column {
        id: activityTextColumn

        Layout.topMargin: 4
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

        spacing: 4

        Label {
            id: activityTextTitle
            text: (root.activityData.type === "Activity" || root.activityData.type === "Notification") ? root.activityData.subject : root.activityData.message
            width: parent.width
            elide: Text.ElideRight
            font.pixelSize: Style.topLinePixelSize
            color: root.activityData.activityTextTitleColor
        }

        Label {
            id: activityTextInfo
            text: (root.activityData.type === "Sync") ? root.activityData.displayPath
                                    : (root.activityData.type === "File") ? root.activityData.subject
                                                        : (root.activityData.type === "Notification") ? root.activityData.message
                                                                                    : ""
            height: (text === "") ? 0 : activityTextTitle.height
            width: parent.width
            elide: Text.ElideRight
            font.pixelSize: Style.subLinePixelSize
        }

        Label {
            id: activityTextDateTime
            text: root.activityData.dateTime
            height: (text === "") ? 0 : activityTextTitle.height
            width: parent.width
            elide: Text.ElideRight
            font.pixelSize: Style.subLinePixelSize
            color: "#808080"
        }
    }

    Button {
        id: dismissActionButton

        Layout.preferredWidth: parent.height * 0.40
        Layout.preferredHeight: parent.height * 0.40

        Layout.alignment: Qt.AlignCenter

        Layout.margins: Style.roundButtonBackgroundVerticalMargins

        ToolTip.visible: hovered
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.text: qsTr("Dismiss")

        Accessible.name: qsTr("Dismiss")

        visible: root.showDismissButton && !shareButton.visible

        background: Rectangle {
            color: "transparent"
        }

        contentItem: Image {
            anchors.fill: parent
            source: parent.hovered ? "image://svgimage-custom-color/clear.svg/black" : "image://svgimage-custom-color/clear.svg/grey"
            sourceSize.width: 24
            sourceSize.height: 24
        }

        onClicked: root.dismissButtonClicked()
    }

    CustomButton {
        id: shareButton

        Layout.preferredWidth: parent.height * 0.70
        Layout.preferredHeight: parent.height * 0.70

        visible: root.activityData.isShareable

        imageSource: "image://svgimage-custom-color/share.svg" + "/" + Style.ncBlue
        imageSourceHover: "image://svgimage-custom-color/share.svg" + "/" + Style.ncTextColor

        toolTipText: qsTr("Open share dialog")

        bgColor: Style.ncBlue

        onClicked: root.shareButtonClicked()
    }
}
