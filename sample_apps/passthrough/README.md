# Pass through Edge Application Sample

Pass through "**Edge Application**" sample is showing how to get image data from Senscord and send it to the AITRIOS Console. It uses no AI model and performs no postprocessing.

## Initialize process

Since the application has no Custom Parameters the **`onConfigure(char *topic, void *value, int valuesize)`** event function is empty. 

Nevertheless, other parameters can be of use to shift the "**Edge Application**" to a different state. The parameters to configure it are defined in DTDL [**`edge_app_dummy_interface.json`**](./package/edge_app_passthrough_interface.json). For a sample of Configuration, see [**`configuration.json`**](./configuration/configuration.json).

## Analyze process

Due to not using any AI model, no inference data is going o be analyzed, so no data processing functionality is provided for this "**Edge Application**".

## Package "**Edge Application**"

Check [README](../../tutorials/2_import_edge_app/README.md) for how to package an app.

Please refer to the [**`manifest.json`**](./package/manifest.json) for packaging the sample.
