# TFTP Client Testing Strategy

## Overview

The TFTP client test suite is organized into unit tests and integration tests based on the complexity and dependencies of the code under test.

## Unit Tests

Unit tests focus on testing individual functions and components in isolation without external dependencies.

### test_tftp_impl.cpp

Tests utility functions from `tftp/impl/tftp_impl.hpp`:

- **`get_error_message()`** - Validates and extracts error messages from TFTP ERROR packets
  - Valid messages with various content
  - Empty messages
  - Missing null terminators
  - Boundary conditions
  - Special characters

- **`try_with()`** - Exception handling wrapper for async operations
  - Successful execution without exceptions
  - `std::bad_alloc` mapping to `not_enough_memory`
  - Generic exceptions mapping to `state_not_recoverable`
  - Cleanup invocation on errors

### test_client_senders.cpp

Tests sender type construction and properties:

- `connect_t` sender construction
- `put_file_t` sender construction
- `get_file_t` sender construction
- Sender type traits and inheritance
- Edge cases (empty paths, long paths, special characters)

### test_tftp_client.cpp

Tests the high-level client API:

- Client manager and client creation
- Connection sender creation
- Put/Get file sender creation
- Default vs explicit transfer modes
- Parameter ordering

### Other Unit Tests

- **test_argument_parser.cpp** - CLI argument parsing
- **test_dns.cpp** - DNS error codes and categories
- **test_endian.cpp** - Endianness conversion utilities
- **test_filesystem.cpp** - File system operations
- **test_generator.cpp** - Coroutine generator
- **test_tftp_protocol.cpp** - TFTP protocol message handling

## Integration Tests

### Methods Requiring Integration Testing

The following template methods in `tftp/impl/tftp_impl.hpp` are marked with `GCOVR_EXCL_START/STOP` and excluded from unit test coverage because they require complex integration testing:

#### client_state Methods

- **`cleanup()`** - Requires timer removal, file cleanup, socket shutdown
- **`finalize()`** - Requires cleanup and receiver completion
- **`error_handler()`** - Requires finalize through try_with
- **`submit_sendmsg()`** - Requires async socket operations
- **`submit_recvmsg()`** - Requires async socket operations

#### put_file_t State Methods

- **`start()`** - Initializes file upload
- **`send_wrq()`** - Sends write request
- **`ack_handler()`** - Handles ACK messages
- **`route_message()`** - Routes incoming messages
- **`connect()`** - Creates connection state

#### get_file_t State Methods

- **`start()`** - Initializes file download
- **`send_rrq()`** - Sends read request
- **`data_handler()`** - Handles DATA messages
- **`route_message()`** - Routes incoming messages
- **`connect()`** - Creates connection state

### Integration Test Requirements

These methods depend on:

- **Async I/O Operations** - `io::sendmsg()`, `io::recvmsg()`
- **Timer Management** - `ctx->timers.add()`, `ctx->timers.remove()`
- **Socket Operations** - `ctx->poller.emplace()`, `io::shutdown()`
- **File System** - `std::fstream`, `std::filesystem::remove()`, `std::filesystem::rename()`
- **Sender/Receiver Framework** - `stdexec::set_value()`, `stdexec::set_error()`
- **Async Context** - `ctx->scope.spawn()`

### Creating Integration Tests

Integration tests should:

1. Set up a test TFTP server (or mock server)
2. Create a real async context with timers and event loop
3. Exercise complete file transfer operations
4. Verify file contents and error handling
5. Test timeout and retry behavior
6. Test error conditions (network errors, file errors, protocol errors)

## Running Tests

```bash
# Run all tests
ctest --preset debug --output-on-failure

# Run specific test suite
./build/debug/tests/test_tftp_impl

# Run with coverage report
cmake --build --preset debug --target coverage
```

## Coverage Reports

Coverage reports are generated using gcovr and exclude integration-only code marked with `GCOVR_EXCL_START/STOP`. View the HTML coverage report:

```bash
cmake --build --preset debug --target coverage
# Open build/debug/coverage/index.html
```

## Adding New Tests

Add unit tests when:

- Testing pure functions without external dependencies
- Testing utility functions and data transformations
- Testing type construction and properties
- Testing error conditions that can be simulated

Plan integration tests when code requires:

- Real network I/O operations
- File system operations with state
- Async runtime with timers and scheduling
- Complex interaction between multiple components
- End-to-end behavior validation

Mark such code with `GCOVR_EXCL_START/STOP` in the implementation file and document the integration test requirements.
