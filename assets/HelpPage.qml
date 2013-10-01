import bb.cascades 1.0
import bb 1.0

BasePage
{
    attachedObjects: [
        ApplicationInfo {
            id: appInfo
        },

        PackageInfo {
            id: packageInfo
        }
    ]

    contentContainer: Container
    {
        leftPadding: 20; rightPadding: 20

        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        ScrollView {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            Label {
                multiline: true
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                textStyle.textAlign: TextAlign.Center
                textStyle.fontSize: FontSize.Small
                content.flags: TextContentFlag.ActiveText
                text: qsTr("\n\n(c) 2013 %1. All Rights Reserved.\n%2 %3\n\nPlease report all bugs to:\nsupport@canadainc.org\n\nEver wanted to forward or share multiple SMS or Email conversations with others (or yourself)? Or ever wanted to persist these messages into the file system? This is where Exporter comes in.\n\nThis app makes it really easy to select which conversations you want to share and even allows you to select a subset of the messages that you want to share. You can also select these conversations to be archived into your SD card, a cloud storage account, or any other persistent storage. After selecting the conversations you are free to share it in whichever channel you wish (ie: Facebook, Remember, Email, BBM, SMS, etc.)\n\nThe great thing is that the app gives you access to nearly all your accounts to export messages from. This includes emails, SMS, PIN messages, etc. Exporter is also registered as in BB10's invocation framework and this means that any plain-text information can easily be saved to your file system when you use a Share action!\n\n").arg(packageInfo.author).arg(appInfo.title).arg(appInfo.version)
            }
        }
    }
}
