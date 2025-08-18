# AITRIOS SDK Edge App Development Documentation

## Overview

This documentation provides comprehensive guidance for developing edge applications using the AITRIOS SDK.

## Quick Start

Quickly understand Edge App development using an object detection sample.

1. Git clone
    ```bash
    git clone https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app.git
    cd aitrios-sdk-edge-app
    ```
2. Get submodules
    ```bash
    git submodule update --init --recursive
    ```
3. Build the sample **Edge Application** for object detection 
    ```bash
    make CMAKE_FLAGS="-DAPPS_SELECTION=detection"
    ```
    The `edge_app.wasm` file will be generated under the `./bin` directory.

4. Install senscord-libcamera to run **Edge Application** 

    === "Ubuntu 22.04 (Jammy/amd64)"
        ```bash
        wget http://midokura.github.io/debian/evp-archive-keyring_jammy_amd64.deb
        sudo dpkg -i ./evp-archive-keyring_jammy_amd64.deb
        sudo apt update
        sudo apt install senscord-libcamera
        ```

    === "Ubuntu 20.04 (Focal/amd64)"
        ```bash
        wget http://midokura.github.io/debian/evp-archive-keyring_focal_amd64.deb
        sudo dpkg -i ./evp-archive-keyring_focal_amd64.deb
        sudo apt update
        sudo apt install senscord-libcamera
        ```

    === "Raspberry Pi (Bookworm/aarch64)"
        ```bash
        wget http://midokura.github.io/debian/evp-archive-keyring_bookworm_arm64.deb
        sudo dpkg -i ./evp-archive-keyring_bookworm_arm64.deb
        sudo apt update
        sudo apt install senscord-libcamera
        ```

5. Run **Edge Application**  

    === "Without AI camera"
        ```bash
        /opt/senscord/run_iwasm.sh -d pc ./bin/edge_app.wasm
        ```

    === "With AI camera"
        ```bash
        /opt/senscord/run_iwasm.sh ./bin/edge_app.wasm
        ```

    After running the above command, **Edge Application** will be waiting to receive the Configuration.

6. Start **Edge Application** by sending Configuration

    Please open a separate terminal.  
    Install netcat.
    ```bash
    sudo apt update && sudo apt install -y netcat-traditional
    ```

    Send Configuration.
    ```bash
    ./tools/send_configuration.sh --process_state=2
    ```
    !!! note "Note: When Edge Application is running with `process_state=2`"
        Directories named `image` and `inference` will be created under the directory where the Edge Application is running. The input tensors will be saved in the `image` directory, and the output tensors will be saved in the `inference` directory until `process_state` becomes `1` or the **Edge Application** is destroyed.        

    ??? info "The logs of Edge Application"
        If the Configuration is sent correctly to **Edge Application**, the log of **Edge Application** will be displayed in the terminal where `edge_app.wasm` is running.


7. Stop **Edge Application** by sending Configuration

    ```bash
    ./tools/send_configuration.sh --process_state=1
    ```

    ??? info "Modify Configuration?"
        You can check the contents of the Configuration sent to **Edge Application** from the terminal output after executing `send_configuration.sh`. If you want to change the contents of the Configuration, please modify the contents of `send_configuration.sh`.  

8. Check the result  
    Check directories named `image` and `inference` will be created under the directory where the Edge Application is running.

    You can check with `draw.py` to display the latest result.
    !!! note "`draw.py` supports only object detection"

    Create the virtual environment.
    ```bash
    python3 -m venv venv
    ```
    Activate the virtual environment.
    ```bash
    source venv/bin/activate
    ```

    Install OpenCV.
    ```bash
    pip install opencv-python
    ```

    Execute `draw.py`, specifying the directory where the **Edge Application** is running as the first argument.
    ```bash
    python3 /opt/senscord/draw.py .
    ```
    Then you can view live output as follows.  
    ???+ example "Detection sample"  
        <div style="text-align:center"><video src="assets/videos/sample_detection_person_cup.mp4" autoplay loop muted playsinline style="max-width:100%;"></video></div>


9. Destroy **Edge Application**  

    Send the empty Configuration to destroy **Edge Application**.
    ```bash
    ./tools/send_configuration.sh d
    ```


## License

Please refer the latest [LICENSE](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/blob/main/LICENSE).
