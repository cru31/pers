#pragma once

#include "test_handlers.h"

namespace pers::tests::json {

class BufferCreationHandler : public ITestHandler {
public:
    std::string getTestType() const override;
    bool execute(const JsonTestCase& testCase, 
                std::string& actualResult, 
                std::string& failureReason) override;
};

class BufferSpecificTestHandler : public ITestHandler {
public:
    std::string getTestType() const override;
    bool canHandle(const JsonTestCase& testCase) const override;
    bool execute(const JsonTestCase& testCase, 
                std::string& actualResult, 
                std::string& failureReason) override;
};

} // namespace pers::tests::json