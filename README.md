# mgwrapper

Mongoose C/C++ wrapper that provides simple TCP/HTTP/MQTT client helpers and a small test harness.

This repository bundles Mongoose source files and a lightweight C++ wrapper to create TCP and HTTP clients with a tiny event loop and test cases using GoogleTest.

## Repository layout

- `CMakeLists.txt` - top-level CMake build file.
- `src/` - source files for mongoose and wrapper implementation.
  - `connect.h`, `connect.cc` - client connection abstraction (TCP / HTTP connect helpers).
  - `client.h`, `client.cc` - client manager and event loop (`IClient`, `ILoop`).
  - `http_client.h`, `http_client.cc` - higher-level HTTP client wrapper (singleton-based).
  - `mongoose.c`, `mongoose.h` - bundled Mongoose library sources (single-file style split into many sections).
  - `iloop.h` - internal event loop helper.
- `test.cc` - GoogleTest unit tests which exercise the HTTP client.
- `build/` - CMake build output (ignored in source control normally).

## Purpose

This project demonstrates a small wrapper around Mongoose that:
- Runs a background poll loop (in a thread).
- Provides `TcpConnect` and `HttpConnect` convenience classes for HTTP/TCP client flows.
- Exposes a simple client manager `IClient` that schedules connections.
- Contains unit tests (`mg_test.cc`) using GoogleTest.

## Requirements

- CMake >= 3.20
- A C/C++ compiler (MSVC / clang / GCC compatible with C++17)
- GoogleTest for unit tests (the CMake build has an `ENABLE_TEST` option)
- (Optional) TLS library (OpenSSL or mbedTLS) if you need HTTPS support

On Windows, consider using vcpkg to install dependencies (OpenSSL, mbedtls, gtest) and integrate with CMake.

## Build (Windows / PowerShell)

1) Create a build directory and configure with tests enabled:

```powershell
# from project root (z:\Project\mgwrapper)
cmake -S . -B build -DENABLE_TEST=ON
cmake --build build --config Debug
```

2) Run the tests (if built):

```powershell
ctest --test-dir build --output-on-failure
```

If you use vcpkg, you can integrate by adding `-DCMAKE_TOOLCHAIN_FILE=path\to\vcpkg.cmake` to the `cmake` command and installing packages like `gtest`, `openssl` or `mbedtls` via vcpkg first.

## Enabling HTTPS/TLS support

The bundled Mongoose code supports several TLS backends, but TLS is controlled at compile time by the `MG_TLS` macro. If TLS is not enabled in the build, any call to `mg_tls_init()` will hit a dummy implementation that logs `"TLS is not enabled"` and will not perform TLS.

To enable TLS you must:

1. Provide and link a TLS implementation (OpenSSL or mbedTLS) to the build.
2. Add a compile definition so that mongoose is built with the chosen backend.

Example CMake additions for OpenSSL (suggested; adjust to your environment):

```cmake
find_package(OpenSSL REQUIRED)
# Choose backend mapping supported by the mongoose version in this repo
# See mongoose sources for names like MG_TLS_OPENSSL or MG_TLS_MBED
target_compile_definitions(${PROJECT_NAME} PRIVATE MG_TLS=MG_TLS_OPENSSL)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
```

For mbedTLS similarly:

```cmake
find_package(MbedTLS REQUIRED)
target_compile_definitions(${PROJECT_NAME} PRIVATE MG_TLS=MG_TLS_MBED)
target_link_libraries(${PROJECT_NAME} PRIVATE MbedTLS::MbedTLS)
```

Notes:
- On Windows using vcpkg: `vcpkg install openssl gtest` and pass `-DCMAKE_TOOLCHAIN_FILE=...\vcpkg.cmake`.
- The unit test `mg_test.cc` sets a CA path (`/etc/ssl/certs/ca-certificates.crt`) in `SetUp()` — this is a UNIX path. On Windows you need to pass a valid certificate bundle path or change the test to not require TLS, otherwise hostname verification or CA loading will fail.

