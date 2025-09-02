#!/usr/bin/env python3
"""
Generate expanded test cases from option schema
"""

import json
import itertools
import os
from pathlib import Path
from typing import Dict, List, Any
import hashlib

def load_schema(schema_path: str) -> Dict:
    """Load test option schema"""
    with open(schema_path, 'r', encoding='utf-8') as f:
        return json.load(f)

def generate_option_combinations(category_config: Dict, strategy: str = "pairwise") -> List[Dict]:
    """Generate option combinations based on strategy"""
    dimensions = category_config["option_dimensions"]
    
    if strategy == "exhaustive":
        # Generate all possible combinations
        keys = list(dimensions.keys())
        value_lists = [dimensions[k]["values"] for k in keys]
        
        combinations = []
        for values in itertools.product(*value_lists):
            combo = dict(zip(keys, values))
            combinations.append(combo)
        return combinations
    
    elif strategy == "boundary":
        # Test boundary values - first, last, and one middle value
        combinations = []
        keys = list(dimensions.keys())
        
        # First values
        first_combo = {k: dimensions[k]["values"][0] for k in keys}
        combinations.append(first_combo)
        
        # Last values
        last_combo = {k: dimensions[k]["values"][-1] for k in keys}
        combinations.append(last_combo)
        
        # Middle values (if available)
        middle_combo = {}
        for k in keys:
            values = dimensions[k]["values"]
            if len(values) > 2:
                middle_combo[k] = values[len(values)//2]
            else:
                middle_combo[k] = values[0]
        combinations.append(middle_combo)
        
        # One option at a time variation (keeping others at default)
        for key in keys:
            for value in dimensions[key]["values"]:
                combo = {k: dimensions[k]["values"][0] for k in keys}
                combo[key] = value
                if combo not in combinations:
                    combinations.append(combo)
        
        return combinations
    
    elif strategy == "critical_path":
        # Test most important combinations
        combinations = []
        keys = list(dimensions.keys())
        
        # Default configuration
        default_combo = {k: dimensions[k]["values"][0] for k in keys}
        combinations.append(default_combo)
        
        # Most common configurations
        if "validation" in dimensions:
            # Test with validation on and off
            for val in dimensions["validation"]["values"]:
                combo = default_combo.copy()
                combo["validation"] = val
                if combo not in combinations:
                    combinations.append(combo)
        
        if "size" in dimensions:
            # Test critical sizes
            critical_sizes = [0, 256, 65536, 268435456]
            for size in critical_sizes:
                if size in dimensions["size"]["values"]:
                    combo = default_combo.copy()
                    combo["size"] = size
                    if combo not in combinations:
                        combinations.append(combo)
        
        if "backend_type" in dimensions:
            # Test each backend
            for backend in dimensions["backend_type"]["values"]:
                combo = default_combo.copy()
                combo["backend_type"] = backend
                if combo not in combinations:
                    combinations.append(combo)
        
        return combinations
    
    else:  # pairwise or default
        # Simplified pairwise - ensure each pair of option values appears at least once
        # This is a simplified implementation - real pairwise testing tools are more sophisticated
        combinations = []
        keys = list(dimensions.keys())
        
        # Start with some base combinations
        for i in range(min(10, 2 ** len(keys))):  # Limit to reasonable number
            combo = {}
            for j, key in enumerate(keys):
                values = dimensions[key]["values"]
                # Rotate through values
                combo[key] = values[(i + j) % len(values)]
            combinations.append(combo)
        
        return combinations

def generate_test_case(base_type: str, option_combo: Dict, test_id: int, category: str) -> Dict:
    """Generate a single test case from option combination"""
    
    # Create unique ID based on options
    option_str = json.dumps(option_combo, sort_keys=True)
    option_hash = hashlib.md5(option_str.encode()).hexdigest()[:8]
    
    test_case = {
        "id": f"{test_id:04d}",
        "category": category,
        "test_type": base_type,
        "option_set_id": option_hash,
        "input": {
            "type": "OptionBased",
            "options": option_combo
        },
        "expected_result": "Success with options",
        "timeout_ms": 1000,
        "enabled": True,
        "metadata": {
            "generated": True,
            "option_hash": option_hash
        }
    }
    
    # Adjust expected results based on specific options
    if option_combo.get("size") == 0:
        test_case["expected_result"] = "Returns nullptr"
    elif option_combo.get("validation") == True:
        test_case["expected_result"] = "Valid with validation"
    elif option_combo.get("backend_type") == "Fallback":
        test_case["expected_result"] = "Success with fallback"
    
    return test_case

def save_test_cases(test_cases: List[Dict], output_dir: str, category: str):
    """Save test cases to JSON files in category directory"""
    category_dir = Path(output_dir) / category
    category_dir.mkdir(parents=True, exist_ok=True)
    
    # Group by option sets (batch of 10)
    for i in range(0, len(test_cases), 10):
        batch = test_cases[i:i+10]
        batch_file = category_dir / f"option_set_{i//10:03d}.json"
        
        output = {
            "metadata": {
                "category": category,
                "batch": i//10,
                "count": len(batch)
            },
            "test_cases": batch
        }
        
        with open(batch_file, 'w', encoding='utf-8') as f:
            json.dump(output, f, indent=2, ensure_ascii=False)
        
        print(f"Saved {len(batch)} test cases to {batch_file}")

def main():
    # Load schema
    schema_path = "test_case_data/20250102/test_option_schema.json"
    schema = load_schema(schema_path)
    
    output_base = "test_case_data/20250102"
    
    # Test strategies to use for each category
    category_strategies = {
        "instance_creation": "critical_path",
        "adapter_request": "boundary",
        "device_creation": "critical_path", 
        "queue_creation": "boundary",
        "command_encoder": "pairwise",
        "buffer_creation": "critical_path"
    }
    
    all_test_cases = []
    test_id = 1
    
    for category, strategy in category_strategies.items():
        if category not in schema["test_categories"]:
            continue
            
        config = schema["test_categories"][category]
        print(f"\nGenerating test cases for {category} using {strategy} strategy...")
        
        # Generate option combinations
        combinations = generate_option_combinations(config, strategy)
        print(f"  Generated {len(combinations)} option combinations")
        
        # Generate test cases
        category_tests = []
        for combo in combinations:
            test_case = generate_test_case(
                config["base_type"], 
                combo, 
                test_id,
                category
            )
            category_tests.append(test_case)
            all_test_cases.append(test_case)
            test_id += 1
        
        # Save to files
        save_test_cases(category_tests, output_base, category)
    
    # Save master test list
    master_file = Path(output_base) / "all_generated_tests.json"
    master_output = {
        "metadata": {
            "version": "2.0.0",
            "date": "2025-01-02",
            "total_tests": len(all_test_cases),
            "generation_strategy": "option_based"
        },
        "test_cases": all_test_cases
    }
    
    with open(master_file, 'w', encoding='utf-8') as f:
        json.dump(master_output, f, indent=2, ensure_ascii=False)
    
    print(f"\nTotal test cases generated: {len(all_test_cases)}")
    print(f"Master list saved to: {master_file}")

if __name__ == "__main__":
    main()