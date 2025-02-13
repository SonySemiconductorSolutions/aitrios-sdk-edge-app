#!/bin/bash
# Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Adds license to C/C++ files.

base_header=\
"# Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the \"License\");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an \"AS IS\" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License."

c_header=$(echo -e "$base_header" | sed 's@#@ *@')
c_header=$(echo -e "/****************************************************************************
$c_header
 ****************************************************************************/")

sh_header=$(echo -e "#!/bin/bash\n$base_header")

c_header_lines="$(echo -e "$c_header" | wc -l)"
base_header_lines="$(echo -e "$base_header" | wc -l)"
sh_header_lines="$(echo -e "$sh_header" | wc -l)"

add_header_if_missing() {
  local header="$1"
  local header_lines="$2"
  local file="$3"

  if [ -f "$file" ]; then
    first_lines=$(head -n "$header_lines" "$file")
    if [ "$first_lines" != "$header" ]; then
      sed -i --quiet '/^#!\/bin\/bash$/!p' "$file"
      echo -e "$header\n" > "$file.tmp"
      chmod "$(stat -c "%a" "$file")" "$file.tmp"
      cat "$file" >> "$file.tmp"
      mv "$file.tmp" "$file"
      echo "Header added to $file"
    else
      echo "Header already exists in $file"
    fi
  else
    echo "File $file not found"
  fi
}

for x in "$@"; do
  name=$(basename "$x")
  file_extension="${name##*.}"
  case "$file_extension" in
    Dockerfile|cmake|txt|Makefile|py)
      add_header_if_missing "$base_header" "$base_header_lines" "$x"
      ;;
    sh)
      add_header_if_missing "$sh_header" "$sh_header_lines" "$x"
      ;;
    c|cpp|h|hpp|fbs)
      add_header_if_missing "$c_header" "$c_header_lines" "$x"
      ;;
    *)
      echo "Unknown extension: $x"
      exit 1
  esac
done
