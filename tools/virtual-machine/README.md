# Edge Application Virtual Machine
This is a simple and lightweight VM for running WebAssembly targets. It provides a quick debugger for C/C++ code that is compiled into WebAssembly.

## Features
- Easy wasm app deployment without network, camera firmware.
- One-click execution for compilation, running, and debugging within VS Code.

## VM in docker
The Edge App VM is executed in Docker. When creating an image, Docker installs sensorcord_libcamera.deb, which includes modules that the VM depends on, such as evp-agent, sensorcord, and esf.
#### Build the docker env
```
cd aitrios-sdk-edge-app-dev
make docker_build
```
## The edge app to be launched
You can pass the full path of any wasm file to the docker container and use make to pass the arguments,<br>
The edge_app.wasm must be built with `DEBUG` option, if you want to debug it

## Launch edge app in VM for running
```
cd aitrios-sdk-edge-app-dev
make vm_run edge_app=Path/of/edge_app.wasm
```

## Launch edge app in VM for debug purpose
```
cd aitrios-sdk-edge-app-dev
make vm_debug edge_app=Path/of/edge_app.wasm
```
### Start GUI in VS Code
Load the aitrios-sdk-edge-app-dev in VS Code, go to `Run and Debug` tab, click `wamr-debug` start debug