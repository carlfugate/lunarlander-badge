import re
import os

# Define the paths to the files
ota_hpp_path = 'include/QA/ota.hpp'
version_file_path = '.pio/build/esp32-s3/version'

# Read the content of ota.hpp
with open(ota_hpp_path, 'r') as ota_hpp_file:
    ota_hpp_content = ota_hpp_file.read()

# Extract the VERSION value using a regular expression
version_pattern = re.compile(r'#define VERSION (\d+)')
match = version_pattern.search(ota_hpp_content)
if match:
    version_value = match.group(1)

    # Ensure the directory exists
    os.makedirs(os.path.dirname(version_file_path), exist_ok=True)

    # Write the VERSION value to the version file
    with open(version_file_path, 'w') as version_file:
        version_file.write(version_value)

    print(f'Written VERSION {version_value} to {version_file_path}')
else:
    print('VERSION not found in ota.hpp')