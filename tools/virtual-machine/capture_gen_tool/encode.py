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
import msgpack
from pathlib import Path
import argparse
import xml.etree.ElementTree as ET
from datetime import datetime
# ===== Setting =====
START_TIMESTAMP = 1749046034167305728  # in nanoseconds
INTERVAL_NS = 100_000

RAW_INDEX_FILE = "raw_index.dat"
CHANNEL_DIR = "channel_0x00000000"
PROPERTIES_FILE = os.path.join(CHANNEL_DIR, "properties.dat")
start_sequence_number = 0
record_type_enum = 0
# ===== Property initial setting =====
frame_rate_data = {"num": 10, "denom": 1}
image_property_data = {
    "width": 1920,
    "height": 1080,
    "stride_bytes": 1920,
    "pixel_format": "image_rgb24"
}
tensor_shapes_data = {
    "tensor_count": 4,
    "shapes_array": [2, 10, 4, 1, 10, 1, 10, 1, 1] + [0] * 183
}

def parse_args():
    parser = argparse.ArgumentParser(description="generate AITRIOS-compatible output tensor data")
    parser.add_argument("--output-dir", required=True, help="Output directory base path")
    parser.add_argument("--output-tensor", help="Directory containing Ch0 (input_tensor) binary files")
    return parser.parse_args()

def generate_raw_index_and_properties(raw_index_path, properties_path, output_tensor_dir):
    if not os.path.exists(output_tensor_dir):
        print(f"Directory '{output_tensor_dir}' does not exist. Skipping generation.")
        return

    files = sorted(
        f for f in os.listdir(output_tensor_dir)
        if os.path.isfile(os.path.join(output_tensor_dir, f))
    )

    if not files:
        print(f"Directory '{output_tensor_dir}' is empty. Skipping generation.")
        return

    if not files:
        print(f"output_tensor_dir '{output_tensor_dir}' is empty. Skipping raw_index/properties generation.")
        return

    sequence_number = start_sequence_number
    timestamp = START_TIMESTAMP
    count = 0
    os.makedirs(os.path.dirname(properties_path), exist_ok=True)

    with open(raw_index_path, "wb") as raw_f, open(properties_path, "wb") as prop_f:
        raw_packer = msgpack.Packer()
        prop_packer = msgpack.Packer()

        for _ in files:
            # raw_index.dat entry
            raw_record = {
                "sequence_number": sequence_number,
                "channel_id": 0,
                "caputured_timestamp": timestamp,
                "sent_time": timestamp,
                "record_type": record_type_enum,
                "rawdata": b""
            }
            raw_f.write(msgpack.packb(raw_record, use_bin_type=True))

            # properties.dat entry
            prop_record = {
                "sequence_number": sequence_number,
                "properties": {
                    "frame_rate_property": {
                        "data": msgpack.packb(frame_rate_data, use_bin_type=True)
                    },
                    "image_property": {
                        "data": msgpack.packb(image_property_data, use_bin_type=True)
                    },
                    "tensor_shapes_property": {
                        "data": msgpack.packb(tensor_shapes_data, use_bin_type=True)
                    }
                }
            }
            prop_f.write(prop_packer.pack(prop_record))

            sequence_number += 1
            timestamp += INTERVAL_NS
            count += 1

    print(f"✔ Generated {count} records in:")
    print(f"  - {raw_index_path}")
    print(f"  - {properties_path}")

def generate_verified_properties(output_dir):
    output_dir = Path(output_dir) / "properties"
    output_dir.mkdir(parents=True, exist_ok=True)

    properties_data = {
        "LibcameraAccessProperty": {},
        "LibcameraDeviceEnumerationProperty": {},
        "ai_model_bundle_id_property": {"model_bundle_id": "000000"},
        "camera_image_flip_property": {"flip_horizontal": False, "flip_vertical": False},
        "image_crop_property": {"x": 0, "y": 0, "width": 640, "height": 480},
        "image_property": {"width": 416, "height": 320, "stride_bytes": 1248, "pixel_format": "image_rgb24"},
        "image_rotation_property": {"rotation": 0},
        "image_sensor_function_supported_property": {"ae": True, "awb": True, "af": False},
        "info_string_property": {"category": 0, "info": "IMX500"},
        "input_data_type_property": {"count": 0, "channels": [0] * 8}
    }

    for filename, content in properties_data.items():
        path = output_dir / filename
        with open(path, "wb") as f:
            f.write(msgpack.packb(content, use_bin_type=True))

def indent(elem, level=0):
    i = "\n" + level * "  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        for child in elem:
            indent(child, level + 1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i

def generate_info_xml(output_dir):
    record = ET.Element("record", date=datetime.now().strftime("%Y/%m/%d %H:%M:%S"))
    stream = ET.SubElement(record, "stream", key="inference_stream", type="image")
    ET.SubElement(stream, "framerate", num=str(2997), denom=str(1000))
    ET.SubElement(stream, "skipframe", rate="1")
    props = ET.SubElement(stream, "properties")
    for key in [
        "LibcameraAccessProperty",
        "LibcameraDeviceEnumerationProperty",
        "ai_model_bundle_id_property",
        "camera_image_flip_property",
        "image_crop_property",
        "image_property",
        "image_rotation_property",
        "image_sensor_function_supported_property",
        "info_string_property",
        "input_data_type_property"
    ]:
        ET.SubElement(props, "property", key=key)
    channels = ET.SubElement(record, "channels")
    ET.SubElement(channels, "channel", id="0", type="meta_data", description="meta")

    indent(record)

    tree = ET.ElementTree(record)
    tree.write(Path(output_dir) / "info.xml", encoding="utf-8", xml_declaration=True)


if __name__ == "__main__":
    args = parse_args()
    raw_index_path = Path(args.output_dir) / RAW_INDEX_FILE
    properties_path = Path(args.output_dir) / PROPERTIES_FILE
    if args.output_tensor:
       output_tensor_dir = Path(args.output_tensor)
       generate_raw_index_and_properties(raw_index_path, properties_path, output_tensor_dir)
       generate_info_xml(args.output_dir)
       generate_verified_properties(args.output_dir)
       print(f"Saving output tensor to: {output_tensor_dir}")
    else:
       print("output_tensor not specified, skipping.")
