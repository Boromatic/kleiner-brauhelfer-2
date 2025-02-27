#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QStyleFactory>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>
#include <QFileDialog>
#include <QMessageBox>
#if QT_NETWORK_LIB
#include <QSslSocket>
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
#include <QOperatingSystemVersion>
#endif
#include "brauhelfer.h"
#include "settings.h"
#include "widgets/webview.h"
#include "dialogs/dlgconsole.h"

// Modus, um Datenbankupdates zu testen.
// In diesem Modus wird eine Kopie der Datenbank erstellt.
// Diese Kopie wird aktualisiert ohne die ursprüngliche Datenbank zu verändern.
//#define MODE_TEST_UPDATE

#if defined(QT_NO_DEBUG) && defined(MODE_TEST_UPDATE)
#error MODE_TEST_UPDATE in release build defined.
#endif

// global variables
extern Brauhelfer* bh;
extern Settings* gSettings;
Brauhelfer* bh = nullptr;
Settings* gSettings = nullptr;

static QFile* logFile = nullptr;
static QtMessageHandler defaultMessageHandler;

static bool chooseDatabase()
{
    QString text;
    QString dir;
    if (gSettings->databasePath().isEmpty())
    {
        text = QObject::tr("Es wurde noch keine Datenbank ausgewählt. Soll eine neue angelegt werden oder soll eine bereits vorhandene geöffnet werden?");
        dir = gSettings->settingsDir();
    }
    else
    {
        text = QObject::tr("Soll eine neue Datenbank angelegt werden oder soll eine bereits vorhandene geöffnet werden?");
        dir = gSettings->databaseDir();
    }

    int ret = QMessageBox::question(nullptr, QApplication::applicationName(), text,
                                    QObject::tr("Anlegen"), QObject::tr("Öffnen"), QObject::tr("Abbrechen"));
    if (ret == 1)
    {
        QString databasePath = QFileDialog::getOpenFileName(nullptr, QObject::tr("Datenbank auswählen"),
                                                            dir,
                                                            QObject::tr("Datenbank (*.sqlite);;Alle Dateien (*.*)"));
        if (!databasePath.isEmpty())
        {
            gSettings->setDatabasePath(databasePath);
            return true;
        }
    }
    else if (ret == 0)
    {
        QString databasePath = QFileDialog::getSaveFileName(nullptr, QObject::tr("Datenbank anlegen"),
                                                            dir + "/kb_daten.sqlite",
                                                            QObject::tr("Datenbank") + " (*.sqlite)");
        if (!databasePath.isEmpty())
        {
            QFile file(databasePath);
            QFile file2(":/data/kb_daten.sqlite");
            file.remove();
            if (file2.copy(file.fileName()))
                file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
            gSettings->setDatabasePath(databasePath);
            return true;
        }
    }
    return false;
}

