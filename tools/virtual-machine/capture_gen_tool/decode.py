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

import msgpack

def decode_properties(filepath):
    print(f"\n=== Decoding: {filepath} ===")
    with open(filepath, "rb") as f:
        unpacker = msgpack.Unpacker(f, raw=False)
        for record in unpacker:
            print(f"sequence_number: {record['sequence_number']}")

            props = record.get("properties", {})

            # frame_rate_property
            if "frame_rate_property" in props:
                raw = props["frame_rate_property"]["data"]
                decoded = msgpack.unpackb(raw, raw=False)
                print("  frame_rate_property:", decoded)

            # image_property
            if "image_property" in props:
                raw = props["image_property"]["data"]
                decoded = msgpack.unpackb(raw, raw=False)
                print("  image_property:")
                print(f"    width: {decoded['width']}")
                print(f"    height: {decoded['height']}")
                print(f"    stride_bytes: {decoded['stride_bytes']}")
                print(f"    pixel_format: {decoded['pixel_format']}")

            # tensor_shapes_property
            if "tensor_shapes_property" in props:
                raw = props["tensor_shapes_property"]["data"]
                decoded = msgpack.unpackb(raw, raw=False)
                print("  tensor_shapes_property:")
                print(f"    tensor_count: {decoded['tensor_count']}")
                print(f"    shapes_array (first 8): {decoded['shapes_array'][:8]}")

            print("---")


def decode_raw_index(filepath):
    print(f"\n=== Decoding: {filepath} ===")
    with open(filepath, "rb") as f:
        unpacker = msgpack.Unpacker(f, raw=False)
        for record in unpacker:
            print(f"sequence_number: {record['sequence_number']}")
            print(f"channel_id: {record['channel_id']}")
            print(f"caputured_timestamp: {record['caputured_timestamp']}")
            print(f"sent_time: {record['sent_time']}")
            print(f"record_type: {record['record_type']}")
            print("---")


if __name__ == "__main__":
    decode_properties("channel_0x00000000/properties.dat")
    decode_raw_index("raw_index.dat")
