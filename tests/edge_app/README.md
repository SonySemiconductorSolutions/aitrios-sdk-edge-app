# Edge App testing

This build instructions are intended to be a a guide to test apps outside the device. For this reason
the libraries present int he device like Senscord, Sess and Evp are mocked depending on the option you want to use.

## Executable app (mock libraries + EVP)

In this case the app is built with a mocked version of the Senscord and Sess libraries as well as EVP which is present when running a WASM
in the agent. This way the obtained executable can be use without the need of the agent in an ELF format.

### Build

To build executable app, run from the top directory of this repo.

```sh
cmake \
    -B build \
    -DCMAKE_INSTALL_PREFIX=/tmp/dist.native \
    -DAPPS_SELECTION=classification \
    -DCMAKE_BUILD_TYPE=Debug \
    . \
&& cmake --build build
```

You can as well use the options `detection` or `segmentation` for the `APPS_SELECTION` flag.

### How-to run

Once you've built the `edge_app` you'll find it in the generated **`build`** folder. You can run it by executing:

```sh
build/edge_app
```

The app will start to run in the IDLE state (1).

In order to pass to another state you should modify the parameter `process_state` inside `common_settings` of the  
[DTDL model JSON](../../../libs/tests/unit/sm/dtdl_model/sample.json) and send it to the sample app with netcat.

Run the following command from **`./rearch`**,

```sh
cat libs/tests/unit/sm/dtdl_model/sample_implemented.json | nc localhost 8080
```

This way you can pass different configuration to the edge_app using the DTDL model.

In order to destroy the app you can use the following command.

```sh
echo "" | nc localhost 8080
```

## WASM app (mock libraries)

In this case the output of the build will be a WASM that doesn't have acces to EVP,
so in order to run it you'll need to deploy it to the agent.

The agent that has to be used is the following [version](https://github.com/midokura/wedge-agent/tree/tbc/wasmcon-config) and replacing the [lib pthread](https://github.com/midokura/wedge-agent/blob/c300529bb8a8d8869dc3c9eb3b0038e2df04ca73/depend.mk#L72) with wasi-threads:

```
-DWAMR_BUILD_LIB_WASI_THREADS=1
```

and using this [.config file](https://github.com/midokura/wedge-agent/blob/tbc/wasmcon-config/configs/wasm.config) when building the agent. In order to build it please follow the instructions
given in the [BUILD.md document.](https://github.com/midokura/wedge-agent/blob/tbc/wasmcon-config/BUILD.md)

### Build

To build WASM app without mocked EVP (needs to be runned in the agent) from **`./build`** directory,

```sh
cmake \
    -DMOCK=1 \
    -DAPPS_SELECTION=classification \
    .. \
&& make -j8 && mv build/edge_app ../../../../bin/edge_app.wasm
```

You can as well use the options `detection` or `segmentation` for the `APPS_SELECTION` flag.

### How-to run

Once you built the WASM you can run it using the local-console.

1. First start the agent, with the following command you will only get the WASM logs:

```sh
local-console start |& grep wasm: --line-buffered | sed 's/.*std\(out\|err\)://'
```

2. You can deploy using `local-console deploy` command from the [rearch](../../../../rearch) folder.

It can happen that there's a timeout error when deploying. Please try more than 1 time to make the deploy or increase
the timeout threshold by using the `--timeout` option. For example if you want to increase the timeout to 50 seconds you can
write the following `local-console deploy -t 50`.
If you already had a previous deployment in the agent before starting you'll need to redeploy it or remove it
by sending and empty deployment `local-console deploy -e` or by reasing all the data from the previous deployment in the folder **`~/.config/wedge`**.

3. After that you can send the DTDL model configuration.

Run the following command from **`./rearch`**,

```sh
local-console config instance edge_app edge_app "$(cat libs/tests/unit/sm/dtdl_model/sample_implemented.json)"
```
