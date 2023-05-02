#include "pipeoperation.h"
#include <QRegularExpression>

void Radapter::Private::PipeOperation::reset() {
    left.clear();
    right.clear();
    subpipe.clear();
    type = None;
}

void Radapter::Private::PipeOperation::handleSplitter(const QString &splitter) {
    Direction dir = [&]{
        if (splitter == " > ") return Normal;
        else if (splitter == " < ") return Inverted;
        else if (splitter.startsWith(" <=") && splitter.endsWith("=> ")) return Bidirectional;
        else throw std::runtime_error("Invalid pipe splitter: " + splitter.toStdString());
    }();
    if (type != None && (type != dir || type == Bidirectional)) {
        throw std::runtime_error("Conflict in operation direction!\n"
                                 "Probably: worker.1 > *pipe() < worker.2\n"
                                 "(Must be: worker > *pipe() > worker)\n"
                                 "Or: worker.1 <==pipe()==> <==pipe()==> worker.2.\n"
                                 "(Only one bidirectional pipe between workers allowed at once!\n"
                                 "(Must be: worker.1 <==pipe()==pipe()==> worker.2)");
    }
    type = dir;
    if (type == Bidirectional) {
        static QRegularExpression subsplitter("[<=> ]+");
        subpipe = splitter.split(subsplitter, Qt::SkipEmptyParts);
    }
}
