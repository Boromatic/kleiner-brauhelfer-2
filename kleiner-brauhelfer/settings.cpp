#include "settings.h"
#include <QGuiApplication>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

Settings::Settings(QObject *parent) :
    QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName(), parent)
{
    initTheme();
}

Settings::Settings(const QString& dir, QObject *parent) :
    QSettings(dir + "/" + QCoreApplication::applicationName() + ".ini", QSettings::IniFormat, parent)
{
    initTheme();
}

Settings::~Settings()
{
    beginGroup("General");
    setValue("version", QCoreApplication::applicationVersion());
    endGroup();
}

void Settings::initTheme()
{
    beginGroup("Style");

    font = value("Font", QFont()).value<QFont>();
    mTheme = static_cast<Theme>(value("Theme", System).toInt());

    switch (mTheme)
    {
    case System:
        palette = QGuiApplication::palette();
        break;

    case Bright:
        palette.setColorGroup(QPalette::Disabled,
            Qt::darkGray,//windowText
            QColor(226,226,226),//button
            QColor(240,240,240),//light
            QColor(200,200,200),//dark
            QColor(210,210,210),//mid
            Qt::darkGray,//text
            QColor(240,240,240),//bright_text
            QColor(255,255,255),//base
            QColor(226,226,226));//window
        palette.setColorGroup(QPalette::Active,
            QColor(40,40,40),//windowText
            QColor(226,226,226),//button
            QColor(240,240,240),//light
            QColor(200,200,200),//dark
            QColor(210,210,210),//mid
            QColor(40,40,40),//text
            QColor(240,240,240),//bright_text
            QColor(255,255,255),//base
            QColor(226,226,226));//window
        palette.setColorGroup(QPalette::Inactive,
            QColor(40,40,40),//windowText
            QColor(226,226,226),//button
            QColor(240,240,240),//light
            QColor(200,200,200),//dark
            QColor(210,210,210),//mid
            QColor(40,40,40),//text
            QColor(240,240,240),//bright_text
            QColor(255,255,255),//base
            QColor(226,226,226));//window
        palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
        palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        palette.setColor(QPalette::Link, QColor(0, 120, 215));
        palette.setColor(QPalette::LinkVisited, QColor(0, 120, 215));
        break;

    case Dark:
        palette.setColorGroup(QPalette::Disabled,
            QColor(120, 120, 120),//windowText
            QColor(53, 53, 53),//button
            QColor(120, 120, 120),//light
            QColor(25, 25, 25),//dark
            QColor(32, 32, 32),//mid
            QColor(120, 120, 120),//text
            QColor(120, 120, 120),//bright_text
            QColor(25, 25, 25),//base
            QColor(53, 53, 53));//window
        palette.setColorGroup(QPalette::Active,
            QColor(255, 255, 255),//windowText
            QColor(53, 53, 53),//button
            QColor(120, 120, 120),//light
            QColor(25, 25, 25),//dark
            QColor(32, 32, 32),//mid
            QColor(255, 255, 255),//text
            QColor(120, 120, 120),//bright_text
            QColor(25, 25, 25),//base
            QColor(53, 53, 53));//window
        palette.setColorGroup(QPalette::Inactive,
            QColor(255, 255, 255),//windowText
            QColor(53, 53, 53),//button
            QColor(120, 120, 120),//light
            QColor(25, 25, 25),//dark
            QColor(32, 32, 32),//mid
            QColor(255, 255, 255),//text
            QColor(120, 120, 120),//bright_text
            QColor(25, 25, 25),//base
            QColor(53, 53, 53));//window
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::LinkVisited, QColor(42, 130, 218));
        break;
    }

    if (palette.color(QPalette::Active, QPalette::Text).lightness() < 127)
    {
        ErrorBase = QColor(220, 127, 127);
        WarningText = QColor(219, 137, 9);
        ErrorText = QColor(180, 0, 0);

        MekrlisteBackground = QColor(122, 163, 233);
        VerbrauchtBackground = QColor(180, 180, 180);
        AbgefuelltBackground = QColor(193, 225, 178);
        GebrautBackground = QColor(225, 216, 184);
        NichtGebrautBackground = QColor(245, 245, 245);

        HopfenTypBackgrounds = {QColor(), QColor(255,200,170), QColor(200,255,200), QColor(200,200,255)};
        HefeTypOgUgBackgrounds = {QColor(), QColor(255,200,170), QColor(200,200,255)};
        HefeTypTrFlBackgrounds = {QColor(), QColor(255,200,170), QColor(200,200,255)};
        WZTypBackgrounds = {QColor(253, 199, 197),
                            QColor(239, 207, 175),
                            QColor(241, 216, 167),
                            QColor(184, 223, 178),
                            QColor(159, 226, 201),
                            QColor(154, 225, 227),
                            QColor(176, 218, 246),
                            QColor(210, 208, 252),
                            QColor(239, 200, 243),
                            QColor(254, 196, 223)};

        DiagramLinie1 = QColor(116, 30, 166);
        DiagramLinie2 = QColor(56, 104, 2);
        DiagramLinie3 = QColor(170, 0, 0);
        DiagramLinie1Light = QColor(151, 125, 166);
        DiagramLinie2Light = QColor(141, 154, 127);
        DiagramLinie3Light = QColor(170, 100, 100);
    }
    else
    {
        ErrorBase = QColor(170, 100, 100);
        WarningText = QColor(219, 137, 9);
        ErrorText = QColor(238, 71, 71);

        MekrlisteBackground = QColor(81, 139, 232);
        VerbrauchtBackground = QColor(110, 110, 110);
        AbgefuelltBackground = QColor(99, 125, 21);
        GebrautBackground = QColor(125, 99, 21);
        NichtGebrautBackground = QColor(53, 53, 53);

        HopfenTypBackgrounds = {QColor(), QColor(255,150,94), QColor(141,239,141), QColor(162,162,242)};
        HefeTypOgUgBackgrounds = {QColor(), QColor(255,150,94), QColor(162,162,242)};
        HefeTypTrFlBackgrounds = {QColor(), QColor(255,150,94), QColor(162,162,242)};
        WZTypBackgrounds = {QColor(220, 114, 107),
                            QColor(193, 133, 0),
                            QColor(149, 151, 0),
                            QColor(70, 164, 35),
                            QColor(0, 172, 119),
                            QColor(0, 171, 176),
                            QColor(0, 159, 216),
                            QColor(138, 134, 230),
                            QColor(203, 107, 213),
                            QColor(226, 100, 169)};

        DiagramLinie1 = QColor(165, 94, 209);
        DiagramLinie2 = QColor(76, 140, 2);
        DiagramLinie3 = QColor(175, 45, 45);
        DiagramLinie1Light = QColor(151, 125, 166);
        DiagramLinie2Light = QColor(120, 153, 84);
        DiagramLinie3Light = QColor(170, 100, 100);
    }

    paletteInput = palette;

    paletteError = palette;
    paletteError.setColor(QPalette::Base, ErrorBase);

    paletteErrorButton = palette;
    paletteErrorButton.setColor(QPalette::Button, ErrorBase);

    paletteErrorLabel = palette;
    paletteErrorLabel.setColor(QPalette::WindowText, ErrorText);

    endGroup();
}

