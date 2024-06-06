/*	$NetBSD: run-t-ntp_scanner.c,v 1.2 2018/04/07 00:19:54 christos Exp $	*/

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

//=======External Functions This Runner Calls=====
extern void setUp(void);
extern void tearDown(void);
extern void test_keywordIncorrectToken(void);
extern void test_keywordServerToken(void);
extern void test_DropUninitializedStack(void);
extern void test_IncorrectlyInitializeLexStack(void);
extern void test_InitializeLexStack(void);
extern void test_PopEmptyStack(void);
extern void test_IsInteger(void);
extern void test_IsUint(void);
extern void test_IsDouble(void);
extern void test_SpecialSymbols(void);
extern void test_EOC(void);


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
  UnityBegin("t-ntp_scanner.c");
  RUN_TEST(test_keywordIncorrectToken, 8);
  RUN_TEST(test_keywordServerToken, 16);
  RUN_TEST(test_DropUninitializedStack, 24);
  RUN_TEST(test_IncorrectlyInitializeLexStack, 30);
  RUN_TEST(test_InitializeLexStack, 38);
  RUN_TEST(test_PopEmptyStack, 49);
  RUN_TEST(test_IsInteger, 57);
  RUN_TEST(test_IsUint, 76);
  RUN_TEST(test_IsDouble, 90);
  RUN_TEST(test_SpecialSymbols, 104);
  RUN_TEST(test_EOC, 115);

  return (UnityEnd());
}
