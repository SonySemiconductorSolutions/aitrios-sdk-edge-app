# Synchornize DTDL and State Machine

Synchronising DTDL and code is complex. Currently there is no tool in C/C++ that, given a schema, allows us to validate whether or not a JSON is from that schema. To overcome this issue we can use a different approach: generate JSONs from the schema and use them as input for our tests.

In our case, the tests use `sample.json` as input. In case of an update of the DTDL, an update of that JSON would be necessary.

## Generating a DTDL JSON

To generate a DTDL JSON, execute the following script from `./rearch`:

```sh
python3 libs/tests/unit/sm/dtdl_model/generate_sample_dtdl_json.py \
    --developer docs/dtdl/developer_edge_app_interface.json \
    --root docs/dtdl/edge_app_interface.json \
    --output libs/tests/unit/sm/dtdl_model/sample.json
```

`sample.json` has been generated using this tool.
