import QtQuick 2.15
import Style 1.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.2
import com.nextcloud.desktopclient 1.0

Column {
    id: talkLayout
         
    function sendMessage() {        
        if (talkMessage.text === "") {
            rectangleInput.border.color = Style.errorBoxBorderColor;
            return;
        }
        rectangleInput.border.color = Style.menuBorder;
        UserModel.currentUser.sendChatMessage(model.conversationToken, talkMessage.text, model.messageId);
        talkMessageSent.text = talkMessage.text;
        talkMessage.clear();
        rectangleInput.visible = false
    }
    
    Text {
        id: talkMessageSent
        font.pixelSize: Style.topLinePixelSize
        color: Style.menuBorder
        visible: talkMessageSent.text !== ""
        topPadding: 4
    }
    
    Rectangle {  
        id: rectangleInput
        width: 250
        height: 32
        border.width: 1
        border.color: Style.ncBlue
        radius: 22
        clip: true
        TextInput {
            id: talkMessage
            font.pixelSize: Style.topLinePixelSize
            height: activityTextInfo.height
            width: parent.width
            onAccepted: sendMessage()
            topPadding: 4
            leftPadding: 12
            anchors.verticalCenter: parent.verticalCenter 
            Text {
                id: placeholderText
                text: qsTr("Reply to ...")
                color: Style.menuBorder
                visible: talkMessage.text === ""
                width: parent.width
                height: parent.height
                leftPadding: 12
                anchors.verticalCenter: parent.verticalCenter 
            }
        }    
        Button {
            id: sendReply
            icon.source: "image://svgimage-custom-color/send.svg" + "/" + Style.ncBlue
            icon.width: 32
            icon.height: 32
            icon.color: Style.ncBlue
            width: 32
            height: parent.height
            opacity: 0.8
            anchors.right: rectangleInput.right
            anchors.top: rectangleInput.top
            onClicked: sendMessage()
            flat: true
            background: Rectangle {
                color: parent.hovered ? Style.lightHover : "transparent"
            }
            
            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Send reply to chat message")        
        }
    }
}
