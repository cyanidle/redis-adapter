#ifndef UTILS_SIGNALHANDLER_H
#define UTILS_SIGNALHANDLER_H

#include <QObject>

namespace Utils {

class UnixSignalHandler : public QObject
{
    Q_OBJECT
public:
    explicit UnixSignalHandler(int sig, QObject *parent = nullptr);
signals:
    void caught();
private:
    static void handler(int sig);
    static QHash<int, QPointer<UnixSignalHandler>> m_all;
};

} // namespace Utils

#endif // UTILS_SIGNALHANDLER_H
