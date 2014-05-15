#include "precompiled.h"

#include "applicationui.hpp"
#include "AccountImporter.h"
#include "ExportSMS.h"
#include "ImportSMS.h"
#include "InvocationUtils.h"
#include "IOUtils.h"
#include "Logger.h"
#include "MessageImporter.h"
#include "PimUtil.h"

namespace exportui {

using namespace bb::cascades;
using namespace bb::system;
using namespace bb::pim::message;
using namespace canadainc;

ApplicationUI::ApplicationUI(bb::cascades::Application *app) :
        QObject(app), m_cover("Cover.qml"), m_payment(&m_persistance)
{
	switch ( m_invokeManager.startupMode() )
	{
	case ApplicationStartupMode::LaunchApplication:
		initRoot();
		break;

	case ApplicationStartupMode::InvokeCard:
	case ApplicationStartupMode::InvokeApplication:
		connect( &m_invokeManager, SIGNAL( invoked(bb::system::InvokeRequest const&) ), this, SLOT( invoked(bb::system::InvokeRequest const&) ) );
		break;
	}
}


QObject* ApplicationUI::initRoot(QString const& qmlSource)
{
	qmlRegisterType<bb::cascades::pickers::FilePicker>("bb.cascades.pickers", 1, 0, "FilePicker");
	qmlRegisterUncreatableType<bb::cascades::pickers::FileType>("bb.cascades.pickers", 1, 0, "FileType", "Can't instantiate");
	qmlRegisterUncreatableType<bb::cascades::pickers::FilePickerMode>("bb.cascades.pickers", 1, 0, "FilePickerMode", "Can't instantiate");
	qmlRegisterUncreatableType<OutputFormat>("com.canadainc.data", 1, 0, "OutputFormat", "Can't instantiate");

    QmlDocument *qml = QmlDocument::create( QString("asset:///%1").arg(qmlSource) ).parent(this);
    qml->setContextProperty("app", this);
    qml->setContextProperty("persist", &m_persistance);
    qml->setContextProperty("payment", &m_payment);

    AbstractPane* root = qml->createRootObject<AbstractPane>();
    Application::instance()->setScene(root);

	connect( this, SIGNAL( initialize() ), this, SLOT( init() ), Qt::QueuedConnection ); // async startup

	emit initialize();

	return root;
}


void ApplicationUI::invoked(bb::system::InvokeRequest const& request)
{
	QObject* root = initRoot("InvokedPage.qml");
	QString text;

	if ( request.uri().toString().startsWith("pim") )
	{
		QStringList tokens = request.uri().toString().split(":");
	    LOGGER("========= INVOKED DATA" << tokens);

	    if ( tokens.size() > 3 ) {
	    	qint64 accountId = tokens[2].toLongLong();
	    	qint64 messageId = tokens[3].toLongLong();

	    	Message m = MessageService().message(accountId, messageId);
	    	QString name = m.sender().displayableName().trimmed();
	    	root->setProperty( "defaultName", QString("%1.txt").arg(name) );

	        QString timeFormat = tr("MMM d/yy, hh:mm:ss");

	        switch ( m_persistance.getValueFor("timeFormat").toInt() )
	        {
	    		case 1:
	    			timeFormat = tr("hh:mm:ss");
	    			break;

	    		case 2:
	    			timeFormat = "";
	    			break;

	    		default:
	    			break;
	        }

	        QDateTime t = m_persistance.getValueFor("serverTimestamp").toInt() == 1 ? m.serverTimestamp() : m.deviceTimestamp();

	    	text = tr("%1\r\n\r\n%2: %3").arg( m.sender().address() ).arg( timeFormat.isEmpty() ? "" : t.toString(timeFormat) ).arg( PimUtil::extractText(m) );
	    }
	} else {
		text = QString::fromUtf8( request.data().data() );
	}

	root->setProperty("data", text);

	connect( root, SIGNAL( finished() ), this, SLOT( cardFinished() ) );
}


void ApplicationUI::cardFinished() {
	m_invokeManager.sendCardDone( CardDoneMessage() );
}


void ApplicationUI::init()
{
	INIT_SETTING( "userName", tr("You") );
	INIT_SETTING("timeFormat", 0);
	INIT_SETTING("duplicateAction", 0);
	INIT_SETTING("doubleSpace", 0);
	INIT_SETTING("latestFirst", 1);
	INIT_SETTING("serverTimestamp", 1);

	if ( m_persistance.getValueFor("output").isNull() ) // first run
	{
		QString sdDirectory("/accounts/1000/removable/sdcard/documents");

		if ( !QDir(sdDirectory).exists() ) {
			sdDirectory = "/accounts/1000/shared/documents";
		}

		m_persistance.saveValueFor("output", sdDirectory, false);
	}

	bool permissionOK = PimUtil::validateEmailSMSAccess( tr("Warning: It seems like the app does not have access to your Email/SMS messages Folder. This permission is needed for the app to access the SMS and email services it needs to render and process them so they can be saved. If you leave this permission off, some features may not work properly. Select OK to launch the Application Permissions screen where you can turn these settings on.") );

	if (permissionOK)
	{
		permissionOK = InvocationUtils::validateSharedFolderAccess( tr("Warning: It seems like the app does not have access to your Shared Folder. This permission is needed for the app to access the file system so that it can save the text messages as files. If you leave this permission off, some features may not work properly.") );

		if (permissionOK) {
			PimUtil::validateContactsAccess( tr("Warning: It seems like the app does not have access to your contacts. This permission is needed for the app to access your address book so we can properly display the names of the contacts in the output files. If you leave this permission off, some features may not work properly. Select OK to launch the Application Permissions screen where you can turn these settings on.") );
		}
	}
}


void ApplicationUI::create(bb::cascades::Application *app) {
	new ApplicationUI(app);
}


void ApplicationUI::getConversationsFor(qint64 accountId)
{
	ImportSMS* sms = new ImportSMS(accountId);
	connect( sms, SIGNAL( importCompleted(QVariantList const&) ), this, SIGNAL( conversationsImported(QVariantList const&) ) );
	connect( sms, SIGNAL( progress(int, int) ), this, SIGNAL( conversationLoadProgress(int, int) ) );
	IOUtils::startThread(sms);
}


void ApplicationUI::getMessagesFor(QString const& conversationKey, qint64 accountId)
{
	 MessageImporter* ai = new MessageImporter(accountId, false);
	 ai->setUserAlias( m_persistance.getValueFor("userName").toString() );
	 ai->setConversation(conversationKey);
	 ai->setLatestFirst( m_persistance.getValueFor("latestFirst") == 1 );
	 ai->setUseDeviceTime( m_persistance.getValueFor("serverTimestamp") != 1 );

	 connect( ai, SIGNAL( importCompleted(QVariantList const&) ), this, SIGNAL( messagesImported(QVariantList const&) ) );
	 connect( ai, SIGNAL( progress(int, int) ), this, SIGNAL( loadProgress(int, int) ) );

	 IOUtils::startThread(ai);
}


void ApplicationUI::onExportCompleted() {
	m_persistance.showToast( tr("Export complete"), "", "asset:///images/menu/ic_export.png" );
}


void ApplicationUI::exportSMS(QStringList const& conversationIds, qint64 accountId, int outputFormat)
{
    LOGGER(conversationIds << accountId << outputFormat);

	ExportSMS* sms = new ExportSMS(conversationIds, accountId);
	sms->setFormat( static_cast<OutputFormat::Type>(outputFormat) );
	connect( sms, SIGNAL( exportCompleted() ), this, SLOT( onExportCompleted() ) );

	IOUtils::startThread(sms);
}


void ApplicationUI::saveTextData(QString const& file, QString const& data) {
	IOUtils::writeTextFile( file, data, m_persistance.getValueFor("duplicateAction").toInt() == 1 );
}


void ApplicationUI::loadAccounts()
{
	AccountImporter* ai = new AccountImporter();
	connect( ai, SIGNAL( importCompleted(QVariantList const&) ), this, SIGNAL( accountsImported(QVariantList const&) ) );
	IOUtils::startThread(ai);
}


ApplicationUI::~ApplicationUI()
{
}

}
