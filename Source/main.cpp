#include <QCoreApplication>
#include <QThread>

#include <iostream>
#include <csignal>
#include <memory>

/// temporarily included
#include <QDebug>
#include <QFile>
///======================

#include <Source/Global.hpp>

#include "Server/Server.hpp"
#include "Core/BotManager.hpp"

std::unique_ptr<QCoreApplication> a;
std::vector<std::unique_ptr<ThreadId>> threadList;

#ifdef Q_OS_UNIX
void terminate(int)
{
    std::cout << "---App: signal received, preparing to terminate...." << std::endl;
    a->quit();
}
#endif

#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QDBusAbstractAdaptor>
#include <QDBusVariant>
#include <QProcess>
const QString dbus_service_name = "moe.misaka-oneesama.discordbot";

int main(int argc, char **argv)
{
    a.reset(new QCoreApplication(argc, argv));
    a->setApplicationName(QString::fromUtf8("御坂ーお姉さま"));
    a->setApplicationVersion(QLatin1String("v0.0.1"));
    a->setOrganizationName(QString::fromUtf8("マギルゥーベルベット"));
    a->setOrganizationDomain(QLatin1String("magiruuvelvet.gdn"));

    if (!QDBusConnection::sessionBus().isConnected()){}

    // Initialize configuration
    std::cout << "---App: creating [class] ConfigManager..." << std::endl;
    configManager = new ConfigManager();
    configManager->loadConfig();

     /// JUST FOR DEVELOPMENT =========================================================================================
      // MUST be a bot token, don't run this bot on a user account, note: QDiscord rejects user tokens anyway ;P
      QFile tokenFile(configManager->configPath() + "/token");
      if (tokenFile.exists() && tokenFile.open(QFile::ReadOnly | QFile::Text))
      {
          // create a file in "$config_path/token" and paste your bots token here
          // file MUST NOT END WITH A NEWLINE and MUST BE UTF-8 ENCODED!!!
          configManager->setOAuthToken(tokenFile.readAll().constData());
          tokenFile.close();
      }
     ///===============================================================================================================

    // Initialize debugger
    std::cout << "---App: creating [class] Debugger..." << std::endl;
    debugger = new Debugger();
    debugger->setMaxLogFilesToKeep(configManager->maxLogFilesToKeep());
    debugger->setLogDir(configManager->configPath() + "/logs");
    debugger->setEnabled(false);
    debugger->printToTerminal(configManager->debuggerPrintToTerminal()); // true for debug builds, false otherwise


    if (a->arguments().size() == 1)
    {
        std::cout << "master process started" << std::endl;
        QDBusServiceWatcher serviceWatcher(dbus_service_name, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration);

        //QObject::connect(&serviceWatcher, &QDBusServiceWatcher::serviceRegistered, );
        QProcess *server = new QProcess(a.get());
        server->setStandardOutputFile("./misakaoneesama_server.log");
        server->setStandardErrorFile("./misakaoneesama-server-err.log");
        server->start(a->arguments().at(0), QStringList("server"), QProcess::ReadOnly);

        QProcess *bot = new QProcess(a.get());
        bot->setStandardOutputFile("./misakaoneesama_bot.log");
        bot->setStandardErrorFile("./misakaoneesama-bot-err.log");
        bot->start(a->arguments().at(0), QStringList("bot"), QProcess::ReadOnly);
    }

    else if (a->arguments().at(1) == "server")
    {
        std::cout << "server process started" << std::endl;
        Server *server = new Server();
        server->setListeningAddress(QLatin1String("127.0.0.1"));
        server->setListeningPort(4555);
        QTimer::singleShot(0, server, &Server::start);
    }

    else if (a->arguments().at(1) == "bot")
    {
        std::cout << "bot process started" << std::endl;
        BotManager *botManager = new BotManager();
        botManager->setOAuthToken(configManager->token());
        botManager->init();
        QTimer::singleShot(0, botManager, static_cast<void(BotManager::*)()>(&BotManager::login));
    }

    return a->exec();
}

