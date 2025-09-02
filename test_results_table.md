# Test Results Table

Generated: 2025-09-03 00:08:30

## Summary

- **Total Tests**: 40
- **Passed**: 24
- **Failed**: 16
- **Skipped**: 0
- **Pass Rate**: 60.0%
- **Total Time**: 5332.22ms

## adapter_request

| ID | Test Type | Input | Expected Result | Actual Result | Expected Callstack | Pass/Fail | Failure Reason |
|----|-----------|-------|-----------------|---------------|-------------------|-----------|----------------|
| 0009 | Adapter Request | type=OptionBased, power_preference=Default, force_fallback=true, compatible_surface=null, request_adapter_options=Default | Success with options | No adapter found | - | ??FAIL | Failed to request physical device |
| 0010 | Adapter Request | type=OptionBased, power_preference=HighPerformance, force_fallback=false, compatible_surface=mock_surface, request_adapter_options=Unknown | Success with options | At least 1 adapter found | - | ??PASS |  |
| 0011 | Adapter Request | type=OptionBased, power_preference=LowPower, force_fallback=true, compatible_surface=null, request_adapter_options=IntegratedGPU | Success with options | No adapter found | - | ??FAIL | Failed to request physical device |
| 0012 | Adapter Request | type=OptionBased, power_preference=LowPower, force_fallback=true, compatible_surface=null, request_adapter_options=Default | Success with options | No adapter found | - | ??FAIL | Failed to request physical device |
| 0013 | Adapter Request | type=OptionBased, power_preference=HighPerformance, force_fallback=true, compatible_surface=null, request_adapter_options=Default | Success with options | No adapter found | - | ??FAIL | Failed to request physical device |
| 0014 | Adapter Request | type=OptionBased, power_preference=Default, force_fallback=false, compatible_surface=null, request_adapter_options=Default | Success with options | At least 1 adapter found | - | ??PASS |  |
| 0015 | Adapter Request | type=OptionBased, power_preference=Default, force_fallback=true, compatible_surface=mock_surface, request_adapter_options=Default | Success with options | No adapter found | - | ??FAIL | Failed to request physical device |
| 0016 | Adapter Request | type=OptionBased, power_preference=Default, force_fallback=true, compatible_surface=null, request_adapter_options=DiscreteGPU | Success with options | No adapter found | - | ??FAIL | Failed to request physical device |
| 0017 | Adapter Request | type=OptionBased, power_preference=Default, force_fallback=true, compatible_surface=null, request_adapter_options=IntegratedGPU | Success with options | No adapter found | - | ??FAIL | Failed to request physical device |
| 0018 | Adapter Request | type=OptionBased, power_preference=Default, force_fallback=true, compatible_surface=null, request_adapter_options=CPU | Success with options | No adapter found | - | ??FAIL | Failed to request physical device |
| 0019 | Adapter Request | type=OptionBased, power_preference=Default, force_fallback=true, compatible_surface=null, request_adapter_options=Unknown | Success with options | No adapter found | - | ??FAIL | Failed to request physical device |

## buffer_creation

| ID | Test Type | Input | Expected Result | Actual Result | Expected Callstack | Pass/Fail | Failure Reason |
|----|-----------|-------|-----------------|---------------|-------------------|-----------|----------------|
| 0037 | Buffer Creation | type=OptionBased, label=Test Buffer, size=0, usage=Vertex, mapped_at_creation=false | Returns nullptr | Returns nullptr | - | ??PASS |  |
| 0038 | Buffer Creation | type=OptionBased, label=Test Buffer, size=256, usage=Vertex, mapped_at_creation=false | Success with options | Success with options | - | ??PASS |  |
| 0039 | Buffer Creation | type=OptionBased, label=Test Buffer, size=65536, usage=Vertex, mapped_at_creation=false | Success with options | Success with options | - | ??PASS |  |
| 0040 | Buffer Creation | type=OptionBased, label=Test Buffer, size=268435456, usage=Vertex, mapped_at_creation=false | Success with options | Success with options | - | ??PASS |  |

## command_encoder

| ID | Test Type | Input | Expected Result | Actual Result | Expected Callstack | Pass/Fail | Failure Reason |
|----|-----------|-------|-----------------|---------------|-------------------|-----------|----------------|
| 0033 | Command Encoder Creation | type=OptionBased, encoder_label=Main Encoder, encoder_type=Compute | Success with options | Valid encoder created | - | ??PASS |  |
| 0034 | Command Encoder Creation | type=OptionBased, encoder_label=, encoder_type=Render | Success with options | Valid encoder created | - | ??PASS |  |
| 0035 | Command Encoder Creation | type=OptionBased, encoder_label=Frame_0, encoder_type=Bundle | Success with options | Valid encoder created | - | ??PASS |  |
| 0036 | Command Encoder Creation | type=OptionBased, encoder_label=Shadow Pass, encoder_type=General | Success with options | Valid encoder created | - | ??PASS |  |

## device_creation

| ID | Test Type | Input | Expected Result | Actual Result | Expected Callstack | Pass/Fail | Failure Reason |
|----|-----------|-------|-----------------|---------------|-------------------|-----------|----------------|
| 0020 | Device Creation | type=OptionBased, required_limits=, required_features=[], default_queue_desc=, debug_name=Test Device | Success with options | Valid device created | - | ??PASS |  |

## instance_creation