static bool connectDatabase()
{
    while (true)
    {
        // global brauhelfer class
        if (bh)
            delete bh;
        bh = new Brauhelfer(gSettings->databasePath());

        // connect
        if (bh->connectDatabase())
        {
            // check database version
            int version = bh->databaseVersion();
            if (version < 0)
            {
                QMessageBox::critical(nullptr, QApplication::applicationName(), QObject::tr("Die Datenbank ist ungultig."));
            }
            else if (version > bh->supportedDatabaseVersion)
            {
                QMessageBox::critical(nullptr, QApplication::applicationName(),
                                      QObject::tr("Die Datenbankversion (%1) ist zu neu für das Programm. Das Programm muss aktualisiert werden.").arg(version));
            }
            else if (version < bh->supportedDatabaseVersionMinimal)
            {
                QMessageBox::critical(nullptr, QApplication::applicationName(),
                                      QObject::tr("Die Datenbankversion (%1) ist zu alt für das Programm. Die Datenbank muss zuerst mit dem kleinen-brauhelfer v1.4.4.6 aktualisiert werden.").arg(version));
            }
            else if (version < bh->supportedDatabaseVersion)
            {
                int ret = QMessageBox::warning(nullptr, QApplication::applicationName(),
                                               QObject::tr("Die Datenbank muss aktualisiert werden (Version %1 -> %2).").arg(version).arg(bh->supportedDatabaseVersion) + "\n\n" +
                                               QObject::tr("Soll die Datenbank jetzt aktualisiert werden?") + " " +
                                               QObject::tr("ACHTUNG, die Änderungen können nicht rückgängig gemacht werden!"),
                                               QMessageBox::Yes | QMessageBox::No,
                                               QMessageBox::Yes);
                if (ret == QMessageBox::Yes)
                {
                    // create copy of database
                    QFile fileOrg(gSettings->databasePath());
                    QFile fileUpdate(gSettings->databasePath() + "_update.sqlite");
                    fileUpdate.remove();
                    if (fileOrg.copy(fileUpdate.fileName()))
                    {
                        fileUpdate.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

                        // connect and update copy
                        bh->setDatabasePath(fileUpdate.fileName());
                        bh->connectDatabase();
                        if (bh->updateDatabase())
                        {
                            // delete bh to remove file
                            delete bh;
                            bh = nullptr;
                          #ifdef MODE_TEST_UPDATE
                            bh = new Brauhelfer(fileUpdate.fileName());
                            return bh->connectDatabase();
                          #else
                            // copy back
                            fileOrg.remove();
                            if (fileUpdate.copy(fileOrg.fileName()))
                            {
                                fileOrg.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
                                fileUpdate.remove();
                                continue;
                            }
                            else
                            {
                                QMessageBox::critical(nullptr, QApplication::applicationName(), QObject::tr("Datenbank konnte nicht wiederhergestellt werden."));
                            }
                          #endif
                        }
                        else
                        {
                            fileUpdate.remove();
                            QMessageBox::critical(nullptr, QApplication::applicationName(), QObject::tr("Aktualisierung fehlgeschlagen.\n\n") + bh->lastError());
                        }
                    }
                    else
                    {
                        QMessageBox::critical(nullptr, QApplication::applicationName(), QObject::tr("Sicherheitskopie konnte nicht erstellt werden."));
                    }
                }
            }
            else
            {
                break;
            }
        }
        else if (!gSettings->databasePath().isEmpty())
        {
            QMessageBox::critical(nullptr, QApplication::applicationName(),
                                  QObject::tr("Die Datenbank \"%1\" konnte nicht geöffnet werden.").arg(bh->databasePath()));
        }

        // choose other database
        if (!chooseDatabase())
        {
            bh->disconnectDatabase();
            break;
        }
    }
    return bh->isConnectedDatabase();
}

static QByteArray getHash(const QString &fileName)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    hash.addData(file.readAll());
    return hash.result();
}

static void saveResourcesHash()
{
    gSettings->beginGroup("ResourcesHashes");
    QDirIterator it(":/data", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        if (it.fileName() == "kb_daten.sqlite")
            continue;
        if (it.fileInfo().isDir())
            continue;
        QByteArray hash = getHash(it.filePath());
        gSettings->setValue(it.filePath(), hash);
    }
    gSettings->endGroup();
}

static QByteArray getPreviousHash(const QString &fileName)
{
    return gSettings->valueInGroup("ResourcesHashes", fileName).toByteArray();
}

static void copyResources()
{
    bool update = gSettings->isNewProgramVersion();

    // create data directory
    QString dataDir = gSettings->dataDir(0);
    if (!QDir(dataDir).exists())
    {
        if (!QDir().mkpath(dataDir))
            QMessageBox::critical(nullptr, QApplication::applicationName(),
                                  QObject::tr("Der Ordner \"%1\" konnte nicht erstellt werden.").arg(dataDir));
    }

    // copy resources
    QDirIterator it(":/data", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        if (it.fileName() == "kb_daten.sqlite")
            continue;

        // create directory
        QString filePath = dataDir + it.filePath().mid(7);
        if (it.fileInfo().isDir())
        {
            QDir().mkpath(filePath);
            continue;
        }

        // copy file
        QFile fileResource(it.filePath());
        QFile fileLocal(filePath);
        if (!fileLocal.exists())
        {
            if (fileResource.copy(fileLocal.fileName()))
                fileLocal.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
            else
                QMessageBox::critical(nullptr, QApplication::applicationName(),
                                      QObject::tr("Der Datei \"%1\" konnte nicht erstellt werden.").arg(fileLocal.fileName()));
        }
        else if (update)
        {
            QByteArray hashLocal = getHash(fileLocal.fileName());
            if (hashLocal != getHash(it.filePath()))
            {
                QMessageBox::StandardButton ret = QMessageBox::Yes;
                if (hashLocal != getPreviousHash(it.filePath()))
                {
                    ret = QMessageBox::question(nullptr, QApplication::applicationName(),
                                                QObject::tr("Die Ressourcendatei \"%1\" ist verschieden von der lokalen Datei.\n"
                                                            "Die Datei wurde entweder manuell editiert oder durch ein Update verändert.\n\n"
                                                            "Soll die lokale Datei ersetzt werden?").arg(it.fileName()));
                }
                if (ret == QMessageBox::Yes)
                {
                    fileLocal.remove();
                    if (fileResource.copy(fileLocal.fileName()))
                        fileLocal.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
                    else
                        QMessageBox::critical(nullptr, QApplication::applicationName(),
                                              QObject::tr("Der Datei \"%1\" konnte nicht erstellt werden.").arg(fileLocal.fileName()));
                }
            }
        }
    }

    // store hashes for next update
    if (update)
    {
        saveResourcesHash();
    }
}

