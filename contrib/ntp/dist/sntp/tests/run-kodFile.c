/*	$NetBSD: run-kodFile.c,v 1.2 2018/04/07 00:19:53 christos Exp $	*/

/* AUTOGENERATED FILE. DO NOT EDIT. */

//=======Test Runner Used To Run Each Test Below=====
#define RUN_TEST(TestFunc, TestLineNum) \
{ \
  Unity.CurrentTestName = #TestFunc; \
  Unity.CurrentTestLineNumber = TestLineNum; \
  Unity.NumberOfTests++; \
  if (TEST_PROTECT()) \
  { \
      setUp(); \
      TestFunc(); \
  } \
  if (TEST_PROTECT() && !TEST_IS_IGNORED) \
  { \
    tearDown(); \
  } \
  UnityConcludeTest(); \
}

//=======Automagically Detected Files To Include=====
#include "unity.h"
#include <setjmp.h>
#include <stdio.h>
#include "config.h"
#include "ntp_types.h"
#include "ntp_stdlib.h"
#include "fileHandlingTest.h"
#include "kod_management.h"

//=======External Functions This Runner Calls=====
extern void setUp(void);
extern void tearDown(void);
extern void test_ReadEmptyFile(void);
extern void test_ReadCorrectFile(void);
extern void test_ReadFileWithBlankLines(void);
extern void test_WriteEmptyFile(void);
extern void test_WriteFileWithSingleEntry(void);
extern void test_WriteFileWithMultipleEntries(void);


//=======Suite Setup=====
static void suite_setup(void)
{
extern int change_logfile(const char*, int);
change_logfile("stderr", 0);
}

//=======Test Reset Option=====
void resetTest(void);
void resetTest(void)
{
  tearDown();
  setUp();
}

char const *progname;


//=======MAIN=====
int main(int argc, char *argv[])
{
  progname = argv[0];
  suite_setup();
  UnityBegin("kodFile.c");
  RUN_TEST(test_ReadEmptyFile, 19);
  RUN_TEST(test_ReadCorrectFile, 20);
  RUN_TEST(test_ReadFileWithBlankLines, 21);
  RUN_TEST(test_WriteEmptyFile, 22);
  RUN_TEST(test_WriteFileWithSingleEntry, 23);
  RUN_TEST(test_WriteFileWithMultipleEntries, 24);

  return (UnityEnd());
}