int _main(int argc, char **argv)
{
    a.reset(new QCoreApplication(argc, argv));
    a->setApplicationName(QString::fromUtf8("御坂ーお姉さま"));
    a->setApplicationVersion(QLatin1String("v0.0.1"));
    a->setOrganizationName(QString::fromUtf8("マギルゥーベルベット"));
    a->setOrganizationDomain(QLatin1String("magiruuvelvet.gdn"));

    threadList.push_back(std::move(std::unique_ptr<ThreadId>(new ThreadId("main"))));
    threadList.push_back(std::move(std::unique_ptr<ThreadId>(new ThreadId("server"))));
    threadList.push_back(std::move(std::unique_ptr<ThreadId>(new ThreadId("bot"))));

    QThread::currentThread()->setUserData(0, threadList.at(0).get());

#ifdef Q_OS_UNIX
    std::cout << "---App: registering signals SIGINT, SIGTERM and SIGQUIT..." << std::endl;
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);
    signal(SIGQUIT, terminate);
#endif

    // Initialize configuration
    std::cout << "---App: creating [class] ConfigManager..." << std::endl;
    configManager = new ConfigManager();
    configManager->loadConfig();

     /// JUST FOR DEVELOPMENT =========================================================================================
      // MUST be a bot token, don't run this bot on a user account, note: QDiscord rejects user tokens anyway ;P
      QFile tokenFile(configManager->configPath() + "/token");
      if (tokenFile.exists() && tokenFile.open(QFile::ReadOnly | QFile::Text))
      {
          // create a file in "$config_path/token" and paste your bots token here
          // file MUST NOT END WITH A NEWLINE and MUST BE UTF-8 ENCODED!!!
          configManager->setOAuthToken(tokenFile.readAll().constData());
          tokenFile.close();
      }
     ///===============================================================================================================

    // Initialize debugger
    std::cout << "---App: creating [class] Debugger..." << std::endl;
    debugger = new Debugger();
    debugger->setMaxLogFilesToKeep(configManager->maxLogFilesToKeep());
    debugger->setLogDir(configManager->configPath() + "/logs");
    debugger->setEnabled(true);
    debugger->printToTerminal(configManager->debuggerPrintToTerminal()); // true for debug builds, false otherwise

    debugger->notice("Creating threads...");

    // Initialize Server Thread
    debugger->notice("Creating server thread...");
    QThread *serverThread = new QThread;
    serverThread->setUserData(0, threadList.at(1).get());
    Server *server = new Server();
    server->setListeningAddress(QLatin1String("127.0.0.1")); //= todo: add to ConfigManager
    server->setListeningPort(4555);                          //=
    server->moveToThread(serverThread);

    QObject::connect(serverThread, &QThread::started, server, &Server::start);
    QObject::connect(server, &Server::stopped, serverThread, &QThread::quit);
    QObject::connect(server, &Server::stopped, server, &Server::deleteLater);
    QObject::connect(serverThread, &QThread::finished, serverThread, &QThread::deleteLater);

    serverThread->start();
    debugger->notice("Server thread started.");

    // Initialize Bot Thread
    debugger->notice("Creating bot thread...");
    QThread *botThread = new QThread;
    botThread->setUserData(0, threadList.at(2).get());
    BotManager *botManager = new BotManager();
    botManager->moveToThread(botThread);
    botManager->setOAuthToken(configManager->token());
    botManager->init();

    QObject::connect(botThread, &QThread::started, botManager, static_cast<void(BotManager::*)()>(&BotManager::login));
    QObject::connect(botManager, &BotManager::stopped, botThread, &QThread::quit);
    QObject::connect(botManager, &BotManager::stopped, botManager, &BotManager::deleteLater);
    QObject::connect(botThread, &QThread::finished, botThread, &QThread::deleteLater);

    //QObject::connect(serverThread, &QThread::finished, botThread, &QThread::quit);

    botThread->start();
    debugger->notice("Bot thread started.");

    debugger->notice("All threads started.");

    // notice: segfauls on termination using UNIX signal
    // fixme: implement a proper event loop for the code in here

    int status = a->exec();

    std::cout << "---App: cleaning up and freeing resources..." << std::endl;

    a.reset();

    //delete botManager;
    //botThread->quit();
    //delete server;
    //serverThread->quit();

    delete configManager;
    delete debugger;

    threadList.clear();

    std::cout << "---App: exited with status code " << status << std::endl;

    return status;
}