static void checkWebView()
{
  #if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
    if (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows &&
        QOperatingSystemVersion::current() <= QOperatingSystemVersion::Windows7)
    {
        if (gSettings->isNewProgramVersion())
        {
                int ret = QMessageBox::warning(nullptr, QApplication::applicationName(),
                                               QObject::tr("Unter Umständen stürzt das Programm unter Windows 7 ab!\n") +
                                               QObject::tr("Sollen Sudinformationen und Spickzettel/Zusammenfassung deaktiviert werden?"),
                                               QMessageBox::Yes | QMessageBox::No,
                                               QMessageBox::Yes);
                gSettings->setValueInGroup("General", "WebViewEnabled", ret == QMessageBox::No);
        }
    }
    WebView::setSupported(gSettings->valueInGroup("General", "WebViewEnabled", true).toBool());
  #endif
}

static void checkSSL()
{
  #if QT_NETWORK_LIB
    if (!QSslSocket::supportsSsl())
    {
        QString buildVersion = QSslSocket::sslLibraryBuildVersionString();
        QString rutimeVersion = QSslSocket::sslLibraryVersionString();
        qWarning() << "SSL not supported";
        qWarning() << "Version needed:" << buildVersion;
        qWarning() << "Version installed:" << rutimeVersion;
        if (gSettings->isNewProgramVersion())
        {
            QMessageBox::warning(nullptr, QApplication::applicationName(),
                                 QObject::tr("SSL wird nicht unterstüzt.\nVersion benötigt: %1\nVersion installiert: %2").arg(buildVersion, rutimeVersion));
        }
    }
  #endif
}

static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    defaultMessageHandler(type, context, msg);
    QString entry;
    if (logFile || DlgConsole::Dialog)
    {
        QTextStream out(&entry);
        switch (type)
        {
        case QtDebugMsg:
            out << "DEBUG | ";
            break;
        case QtInfoMsg:
            out << "INFO  | ";
            break;
        case QtWarningMsg:
            out << "WARN  | ";
            break;
        case QtCriticalMsg:
            out << "ERROR | ";
            break;
        case QtFatalMsg:
            out << "FATAL | ";
            break;
        }
        out << QDateTime::currentDateTime().toString("dd.MM.yy hh::mm::ss.zzz") << " | ";
        out << context.category << " | " << msg;
    }
    if (logFile)
    {
        if (!logFile->isOpen())
            logFile->open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream out(logFile);
        out << entry;
        if (context.file)
            out << " | " << context.file << ":" << context.line << ", " << context.function;
      #if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
        out << Qt::endl;
      #else
        out << endl;
      #endif
    }
    if (DlgConsole::Dialog)
    {
        DlgConsole::Dialog->append(entry);
    }
}

