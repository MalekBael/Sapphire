import re
import os
from collections import defaultdict

def parse_enum(file_path):
    if not os.path.isfile(file_path):
        print(f"Error: File not found at '{file_path}'")
        return None

    with open(file_path, 'r', encoding='utf-8') as file:
        content = file.read()

    # Extract the enum block for BNpcName
    enum_match = re.search(r'enum\s+BNpcName\s*\{([^}]+)\}', content, re.MULTILINE | re.DOTALL)
    if not enum_match:
        print("Error: 'BNpcName' enum not found in the file.")
        return None

    enum_content = enum_match.group(1)

    # Find all enum entries (name and value)
    entries = re.findall(r'(\w+)\s*=\s*(\d+)', enum_content)
    return entries

def find_duplicates(entries):
    name_count = defaultdict(int)
    value_count = defaultdict(list)

    for name, value in entries:
        name_count[name] += 1
        value_count[value].append(name)

    duplicate_names = [name for name, count in name_count.items() if count > 1]
    duplicate_values = {value: names for value, names in value_count.items() if len(names) > 1}

    return duplicate_names, duplicate_values

def main():
    # Determine the directory where the script resides
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Construct the path to Common.BNpc.h relative to the script's directory
    file_path = os.path.join(script_dir, 'Common.BNpc.h')

    entries = parse_enum(file_path)

    if not entries:
        return

    duplicate_names, duplicate_values = find_duplicates(entries)

    if duplicate_names:
        print("Duplicate Enumerator Names found:")
        for name in duplicate_names:
            print(f" - {name}")
    else:
        print("No duplicate enumerator names found.")

    if duplicate_values:
        print("\nDuplicate Enumerator Values found:")
        for value, names in duplicate_values.items():
            print(f"Value {value} is assigned to: {', '.join(names)}")
    else:
        print("No duplicate enumerator values found.")

if __name__ == "__main__":
    main()
