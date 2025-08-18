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

import os
import glob

print("Check if the saved Input Tensor and Output Tensor file names match.")

# Get file names (without extension) from inference/*.txt
txt_files = glob.glob('inference/*.txt')
txt_names = sorted(os.path.splitext(os.path.basename(f))[0] for f in txt_files)

# Get file names (without extension) from image/*.jpg
jpg_files = glob.glob('image/*.jpg')
jpg_names = sorted(os.path.splitext(os.path.basename(f))[0] for f in jpg_files)

print('Comparing file names:')
is_mismatched = False
for txt, jpg in zip(txt_names, jpg_names):
    print(f'inference/{txt}.txt <-> image/{jpg}.jpg', end=' ')
    try:
        txt_num = int(txt)
        jpg_num = int(jpg)
        diff = txt_num - jpg_num
        print(f'| Diff (txt - jpg): {diff}', end='')
        if diff == 0:
            print(' (OK)')
        else:
            print('  <-- Error: File name mismatch!')
            is_mismatched = True
    except ValueError:
        print('| Difference: Cannot calculate (failed to convert to integer)')

if is_mismatched:
    raise AssertionError(f'File name mismatch detected between inference and image directories.')