static void installTranslator(QApplication &a, QTranslator &translator, const QString &filename)
{
    QLocale locale(gSettings->language());
    a.removeTranslator(&translator);
    if (translator.load(locale, filename, "_", a.applicationDirPath() + "/translations"))
        a.installTranslator(&translator);
    else if (translator.load(locale, filename, "_", ":/translations"))
        a.installTranslator(&translator);
    else if (translator.load(locale, filename, "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        a.installTranslator(&translator);
}

int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;

    // parse arguments
  #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    bool highDpi = true;
    QString scale_factor;
    for (int i = 1; i < argc; i++)
    {
      QString arg(argv[i]);
      arg = arg.toLower();
      while (arg[0] == '-')
          arg.remove(0,1);
      QStringList list({"qt_auto_screen_scale_factor=0",
                        "qt_auto_screen_scale_factor=false",
                        "qt_auto_screen_scale_factor=off"});
      if (list.contains(arg))
          highDpi = false;
      if (arg.startsWith("qt_scale_factor="))
          scale_factor = arg.mid(16);
    }
    if (highDpi)
    {
      #if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
      #endif
        QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    }
    if (!scale_factor.isEmpty())
    {
        qputenv("QT_SCALE_FACTOR", scale_factor.toStdString().c_str());
    }
  #endif

  #if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
  #endif

    QApplication a(argc, argv);

    // set application name, organization and version
    a.setOrganizationName(QString(ORGANIZATION));
    a.setApplicationName(QString(TARGET));
    a.setApplicationVersion(QString(VERSION));

    // load settings
    if (QFile::exists(a.applicationDirPath() + "/portable"))
        gSettings = new Settings(QCoreApplication::applicationDirPath());
    else
        gSettings = new Settings();

    // logging
    defaultMessageHandler = qInstallMessageHandler(messageHandler);
    int logLevel = gSettings->logLevel();
    if (logLevel > 100)
        logFile = new QFile(gSettings->settingsDir() + "/logfile.txt");
    gSettings->initLogLevel(logLevel);

    // modules
    gSettings->initModules();

    qInfo("--- Application start ---");
    qInfo() << "Version:" << QCoreApplication::applicationVersion();
    if (logLevel > 0)
        qInfo() << "Log level:" << logLevel;

    // language
    QTranslator translatorQt, translatorKbh;
    installTranslator(a, translatorQt, "qtbase");
    installTranslator(a, translatorKbh, "kbh");

    // locale
    bool useLanguageLocale = gSettings->valueInGroup("General", "UseLanguageLocale", false).toBool();
    if (useLanguageLocale)
        QLocale::setDefault(QLocale(gSettings->language()));

    // do some checks
    checkSSL();
    checkWebView();

    try
    {
        // copy resources
        copyResources();

        // run application
        do
        {
            if (connectDatabase())
            {
                MainWindow w(nullptr);
                a.setStyle(QStyleFactory::create(gSettings->style()));
                a.setPalette(gSettings->palette);
                a.setFont(gSettings->font);
                w.show();
                try
                {
                    ret = a.exec();
                }
                catch (const std::exception& ex)
                {
                    qCritical() << "Program error:" << ex.what();
                    QMessageBox::critical(nullptr, QObject::tr("Programmfehler"), ex.what());
                    ret = EXIT_FAILURE;
                }
                catch (...)
                {
                    qCritical() << "Program error: unknown";
                    QMessageBox::critical(nullptr, QObject::tr("Programmfehler"), QObject::tr("Unbekannter Fehler."));
                    ret = EXIT_FAILURE;
                }
                if (ret == 1001)
                {
                    installTranslator(a, translatorQt, "qtbase");
                    installTranslator(a, translatorKbh, "kbh");
                    bool useLanguageLocale = gSettings->valueInGroup("General", "UseLanguageLocale", false).toBool();
                    QLocale::setDefault(useLanguageLocale ? QLocale(gSettings->language()) : QLocale::system());
                    ret = 1000;
                }
            }
            else
            {
                ret = EXIT_FAILURE;
            }
        }
        while(ret == 1000);
    }
    catch (const std::exception& ex)
    {
        qCritical() << "Program error:" << ex.what();
        QMessageBox::critical(nullptr, QObject::tr("Programmfehler"), ex.what());
        ret = EXIT_FAILURE;
    }
    catch (...)
    {
        qCritical() << "Program error: unknown";
        QMessageBox::critical(nullptr, QObject::tr("Programmfehler"), QObject::tr("Unbekannter Fehler."));
        ret = EXIT_FAILURE;
    }

    // clean up
    if (bh)
        delete bh;
    delete gSettings;

    qInfo("--- Application end (%d)---", ret);

    // close log
    if (logFile)
    {
        logFile->close();
        delete logFile;
    }

    return ret;
}
