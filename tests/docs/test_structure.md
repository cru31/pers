# Test Structure Documentation

## Terminology

- **TestHandler**: A class that handles one type of test
- **TestType**: A specific functionality to test (e.g., "Instance Creation")
- **TestVariation**: Individual test case with different options within a TestType
- **TestOption**: Parameters that can be changed during test execution
- **TestCase**: Actual execution unit = TestType + specific TestVariation

## JSON Structure

```json
{
  "testCases": [
    {
      "id": 1,
      "category": "Instance Management",
      "testType": "Instance Creation",
      "variation": {
        "name": "Validation Enabled",
        "options": {
          "enableValidation": true,
          "applicationName": "Test App",
          "engineName": "Pers Engine"
        }
      },
      "expectedResult": "Valid instance created",
      "actualResult": "",
      "status": "",
      "failureReason": ""
    }
  ]
}
```

## Handler Structure

Each TestType has one Handler class that:
1. Receives a TestVariation through execute()
2. Applies the options from the variation
3. Runs the test with those options
4. Returns result based on expected behavior for that variation

## Example Handler Implementation

```cpp
class InstanceCreationHandler : public ITestHandler {
private:
    // All possible variations for this test type
    struct Variation {
        std::string name;
        bool enableValidation;
        std::string appName;
        std::string engineName;
        std::string expectedResult;
    };
    
    static const std::vector<Variation> VARIATIONS;
    
public:
    bool execute(const TestVariation& variation,
                std::string& actualResult,
                std::string& failureReason) {
        // Apply options from variation
        InstanceDesc desc;
        desc.enableValidation = variation.options["enableValidation"];
        desc.applicationName = variation.options["applicationName"];
        desc.engineName = variation.options["engineName"];
        
        // Execute test
        auto instance = factory->createInstance(desc);
        
        // Check result
        if (instance) {
            actualResult = variation.expectedResult;
            return true;
        }
        
        actualResult = "Failed";
        failureReason = "Instance creation failed";
        return false;
    }
};
```