int Settings::logLevel()
{
    beginGroup("General");
    int level = value("LogLevel", "0").toInt();
    endGroup();
    return level;
}

void Settings::setLogLevel(int level)
{
    beginGroup("General");
    setValue("LogLevel", level);
    endGroup();
    initLogLevel(level);
}

void Settings::initLogLevel(int level)
{
    QString rules;
    while (level > 100)
        level -= 100;
    if (level <= 0)
        rules = "*.info=false\n*.debug=false";
    else if (level == 1)
        rules = "*.debug=false";
    else if (level == 2)
        rules = "";
    else if (level == 3)
        rules = "SqlTableModel.info=true";
    else
        rules = "SqlTableModel.info=true\nSqlTableModel.debug=true";
    QLoggingCategory::setFilterRules(rules);
}

Settings::Theme Settings::theme() const
{
    return mTheme;
}

void Settings::setTheme(Theme theme)
{
    beginGroup("Style");
    setValue("Theme", theme);
    endGroup();
    initTheme();
}

QString Settings::style()
{
  #if 0
    beginGroup("Style");
    QString style = value("Style", "Fusion").toString();
    endGroup();
    return style;
  #else
    return "Fusion";
  #endif
}

void Settings::setStyle(const QString &style)
{
    beginGroup("Style");
    setValue("Style", style);
    endGroup();
}

bool Settings::useSystemFont()
{
    beginGroup("Style");
    bool system = value("UseSystemFont", true).toBool();
    endGroup();
    return system;
}

void Settings::setUseSystemFont(bool system)
{
    beginGroup("Style");
    setValue("UseSystemFont", system);
    endGroup();
}

void Settings::setFont(const QFont &font)
{
    beginGroup("Style");
    setValue("Font", font);
    endGroup();
    this->font = font;
}

bool Settings::animationsEnabled()
{
    beginGroup("General");
    int enabled = value("Animations", true).toBool();
    endGroup();
    return enabled;
}

void Settings::setAnimationsEnabled(bool enabled)
{
    beginGroup("General");
    setValue("Animations", enabled);
    endGroup();
}

QString Settings::language()
{
    beginGroup("General");
    QString lang = value("language", "de").toString();
    endGroup();
    return lang;
}

void Settings::setLanguage(QString lang)
{
    beginGroup("General");
    setValue("language", lang);
    endGroup();
}

QString Settings::settingsDir() const
{
    return QFileInfo(fileName()).absolutePath() + "/";
}

QString Settings::databasePath()
{
    QString path;
    beginGroup("General");
    path = value("database").toString();
    endGroup();
    if (!path.isEmpty() && QDir::isRelativePath(path))
    {
        QDir dir(settingsDir());
        path = QDir::cleanPath(dir.filePath(path));
    }

    return path;
}

void Settings::setDatabasePath(const QString& path)
{
    QDir dir(settingsDir());
    QString pathRel = dir.relativeFilePath(path);
    beginGroup("General");
    if (pathRel.startsWith(".."))
        setValue("database", path);
    else
        setValue("database", pathRel);
    endGroup();
}

QString Settings::databaseDir()
{
    return QFileInfo(databasePath()).absolutePath() + "/";
}

QString Settings::dataDir(int type) const
{
    switch (type)
    {
    case 1:
        return settingsDir() + "data/Webview/";
    case 2:
        return settingsDir() + "data/Rohstoffe/";
    case 3:
        return settingsDir() + "data/Etiketten/";
    default:
        return settingsDir() + "data/";
    }
}

QString Settings::lastProgramVersion()
{
    QString version;
    beginGroup("General");
    version = value("version").toString();
    endGroup();
    return version;
}

bool Settings::isNewProgramVersion()
{
    return lastProgramVersion() != QCoreApplication::applicationVersion();
}
