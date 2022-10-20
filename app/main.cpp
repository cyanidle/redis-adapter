#include <QCoreApplication>
#include <QThread>
#include "settings-parsing/filereader.h"
#include "redis-adapter/logging.h"
#include "redis-adapter/launcher.h"

void setLoggingFilters(const Settings::LoggingInfo &loggers);

int main (int argc, char **argv) {
    QCoreApplication app(argc, argv);

    qSetMessagePattern(CUSTOM_MESSAGE_PATTERN);

    Settings::FileReader reader("conf/config.toml");
    setLoggingFilters(Settings::LoggingInfoParser::parse(
                          reader.deserialise("log_debug").toMap()));

    auto launcher = new Radapter::Launcher();
    auto initStatus = launcher->initAll();
    if (initStatus < 0) {
        return initStatus;
    }
    launcher->run();
    return app.exec();
}

void setLoggingFilters(const Settings::LoggingInfo &loggers)
{
    auto filterRules = QStringList{
            "default.debug=true",
            QString("%1.debug=true").arg(radapter().categoryName())
    };
    for (auto logInfo = loggers.begin(); logInfo != loggers.end(); logInfo++) {
        auto rule = QString("%1=%2").arg(logInfo.key(), logInfo.value() ? "true" : "false");
        filterRules.append(rule);
    }
    if (!filterRules.isEmpty()) {
        auto filterString = filterRules.join("\n");
        QLoggingCategory::setFilterRules(filterString);
    }
}
