# Memory Usage Tracking Tool for Wasm

`Memory Usage Tracking Tool` is a debugging tool that overrides `malloc` and `free` in `wasi-libc` to track memory usage in WebAssembly (Wasm) applications.


## How to use

1. **Rebuild Docker if you have app_build_env image**:

   ```bash
   docker image rm app_build_env:2.0.0
   ```

2. **Build with Debug**:

   ```bash
   make CMAKE_FLAGS="-DAPPS_SELECTION=your_app -DMEMORY_PROF=1"
   ```

3. **Run App**:

   ```bash
   make vm_run edge_app=$PWD/bin/edge_app.wasm
   ```

4. **Start Inference by changing the process_state and Set log level to debug**:

   ```bash
   ./tools/send_configuration.sh --process_state=2
   ```

5. **Observe logs** (log_level = 5):

   ```
   [DEBUG] memory_usage.cpp: Memory used: 151224 bytes, fragmentation_rate 0.24%
   ```
　 
   - `Memory used`: 

      The total amount of memory currently allocated in bytes.
   - `fragmentation_rate`: 
   
      Indicates how much the available memory has become fragmented, making it harder to allocate large contiguous memory blocks even if the total free memory is sufficient. This metric is expressed as a percentage. A higher rate means more fragmentation, which can lead to memory allocation failures, especially for large buffers.

---

## Future

- Dynamic linking (.so) support when available in WAMR

---

## License

MIT License.  
Based on [wasi-libmalloc](https://github.com/yamt/wasi-libmalloc/)