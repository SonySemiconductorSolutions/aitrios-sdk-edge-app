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

import argparse
import json
import string
import random
from typing import Any

developer_edge_app = {}
edge_app = {}

def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-d",
        "--developer",
        required=True,
        help="Path to the developer DTDL file."
    )
    parser.add_argument(
        "-r",
        "--root",
        required=True,
        help="Path to the root DTDL file."
    )
    parser.add_argument(
        "-o",
        "--output",
        default=None,
        help="Path to store the generated JSON output. If not specified, the output will be shown in the terminal."
    )
    parser.add_argument(
        "-s",
        "--seed",
        default=0,
        type=int,
        help="Seed to generate data."
    )
    return parser.parse_args()

def generate_json(schema: dict | str) -> Any:
    if schema == "string":
        letters = string.ascii_lowercase
        return "GENERATED_STRING-" + ''.join(random.choice(letters) for _ in range(5))
    if schema == "boolean":
        return random.random() < 0.5
    if schema == "integer":
        return random.randint(0, 10)
    if schema == "float":
        return 1 / pow(2, random.randint(1, 4))
    if schema == "double":
        return 1 / pow(2, random.randint(1, 4))

    if isinstance(schema, str) and schema.startswith("dtmi"):
        for field in developer_edge_app["schemas"] + edge_app["schemas"]:
            if field["@id"] == schema:
                return generate_json(field)
        raise Exception("Unexpected schema")

    if isinstance(schema, dict):
        if "@type" in schema:
            if schema["@type"] == "Enum":
                enum_values = schema["enumValues"]
                assert len(enum_values) > 0, "Enum with less than 1 possibility"
                enum_value = random.choice(enum_values)
                return enum_value["enumValue"]
            if schema["@type"] == "Object":
                return {field["name"]: generate_json(field["schema"]) for field in schema["fields"]}
            if schema["@type"] == "Map":
                map_key = generate_json(schema["mapKey"]["schema"])
                map_value = generate_json(schema["mapValue"]["schema"])
                return {map_key: map_value}
            if schema["@type"] == "Property":
                return {schema["name"]: generate_json(schema["schema"])}

    raise Exception("Unexpected schema")

def main() -> None:
    args = get_args()
    random.seed(args.seed)
    with open(args.developer, "r") as f:
        developer_edge_app.update(json.load(f))
    with open(args.root, "r") as f:
        edge_app.update(json.load(f))

    assert len(developer_edge_app["contents"]) == 1, "More than 1 root schema"
    sample_dtdl = generate_json(developer_edge_app["contents"][0])

    # Remove higher level key (edge app topic)
    assert len(sample_dtdl.keys()) == 1
    sample_dtdl = sample_dtdl["edge_app"]

    if args.output:
        with open(args.output, "w") as f:
            json.dump(sample_dtdl, f, indent=4)
    else:
        print(json.dumps(sample_dtdl, indent=4))

if __name__ == "__main__":
    main()
