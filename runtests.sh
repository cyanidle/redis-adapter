#!/bin/sh
# a copy of https://doc.qt.io/qt-5/linux-deployment.html#creating-the-application-package script
dirname=$PWD
if [ -z "${SHARED_LIBS_DIR}" ]; then
SHARED_LIBS_DIR=$dirname
fi
LD_LIBRARY_PATH=$SHARED_LIBS_DIR
if [ -z "${TESTS_DIRECTORY}" ]; then
TESTS_DIRECTORY=$dirname/tests-bin
fi
export LD_LIBRARY_PATH
echo "Libs directory $LD_LIBRARY_PATH"

echo "Clearing results directory"
ls -d $TESTS_DIRECTORY/../test-results/ | xargs rm
echo "Running tests with $0 script!"
###
for TESTNAME in $(ls $TESTS_DIRECTORY/gtest)
do
    TEST=$TESTS_DIRECTORY/gtest/$TESTNAME
    echo "Running $TESTNAME" && $TEST --gtest_output="xml:$TESTS_DIRECTORY/../test-results/$TESTNAME.xml" || echo "$TESTNAME FAILED!"
done
###
for TESTNAME in $(ls $TESTS_DIRECTORY/qtest)
do
    TEST=$TESTS_DIRECTORY/qtest/$TESTNAME
    echo "Running $TESTNAME" && $TEST -o $TESTS_DIRECTORY/../test-results/$TESTNAME.xml,junitxml|| echo "$TEST FAILED!"
done
echo "Running tests done, generated files are sent to $TESTS_DIRECTORY/../test-results directory"