| ID | Test Type | Input | Expected Result | Actual Result | Expected Callstack | Pass/Fail | Failure Reason |
|----|-----------|-------|-----------------|---------------|-------------------|-----------|----------------|
| 0001 | WebGPU Instance Creation | type=OptionBased, application_name=Unit Test, validation=true, debug_mode=true, backend_type=Vulkan, power_preference=Default, force_fallback=true | Valid with validation | Valid instance created | - | ??FAIL |  |
| 0002 | WebGPU Instance Creation | type=OptionBased, application_name=Unit Test, validation=false, debug_mode=true, backend_type=Vulkan, power_preference=Default, force_fallback=true | Success with options | Valid instance created | - | ??PASS |  |
| 0003 | WebGPU Instance Creation | type=OptionBased, application_name=Unit Test, validation=true, debug_mode=true, backend_type=D3D12, power_preference=Default, force_fallback=true | Valid with validation | Valid instance created | - | ??FAIL |  |
| 0004 | WebGPU Instance Creation | type=OptionBased, application_name=Unit Test, validation=true, debug_mode=true, backend_type=Metal, power_preference=Default, force_fallback=true | Valid with validation | Valid instance created | - | ??FAIL |  |
| 0005 | WebGPU Instance Creation | type=OptionBased, application_name=Unit Test, validation=true, debug_mode=true, backend_type=Primary, power_preference=Default, force_fallback=true | Valid with validation | Valid instance created | - | ??FAIL |  |
| 0006 | WebGPU Instance Creation | type=OptionBased, application_name=Unit Test, validation=true, debug_mode=true, backend_type=Secondary, power_preference=Default, force_fallback=true | Valid with validation | Valid instance created | - | ??FAIL |  |
| 0007 | WebGPU Instance Creation | type=OptionBased, application_name=Unit Test, validation=true, debug_mode=true, backend_type=Fallback, power_preference=Default, force_fallback=true | Valid with validation | Valid instance created | - | ??FAIL |  |
| 0008 | WebGPU Instance Creation | type=OptionBased, application_name=Unit Test, validation=true, debug_mode=true, backend_type=All, power_preference=Default, force_fallback=true | Valid with validation | Valid instance created | - | ??FAIL |  |

## queue_creation

| ID | Test Type | Input | Expected Result | Actual Result | Expected Callstack | Pass/Fail | Failure Reason |
|----|-----------|-------|-----------------|---------------|-------------------|-----------|----------------|
| 0021 | Queue Creation | type=OptionBased, queue_label=Default, queue_priority=Default, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0022 | Queue Creation | type=OptionBased, queue_label=Very_Long_Queue_Name_12345, queue_priority=Low, multiple_queues=true | Success with options | Valid queue created | - | ??PASS |  |
| 0023 | Queue Creation | type=OptionBased, queue_label=Graphics, queue_priority=Normal, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0024 | Queue Creation | type=OptionBased, queue_label=Compute, queue_priority=Default, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0025 | Queue Creation | type=OptionBased, queue_label=Transfer, queue_priority=Default, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0026 | Queue Creation | type=OptionBased, queue_label=Graphics, queue_priority=Default, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0027 | Queue Creation | type=OptionBased, queue_label=, queue_priority=Default, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0028 | Queue Creation | type=OptionBased, queue_label=Very_Long_Queue_Name_12345, queue_priority=Default, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0029 | Queue Creation | type=OptionBased, queue_label=Default, queue_priority=High, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0030 | Queue Creation | type=OptionBased, queue_label=Default, queue_priority=Normal, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0031 | Queue Creation | type=OptionBased, queue_label=Default, queue_priority=Low, multiple_queues=false | Success with options | Valid queue created | - | ??PASS |  |
| 0032 | Queue Creation | type=OptionBased, queue_label=Default, queue_priority=Default, multiple_queues=true | Success with options | Valid queue created | - | ??PASS |  |

## Execution Times

| ID | Test Type | Time (ms) |
|----|-----------|----------|
| 0001 | WebGPU Instance Creation | 290.79 |
| 0002 | WebGPU Instance Creation | 128.00 |
| 0003 | WebGPU Instance Creation | 131.19 |
| 0004 | WebGPU Instance Creation | 134.91 |
| 0005 | WebGPU Instance Creation | 135.82 |
| 0006 | WebGPU Instance Creation | 136.90 |
| 0007 | WebGPU Instance Creation | 130.79 |
| 0008 | WebGPU Instance Creation | 135.99 |
| 0009 | Adapter Request | 143.06 |
| 0010 | Adapter Request | 139.98 |
| 0011 | Adapter Request | 140.33 |
| 0012 | Adapter Request | 131.92 |
| 0013 | Adapter Request | 132.27 |
| 0014 | Adapter Request | 131.15 |
| 0015 | Adapter Request | 142.07 |
| 0016 | Adapter Request | 133.04 |
| 0017 | Adapter Request | 131.53 |
| 0018 | Adapter Request | 128.93 |
| 0019 | Adapter Request | 140.75 |
| 0020 | Device Creation | 232.38 |
| 0021 | Queue Creation | 112.42 |
| 0022 | Queue Creation | 113.10 |
| 0023 | Queue Creation | 111.93 |
| 0024 | Queue Creation | 112.13 |
| 0025 | Queue Creation | 117.17 |
| 0026 | Queue Creation | 116.91 |
| 0027 | Queue Creation | 117.22 |
| 0028 | Queue Creation | 108.61 |
| 0029 | Queue Creation | 110.95 |
| 0030 | Queue Creation | 126.33 |
| 0031 | Queue Creation | 120.51 |
| 0032 | Queue Creation | 126.31 |
| 0033 | Command Encoder Creation | 109.62 |
| 0034 | Command Encoder Creation | 121.28 |
| 0035 | Command Encoder Creation | 127.62 |
| 0036 | Command Encoder Creation | 123.79 |
| 0037 | Buffer Creation | 118.78 |
| 0038 | Buffer Creation | 131.50 |
| 0039 | Buffer Creation | 120.36 |
| 0040 | Buffer Creation | 133.89 |
