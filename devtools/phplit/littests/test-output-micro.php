<?php
# RUN: %{phplit} -j 1 -v %{inputs}/test-data-micro --output %t.results.out
# RUN: filechecker < %t.results.out %s


# CHECK: {
# CHECK:    "__version__":
# CHECK:    "elapsed"
# CHECK-NEXT:    "tests": [
# CHECK-NEXT:        {
# CHECK-NEXT:            "name": "test-data-micro :: micro-tests.ini:test{{[0-2]}}",
# CHECK-NEXT:            "code": "PASS",
# CHECK-NEXT:            "output": "",
# CHECK-NEXT:            "elapsed": 0,
# CHECK-NEXT:            "metrics": {
# CHECK-NEXT:                "micro_value0": 4,
# CHECK-NEXT:                "micro_value1": 1.3
# CHECK-NEXT:            }
# CHECK-NEXT:        },
# CHECK-NEXT:        {
# CHECK-NEXT:           "name": "test-data-micro :: micro-tests.ini:test{{[0-2]}}",
# CHECK-NEXT:            "code": "PASS",
# CHECK-NEXT:            "output": "",
# CHECK-NEXT:            "elapsed": 0,
# CHECK-NEXT:            "metrics": {
# CHECK-NEXT:                "micro_value0": 4,
# CHECK-NEXT:                "micro_value1": 1.3
# CHECK-NEXT:            }
# CHECK-NEXT:        },
# CHECK-NEXT:        {
# CHECK-NEXT:           "name": "test-data-micro :: micro-tests.ini:test{{[0-2]}}",
# CHECK-NEXT:            "code": "PASS",
# CHECK-NEXT:            "output": "",
# CHECK-NEXT:            "elapsed": 0,
# CHECK-NEXT:            "metrics": {
# CHECK-NEXT:                "micro_value0": 4,
# CHECK-NEXT:                "micro_value1": 1.3
# CHECK-NEXT:            }
# CHECK-NEXT:        },
# CHECK-NEXT:        {
# CHECK-NEXT:            "name": "test-data-micro :: micro-tests.ini",
# CHECK-NEXT:            "code": "PASS",
# CHECK-NEXT:            "output": "Test passed.",
# CHECK-NEXT:            "elapsed": {{[0-9.]+}},
# CHECK-NEXT:            "metrics": {
# CHECK-NEXT:                "value0": 1,
# CHECK-NEXT:                "value1": 2.3456
# CHECK-NEXT:            }
# CHECK-NEXT:        }
# CHECK-NEXT:    ]
# CHECK-NEXT: }

