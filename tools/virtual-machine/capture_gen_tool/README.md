# Capture data generation tool

This toolset generates MessagePack-formatted metadata files (`properties.dat`, `raw_index.dat`) accordingly.
These .dat files contain metadata required to replay data captured from the sensor in the correct format.
As an example, it processes files in the `output_tensor` folder.

---

## Preparation Steps


### 1. Install Required Library (msgpack)

Install the required Python package:

```
pip3 install msgpack
```

> Verified to work with `msgpack` version >= 0.6.1 and < 1.0

---


### 2. Generate `.dat` Files (Encode)

Prepare a folder containing **dummy or actual output tensor files** (e.g., `output_tensor/`).  
Run the script `encode.py` to generate the following output files.

#### Output Structure

```text
<output-dir>/
├── raw_index.dat
├── channel_0x00000000/
│   └── properties.dat
├── properties/
│   ├── ai_model_bundle_id_property
│   ├── camera_image_flip_property
│   └── ... (other property files)
└── info.xml
```

#### Command Example

```bash
python encode.py --output-dir $(pwd) --output-tensor output_tensor
```

#### Arguments

| Argument         | Required | Description |
|------------------|----------|-------------|
| `--output-dir`   | ✅ Yes   | Output directory where all generated files (`raw_index.dat`, `info.xml`, etc.) will be placed. |
| `--output-tensor`| ❌ No    | Directory containing binary files. Only file names are used to determine the number of records. If not specified or empty, `raw_index.dat` and `properties.dat` will not be generated. |

#### Notes

- If `--output-tensor` is omitted or the folder is empty/non-existent, the tool will skip generating `raw_index.dat` and `properties.dat`.
- `info.xml` and the `properties/` folder are always generated.

---- 

### 3. Leave `info.xml` and the `property` folder unchanged

No changes are needed for these files or folders.

---

### 4. Update the `senscord.xml` Configuration

Edit the following file:

`/opt/senscord/share/senscord/config_pc/senscord.xml`

Set the `target_path` argument to the name of the folder containing `info.xml`.

Example:

```xml
<stream key="inference_stream">
  <address instanceName="player_instance.image" type="image" port="0"/>
  <!-- <client instanceName="client_instance.0"/> -->
  <arguments>
    <!-- <argument name="target_path" value="/home/user/YYYYMMDD_hhmmss_streamkey/"/> -->
    <argument name="target_path" value="/home/user/capture_gen"/>
    <argument name="repeat" value="true"/>
    <argument name="start_offset" value="0"/>
    <argument name="count" value="all"/>
    <argument name="speed" value="30"/>
  </arguments>
</stream>
```

---

### 5. Playback

Follow the playback procedure described in the EdgeApp [README](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/blob/main/README.md)

---

## Validation (Decode)

You can validate the contents of `properties.dat` and `raw_index.dat` using `decode.py`.

```
python decode.py
```

### Example Output

```
=== Decoding: Channel_0x000000/properties.dat ===
sequence_number: 161
  frame_rate_property: {'num': 10, 'denom': 1}
  image_property:
    width: 640
    height: 480
    stride_bytes: 1920
    pixel_format: image_rgb24
  tensor_shapes_property:
    tensor_count: 4
 ...

```
