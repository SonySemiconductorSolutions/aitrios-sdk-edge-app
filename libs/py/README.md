# Edge App Python Bindings Prototype

Python bindings for the Edge App SDK. The package is currently built with a mocked version of the Senscord and EVP libraries. This way the package can be used from Python without the need of the EVP agent in an ELF format.

Optionally, it can also be built with the actual Senscord libraries.

Based on the [Test Edge App](../../sample_apps/tests/edge_app/README.md).

## Build

First, setup a virtual environment:

```sh
python -m venv venv
. venv/bin/activate
```

Then, build and install the Edge App SDK package:

```sh
pip install .
```

>**NOTE**
>
> You need to ensure **Pybind11** is initialized and updated as a submodule. Use the following command.
> ```sh
> git submodule update --init --recursive libs/third_party/pybind11
> ```

## Build with Senscord

Build Senscord:
```sh
git clone git@github.com:midokura/senscord.git
cd senscord
cmake -B build -G Ninja \
    -DSENSCORD_SAMPLE=ON \
    -DSENSCORD_API_C=ON \
    -DSENSCORD_COMPONENT_V4L2=ON \
    -DSENSCORD_LOG_ENABLED=ON \
    -DSENSCORD_LOG_TYPE=CONSOLE \
    -DSENSCORD_STATUS_MESSAGE_ENABLED=ON \
    -DSENSCORD_STATUS_TRACE_ENABLED=ON \
    -DSENSCORD_INSTALL_CONFIG=ON
cmake --build build
cmake --install build --prefix /path/to/senscord-install/
```

Then, provide the Senscord installation path when installing Edge App SDK package:

```sh
SENSCORD_INSTALL_PATH=/path/to/senscord-install/ /path/to/senscord-install/share/senscord/setup_env.sh pip install .
```

Using `setup_env.sh` is required because as part of the built process it generates a Python stub file (`.pyi`) for the bindings. This needs the newly built Python module to be loaded. Without the correct environment, it will not find the Senscord libraries and it will fail to load.

## How-to run

Once you've installed the package, you can access the `edge_app_sdk` module:
```python
import edge_app_sdk
```

A basic edge app implementation can be found in `example/simple_edge_app.py`. Run it with:
```sh
python example/simple_edge_app.py
```

If using Senscord, it needs the correct environment to execute. Use the `setup_env.sh` wrapper in the Senscord installation:
```sh
/path/to/senscord-install/share/senscord/setup_env.sh python example/simple_edge_app.py
```


The app will start to run in the IDLE state (1).

In order to pass to another state you can send a new configuration to the edge app with netcat.

```sh
cat example/configs/run_5_iterations.json | nc localhost 8080
```

This way you can pass different configuration to the edge_app using the DTDL model.

In order to destroy the app you can use the following command.

```sh
echo "" | nc localhost 8080
```