## Running the example test (`mg_test.cc`)

The test includes two tests (GET and POST). Some notes when running them:

- The GET test uses `http://httpbin.org/get?...` and should work without TLS.
- The POST test in `mg_test.cc` uses an `https://` URL; if your build has no TLS backend enabled you'll get an error from Mongoose: `TLS is not enabled`. To run HTTPS tests, enable TLS as described above and provide a valid CA bundle path appropriate for your platform.

## Common issues and troubleshooting

- Log: "TLS is not enabled"
  - Cause: Build was compiled with `MG_TLS == MG_TLS_NONE` (no TLS backend). Fix: enable OpenSSL/mbedTLS in CMake and link the library.

- Unit test sees `400 Bad Request` from server
  - Possible causes:
    - Missing or malformed HTTP headers (e.g., `Content-Length` missing when sending a body). For POST requests ensure you set `Content-Length` or use chunked transfer if supported.
    - The HTTP method/URI line or header formatting may be incorrect. The wrapper builds request lines like `"%s %s HTTP/1.0\r\nHost: %.*s\r\n%s\r\n"`. Make sure `options_.method` is a C string (the code uses `options_.method.c_str()`), the URI is valid, and `ParseHeaders()` produces correctly formatted headers (each header line must end with `\r\n`).
    - Some servers expect HTTP/1.1 and particular headers; switch to HTTP/1.1 and include `Connection: close` or other headers if needed.
  - How to debug:
    - Log the full request being sent (method, URI, headers, and body). Mongoose `mg_printf()` is used to write the request; inspect the composed string if you see a 400.
    - Use a known-good test endpoint (for example `http://httpbin.org`) and verify headers.

- Segmentation fault in `connect.cc` or `EventHandler`
  - Make sure that when registering event handler you pass the object pointer as `c->fn_data` (Mongoose stores callback userdata in `fn_data`). The wrapper sets `mg_connect(..., static_cast<void*>(this))` which is correct; `EventHandler` then reads `c->fn_data` and casts to the wrapper `Base` pointer. If `fn_data` is not set correctly or the object is freed early, you'll get a segfault.
  - Ensure lifetime of objects created and passed to the event loop: `IClient` creates connections and schedules them onto the loop. If the connection object is destroyed before the connection uses `fn_data`, it causes invalid pointer access.

## API usage examples

Simple HTTP GET using the client manager:

```cpp
IClient client; // starts event loop in background
HttpOptions opt;
opt.method = "GET";
opt.url = "http://httpbin.org/get?user=abc";
opt.on_message = [](const Message &m) {
  // m.body contains response bytes
};
client.Create<HttpConnect>(std::move(opt));
```

Notes:
- The wrapper is minimal and uses callbacks for events (`on_message`, `on_error`, `on_closed`).
- For HTTPS, set `HttpOptions::cert` to a path or packed certificate if required and build with TLS support.

## Tests

- Enable tests with CMake option `-DENABLE_TEST=ON`.
- The test executable (when enabled) is named `mgtest` in the top-level `CMakeLists.txt`.

## Development notes

- This repository includes a full copy of `mongoose.c`. Keep Mongoose licenses intact when distributing derivatives.
- The code uses `mg_*` APIs from the local `mongoose.c` bundle; changes to the Mongoose version may require adapting compile-time macros (MG_TLS, MG_ENABLE_* flags).

## Next steps and recommended improvements

- Add a proper CMake option to choose TLS backend and automatically find/link OpenSSL or mbedTLS.
- Add a `tests/` directory and more unit tests that don't rely on external hosts (use a local test server or mocks).
- Add CI steps (GitHub Actions) to build with/without TLS and run tests.

---

If you'd like, I can:
- Add example CMake modifications to enable OpenSSL/mbedTLS to this repo.
- Add a small self-contained test server for offline unit tests.
- Improve the README with platform-specific commands for vcpkg and Windows OpenSSL installation.

Which of these would you like next?