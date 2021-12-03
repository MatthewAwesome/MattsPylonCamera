import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtMultimedia 5.8
import Filters 1.0

Window {
    width: 1200
    height: 870
    visible: true
    title: qsTr("Matt's Pylon Camera")
    // We want this button to open/close connection with Basler Camera.
    RowLayout{
        Layout.margins: 10
        anchors.top:parent.top
        anchors.left:parent.left
        anchors.leftMargin: 5
        anchors.topMargin: 5

        Layout.preferredHeight: 70
        Layout.preferredWidth: parent.width - 10
        Button{
            id:openCloseButton
            text: "Open Camera"
            property bool opened:false
            onClicked:{
                pylonCamera.isOpen() ? pylonCamera.close() : pylonCamera.open();
                pylonCamera.isOpen() ? buttonRect.border.color = "Red" : buttonRect.border.color = "Green"
                pylonCamera.isOpen() ? this.text = "Close Camera" :  this.text = "Open Camera"
                // Add stuffs for the stream button:
                pylonCamera.isOpen() && pylonCamera.isGrabbing() ? streamRect.border.color = "Red" : streamRect.border.color = "Green"
                pylonCamera.isOpen() && pylonCamera.isGrabbing() ? streamButton.text = "Stop Stream" :  streamButton.text = "Start Stream"
            }
            Rectangle{
                id:buttonRect
                anchors.fill: parent
                border.color: "Green"

            }
//                Layout.preferredHeight: 50
//                Layout.preferredWidth: 100
        }

        Button{
            id:streamButton
            text: "Start Stream"
            onClicked:{
                if(!pylonCamera.isOpen()){
                    pylonCamera.open();
                    buttonRect.border.color = "Red";
                    openCloseButton.text = "Close Camera"
                };
                pylonCamera.isGrabbing() ? pylonCamera.stopGrabbing() : pylonCamera.startStream();
                pylonCamera.isGrabbing() ? streamRect.border.color = "Red" : streamRect.border.color = "Green"
                pylonCamera.isGrabbing() ? this.text = "Stop Stream" :  this.text = "Start Stream"
            }
            Rectangle{
                id:streamRect
                anchors.fill: parent
                border.color: "Green"
            }
            leftPadding: 10
        }
    }
    VideoFilter{
        id:videoFilter
    }
    CannyFilter{
        id:cannyFilter
    }
    BWFilter{
        id:bwFilter
    }
    HoughFilter{
        id:houghFilter
    }
    // Filter for processing:
    Item{
        width:parent.width-30
        height:parent.height-50
        anchors.centerIn: parent
        GridLayout{
            anchors.fill:parent
            columns:2
            rows:2
            Layout.margins: 10
            VideoOutput {
                source: pylonCamera.streamReturn(0)
                filters:[videoFilter]
                Layout.preferredHeight: parent.height/2
                Layout.preferredWidth: parent.width/2
                Layout.row: 0
                Layout.column: 0
                // Allows us to draw some stuff on top of the video frame.
//                Canvas {
//                    id: mycanvas
//                    anchors.fill:parent
//                    onPaint: {
//                        var ctx = getContext("2d");
//                        // Let's write a function there that allows us to move the ellipse. It will be a child of pylonCamera!
//                        var vals = [width/2,height/2,50,70];
//        //                var output = pylonCamera.getEllipse();
//        //                console.log(output)
//                        ctx.strokeStyle = "red";
//                        ctx.lineWidth = 3;
//                        ctx.beginPath();
//                        ctx.ellipse(vals[0],vals[1],vals[2],vals[3]);
//                        ctx.stroke();
//                    }

//                }
            }
            VideoOutput {
                source: pylonCamera.streamReturn(1)
                filters:[cannyFilter]
                Layout.preferredHeight: parent.height/2
                Layout.preferredWidth: parent.width/2
                Layout.row: 0
                Layout.column: 1

                // Allows us to draw some stuff on top of the video frame.
            }
            VideoOutput {
                source: pylonCamera.streamReturn(2)
                filters:[bwFilter]
                Layout.preferredHeight: parent.height/2
                Layout.preferredWidth: parent.width/2
                Layout.row: 1
                Layout.column: 0
                // Allows us to draw some stuff on top of the video frame.
            }
            VideoOutput {
                Layout.preferredHeight: parent.height/2
                Layout.preferredWidth: parent.width/2
                Layout.row: 1
                Layout.column: 1
                source: pylonCamera.streamReturn(3)
                filters:[houghFilter]
                // Allows us to draw some stuff on top of the video frame.
            }


        }
    }
    // Renders the video frame:
//    VideoOutput {
//        source: pylonCamera.streamReturn()
//        anchors.fill: parent
//        filters:[videoFilter]
//        // Allows us to draw some stuff on top of the video frame.
//        Canvas {
//            id: mycanvas
//            anchors.fill:parent
//            onPaint: {
//                var ctx = getContext("2d");
//                // Let's write a function there that allows us to move the ellipse. It will be a child of pylonCamera!
//                var vals = [width/2,height/2,50,70];
////                var output = pylonCamera.getEllipse();
////                console.log(output)
//                ctx.strokeStyle = "red";
//                ctx.lineWidth = 3;
//                ctx.beginPath();
//                ctx.ellipse(vals[0],vals[1],vals[2],vals[3]);
//                ctx.stroke();
//            }

//        }
//    }


    // Nex we want Button begin grabbing video:

}
