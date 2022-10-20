#include "gtest_localstorage.h"


TEST(LocalStorage, setGetLastStreamid)
{
    LocalStorage::init(nullptr);
    auto inst = LocalStorage::instance();
    inst->setLastStreamId("test:key", "31");
    QString actual = inst->getLastStreamId("test:key");
    EXPECT_EQ(actual, "31");

    inst->setLastStreamId("test:key:second", "32");
    actual = inst->getLastStreamId("test:key:second");
    EXPECT_EQ(actual, "32");

    actual = inst->getStorageName();
    EXPECT_EQ(actual, "localstorage.ini"); // if fails here check default file name

    actual = inst->currentDirectory();
    actual = QDir(QCoreApplication::applicationDirPath()).relativeFilePath(actual);
    EXPECT_EQ(actual, "conf"); // same as above, check default dir name, "conf" may be out of date
}

TEST(LocalStorage, setDirectoryAndName)
{
    auto inst = LocalStorage::instance();
    EXPECT_TRUE(inst->setStorageDirectory("../testfiles/localstorage"));
    QString actual = inst->currentDirectory();
    actual = QDir(QCoreApplication::applicationDirPath()).relativeFilePath(actual);
    EXPECT_EQ(actual, "../testfiles/localstorage");
    actual = inst->getLastStreamId("test:key:new");
    EXPECT_EQ(actual,"42");

    EXPECT_TRUE(inst->setStorageDirectory("conf"));
    actual = inst->currentDirectory();
    actual = QDir(QCoreApplication::applicationDirPath()).relativeFilePath(actual);
    EXPECT_EQ(actual, "conf");
    actual = inst->getLastStreamId("test:key");
    EXPECT_EQ(actual,"31");

    EXPECT_TRUE(inst->setStorageDirectory("../testfiles/localstorage"));
    EXPECT_TRUE(inst->setStorageName("localstorage2.ini"));
    actual = inst->getStorageName();
    EXPECT_EQ(actual, "localstorage2.ini");
    actual = inst->getLastStreamId("test:key:new:2");
    EXPECT_EQ(actual,"422");
    actual = inst->getLastStreamId("test:key:new:nonexistent");
    EXPECT_EQ(actual,"");

    auto currentTime = QDateTime::currentDateTime();
    auto timeString = currentTime.toString(Qt::DateFormat::LocalDate);
    inst->setLastStreamId("current:time", timeString);
    actual = inst->getLastStreamId("current:time");
    EXPECT_EQ(actual,timeString);
    EXPECT_FALSE(inst->setStorageDirectory("../testfiles/localstorage/nonexistent"));
    actual = inst->currentDirectory();
    actual = QDir(QCoreApplication::applicationDirPath()).relativeFilePath(actual);
    EXPECT_EQ(actual,"../testfiles/localstorage");
    actual = inst->getLastStreamId("current:time");
    EXPECT_EQ(actual,timeString);
}

int main(int argc, char *argv[])
{
    QCoreApplication app{argc, argv};
    QTimer::singleShot(0, [&]()
    {
        ::testing::InitGoogleTest(&argc, argv);
        auto testResult = RUN_ALL_TESTS();
        app.exit(testResult);
    });
    return app.exec();
}
