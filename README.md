# mgwrapper

Mongoose C/C++ wrapper that provides simple TCP/HTTP/MQTT client helpers and a small test harness.

This repository bundles Mongoose source files and a lightweight C++ wrapper to create TCP and HTTP clients with a tiny event loop and test cases using GoogleTest.

## Purpose

This project demonstrates a small wrapper around Mongoose that:
- Runs a background poll loop (in a thread).
- Provides `TcpConnect` and `HttpConnect` convenience classes for HTTP/MQTT/TCP client flows.
- Exposes a simple client manager `IClient` that schedules connections.
- Contains unit tests (`test.cc`) using GoogleTest.

## Requirements

- CMake >= 3.20
- A C/C++ compiler (MSVC / clang / GCC compatible with C++17)
- GoogleTest for unit tests (the CMake build has an `ENABLE_TEST` option)
- (Optional) TLS library (OpenSSL or mbedTLS) if you need HTTPS support

## Running the example test (`test.cc`)

The test includes two tests (GET and POST). Some notes when running them:

- The GET test uses `http://httpbin.org/get?...` and should work without TLS.
- The POST test in `mg_test.cc` uses an `https://` URL; if your build has no TLS backend enabled you'll get an error from Mongoose: `TLS is not enabled`. To run HTTPS tests, enable TLS as described above and provide a valid CA bundle path appropriate for your platform.

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

## Tests

- Enable tests with CMake option `-DENABLE_TEST=ON`.
- The test executable (when enabled) is named `mgtest` in the top-level `CMakeLists.txt`.