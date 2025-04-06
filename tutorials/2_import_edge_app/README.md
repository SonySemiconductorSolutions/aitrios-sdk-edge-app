# Importing an "**Edge Application**"

This tutorial shows you how to import an "**Edge Application**" to "**Console for AITRIOS**".

## 1. Package an "**Edge Application**"

You need to package the "**Edge Application**" to import it to "**Console for AITRIOS**".

1. Prepare files required for packaging

The following three files are required:
- "**Edge Application**" (.wasm)<br>
Prepare an application file. Check the [README](../1_develop_edge_app/README.md) to learn how to develop an app.
- DTDL (.json)<br>
Prepare a DTDL file.
- **`manifest.json`**<br>
Prepare manifest file. Refer to the following for how to create it.

The parameters required for **`manifest.json`** are :

- **`manifest_version`**<br>
The version of manifest file.
- **`edge_app`**
  - **`app_name`**<br>
  The name of "**Edge Application**".
  - **`file_name`**<br>
  The file name of "**Edge Application**".
  - **`version`**<br>
  The version of "**Edge Application**".
  - **`app_dtdl_file`**<br>
  The file name of DTDL.
  - **`compiled_flg`**<br>
  Specify `false`.

**`manifest.json`** sample is the following:
```json
{
  "manifest_version": "1.0.0",
  "edge_app": {
    "app_name": "edge_app",
    "file_name": "edge_app.wasm",
    "version": "1.0.0",
    "app_dtdl_file": "edge_app_interface.json",
    "referred_dtdl_files": [],
    "compiled_flg": false,
    "schema_info": ""
  }
}
```

> **NOTE**
>
> **`referred_dtdl_files`** and **`schema_info`** are not used. Please specify them as an empty array and an empty string respectively, like the preceding sample.

2. Create a folder with any name and store the these three files ("**Edge Application**", DTDL and **`manifest.json`**) in the folder

```
edgeapp_pkg
├── edge_app.wasm            (Edge Application)
├── edge_app_interface.json  (DTDL)
└── manifest.json
```

3. Compress the folder into a zip file

Now the packaging is completed.


## 2. Import an "**Edge Application**"

You can import an "**Edge Application**" using the functions provided by "**Console for AITRIOS**".

See ["**Console User Manual**"](https://developer.aitrios.sony-semicon.com/en/edge-ai-sensing/documents/console-v2/console-user-manual/) for details.
