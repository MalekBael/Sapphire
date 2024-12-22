import re

def format_enum_entries(input_file, output_file):
    enum_start = False
    processed_lines = []
    duplicates = {}
    japanese_char_pattern = re.compile(r'[\u3040-\u30FF\u4E00-\u9FFF]')

    # Pattern to match lines with "name number"
    enum_entry_pattern = re.compile(r'^\s*([^\s].*?)\s+(\d+)\s*$')

    with open(input_file, 'r', encoding='utf-8') as f:
        for line in f:
            if 'enum BNpcName' in line or 'enum class BNpcName' in line:
                enum_start = True
                processed_lines.append(line)
                continue

            if enum_start:
                if '};' in line:
                    enum_start = False
                    processed_lines.append(line)
                    continue

                match = enum_entry_pattern.match(line)
                if match:
                    name, number = match.groups()

                    # Check for empty name or number
                    if not name or not number:
                        processed_lines.append(f'    // {line.strip()}\n')
                        continue

                    # Clean the name to be a valid C++ identifier
                    clean_name = re.sub(r'[^\w]', '_', name.strip()).lower()

                    # Prepend an underscore if the name starts with a number or contains Japanese characters
                    if re.match(r'^\d', clean_name) or japanese_char_pattern.search(name):
                        clean_name = f'_{clean_name}'

                    # Handle duplicates
                    if clean_name in duplicates:
                        duplicates[clean_name] += 1
                        clean_name = f"{clean_name}_duplicate{duplicates[clean_name]}"
                    else:
                        duplicates[clean_name] = 1

                    # Format the line
                    new_line = f'    {clean_name} = {number},\n'
                    processed_lines.append(new_line)
                else:
                    # Comment out lines that do not match the pattern
                    processed_lines.append(f'    // {line.strip()}\n')
            else:
                processed_lines.append(line)

    with open(output_file, 'w', encoding='utf-8') as f:
        f.writelines(processed_lines)

    print(f"Formatted enumeration has been saved to {output_file}")

if __name__ == "__main__":
    input_file = 'src/world/Actor/Common.BNpc.h'  # Update this path if necessary
    output_file = 'BNpcName_fixed.h'
    format_enum_entries(input_file, output_file)
