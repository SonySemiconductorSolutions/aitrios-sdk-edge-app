# SSL Edge Application Sample

The **SSL Edge Application** sample demonstrates how to establish secure SSL/TLS communication with a remote server, perform edge device authentication, and exchange inference data securely. This application includes SSL client functionality for secure communication and SSL server functionality for hosting secure services.

## Initialize process

The application uses Custom Parameters to configure SSL communication settings. These parameters are passed to the **`onConfigure(char *topic, void *value, int valuesize)`** event function as a JSON string inside the **`value`** parameter.

The following parameters are used in the sample **Edge Application**:

### SSL Client Configuration
The SSL client configuration is defined under the **`ssl_client`** section and includes:

- **`server`**: Server connection settings (name, port, base_path)
- **`authentication`**: Edge login and edge info authentication parameters
- **`metadata`**: Service and dataset identifiers
- **`api_endpoints`**: API endpoint paths for various operations

For detailed SSL configuration options, see the [**`edge_app_ssl_interface.json`**](./package/edge_app_ssl_interface.json) DTDL file.

The parameters to configure the application are defined in DTDL [**`edge_app_ssl_interface.json`**](./package/edge_app_ssl_interface.json). For a sample of Configuration, see [**`configuration.json`**](./configuration/configuration.json).

## Accessing the SSL Server

The SSL server can be accessed using curl to test the HTTPS functionality:

```bash
# Basic HTTPS request (ignore certificate verification for testing)
# Replace DEVICE_IP with your device's actual IP address (e.g., 192.168.1.100)
curl -k https://DEVICE_IP:4433

# Example response:
# Hello from SSL Server!
```

**Note**: The SSL server uses self-signed certificates for development, so the `-k` flag is used to disable certificate verification.

## Analyze process

The SSL application processes inference data and sends it securely to the SSL server. The main functionality includes:

1. **SSL Connection**: Establishes secure connection to the SSL server
2. **Edge Authentication**: Performs edge login to obtain authentication tokens
3. **Data Transmission**: Sends inference output tensor data securely to the server
4. **Result Processing**: Receives and processes results from the server

The SSL client implementation is located in the [**`ssl_client/`**](./ssl_client/) directory and provides functions for:
- `connect_ssl_server()`: Establishes SSL connection and performs authentication
- `send_output_tensor()`: Sends inference data securely
- `send_result()`: Sends processing results back to the server

The SSL server implementation is located in the [**`ssl_server/`**](./ssl_server/) directory and provides functions for:
- `start_ssl_server()`: Initializes and starts the SSL server
- `stop_ssl_server()`: Gracefully shuts down the SSL server

**Note**: The SSL server is automatically started when the edge application is created (`onCreate`) and stopped when it is destroyed (`onDestroy`), ensuring proper lifecycle management.

