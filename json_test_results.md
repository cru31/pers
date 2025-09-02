# JSON Test Execution Report

Generated: 2025-09-03 00:08:30

## Test File Metadata

- Version: 2.0.0
- Date: 2025-01-02
- Total Tests: 40

## Execution Summary

- Executed: 40
- Passed: 24
- Failed: 16
- Pass Rate: 60.0%
- Total Time: 5332.22ms

## Detailed Results

### adapter_request

| ID | Test Type | Expected | Actual | Time (ms) | Status | Failure Reason |
|----|-----------|----------|--------|-----------|--------|----------------|
| 0009 | Adapter Request | Success with options | No adapter found | 143.06 | ??FAIL | Failed to request physical device |
| 0010 | Adapter Request | Success with options | At least 1 adapter found | 139.98 | ??PASS |  |
| 0011 | Adapter Request | Success with options | No adapter found | 140.33 | ??FAIL | Failed to request physical device |
| 0012 | Adapter Request | Success with options | No adapter found | 131.92 | ??FAIL | Failed to request physical device |
| 0013 | Adapter Request | Success with options | No adapter found | 132.27 | ??FAIL | Failed to request physical device |
| 0014 | Adapter Request | Success with options | At least 1 adapter found | 131.15 | ??PASS |  |
| 0015 | Adapter Request | Success with options | No adapter found | 142.07 | ??FAIL | Failed to request physical device |
| 0016 | Adapter Request | Success with options | No adapter found | 133.04 | ??FAIL | Failed to request physical device |
| 0017 | Adapter Request | Success with options | No adapter found | 131.53 | ??FAIL | Failed to request physical device |
| 0018 | Adapter Request | Success with options | No adapter found | 128.93 | ??FAIL | Failed to request physical device |
| 0019 | Adapter Request | Success with options | No adapter found | 140.75 | ??FAIL | Failed to request physical device |

### buffer_creation

| ID | Test Type | Expected | Actual | Time (ms) | Status | Failure Reason |
|----|-----------|----------|--------|-----------|--------|----------------|
| 0037 | Buffer Creation | Returns nullptr | Returns nullptr | 118.78 | ??PASS |  |
| 0038 | Buffer Creation | Success with options | Success with options | 131.50 | ??PASS |  |
| 0039 | Buffer Creation | Success with options | Success with options | 120.36 | ??PASS |  |
| 0040 | Buffer Creation | Success with options | Success with options | 133.89 | ??PASS |  |

### command_encoder

| ID | Test Type | Expected | Actual | Time (ms) | Status | Failure Reason |
|----|-----------|----------|--------|-----------|--------|----------------|
| 0033 | Command Encoder Creation | Success with options | Valid encoder created | 109.62 | ??PASS |  |
| 0034 | Command Encoder Creation | Success with options | Valid encoder created | 121.28 | ??PASS |  |
| 0035 | Command Encoder Creation | Success with options | Valid encoder created | 127.62 | ??PASS |  |
| 0036 | Command Encoder Creation | Success with options | Valid encoder created | 123.79 | ??PASS |  |

### device_creation

| ID | Test Type | Expected | Actual | Time (ms) | Status | Failure Reason |
|----|-----------|----------|--------|-----------|--------|----------------|
| 0020 | Device Creation | Success with options | Valid device created | 232.38 | ??PASS |  |

### instance_creation

| ID | Test Type | Expected | Actual | Time (ms) | Status | Failure Reason |
|----|-----------|----------|--------|-----------|--------|----------------|
| 0001 | WebGPU Instance Creation | Valid with validation | Valid instance created | 290.79 | ??FAIL |  |
| 0002 | WebGPU Instance Creation | Success with options | Valid instance created | 128.00 | ??PASS |  |
| 0003 | WebGPU Instance Creation | Valid with validation | Valid instance created | 131.19 | ??FAIL |  |
| 0004 | WebGPU Instance Creation | Valid with validation | Valid instance created | 134.91 | ??FAIL |  |
| 0005 | WebGPU Instance Creation | Valid with validation | Valid instance created | 135.82 | ??FAIL |  |
| 0006 | WebGPU Instance Creation | Valid with validation | Valid instance created | 136.90 | ??FAIL |  |
| 0007 | WebGPU Instance Creation | Valid with validation | Valid instance created | 130.79 | ??FAIL |  |
| 0008 | WebGPU Instance Creation | Valid with validation | Valid instance created | 135.99 | ??FAIL |  |

### queue_creation

| ID | Test Type | Expected | Actual | Time (ms) | Status | Failure Reason |
|----|-----------|----------|--------|-----------|--------|----------------|
| 0021 | Queue Creation | Success with options | Valid queue created | 112.42 | ??PASS |  |
| 0022 | Queue Creation | Success with options | Valid queue created | 113.10 | ??PASS |  |
| 0023 | Queue Creation | Success with options | Valid queue created | 111.93 | ??PASS |  |
| 0024 | Queue Creation | Success with options | Valid queue created | 112.13 | ??PASS |  |
| 0025 | Queue Creation | Success with options | Valid queue created | 117.17 | ??PASS |  |
| 0026 | Queue Creation | Success with options | Valid queue created | 116.91 | ??PASS |  |
| 0027 | Queue Creation | Success with options | Valid queue created | 117.22 | ??PASS |  |
| 0028 | Queue Creation | Success with options | Valid queue created | 108.61 | ??PASS |  |
| 0029 | Queue Creation | Success with options | Valid queue created | 110.95 | ??PASS |  |
| 0030 | Queue Creation | Success with options | Valid queue created | 126.33 | ??PASS |  |
| 0031 | Queue Creation | Success with options | Valid queue created | 120.51 | ??PASS |  |
| 0032 | Queue Creation | Success with options | Valid queue created | 126.31 | ??PASS |  |

