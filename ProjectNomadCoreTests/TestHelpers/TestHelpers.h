#pragma once

#include "TestLogger.h"
#include "Math/FPVector.h"

// Starting to see why "using namespace" and these includes ain't in the header file... (slowness and finally what "poisoning" namespace means)
// TODO: Replace with forward declaration in header file
using namespace ProjectNomad;

static float toFloat(fp value) {
    return static_cast<float>(value);
}

class TestHelpers {
public:
    static void assertNear(fp expected, fp actual, fp range);
    static void expectNear(fp expected, fp actual, fp range);

    static void assertNear(FPVector expected, FPVector actual, fp range);
    static void expectNear(FPVector expected, FPVector actual, fp range);
    
    static void expectEq(FPVector expected, FPVector actual);

    static void verifyErrorsLogged(TestLogger& logger);
    static void verifyNoErrorsLogged(TestLogger& logger);
};

class BaseSimTest : public ::testing::Test {
protected:
    TestLogger testLogger;
    
    void SetUp() override {}

    void TearDown() override {
        TestHelpers::verifyNoErrorsLogged(testLogger);
    }
};