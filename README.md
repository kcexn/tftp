# TFTP client

[![Tests](https://github.com/kcexn/rfc862-echo/actions/workflows/tests.yml/badge.svg)](https://github.com/kcexn/rfc862-echo/actions/workflows/tests.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Coverage/05993300535548f5a0f0585b5b3d2c26)](https://app.codacy.com/gh/kcexn/tftp/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_coverage)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/05993300535548f5a0f0585b5b3d2c26)](https://app.codacy.com/gh/kcexn/tftp/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

A modern TFTP (Trivial File Transfer Protocol) client. Implements [RFC 1350](https://www.rfc-editor.org/rfc/rfc1350). This is the client counterpart to my [TFTP server](https://github.com/kcexn/tftpd).

## Features

- **Sender/Receivers** implementation using asynchronous senders/receivers pattern.
- **RFC 1350 compliant** TFTP protocol support
- **Command-line interface** for file transfers

## Building

Requires CMake 3.28+ and a C++20 compiler.

```bash
# Configure debug build
cmake --preset debug

# Build
cmake --build --preset debug

# Run tests
ctest --preset debug --output-on-failure

# Build optimized release
cmake --preset release
cmake --build --preset release
```

## Installing

The default installation prefix is `/usr/local`. To install into a different location, you can set the `CMAKE_INSTALL_PREFIX` when configuring the build.

```bash
# Configure a release build.
cmake --preset release

# Configure a release build with a custom install prefix (default: /usr/local/).
cmake --preset release -D CMAKE_INSTALL_PREFIX=~/.local

# Build the release version.
cmake --build --preset release

# Install into the configured target.
# This may require root privileges (e.g. `sudo`) if installing to a system directory.
cmake --install build/release
```

## Uninstalling

```bash
# The install manifest is created in the build directory,
# we can use it to remove all installed files.
xargs rm < build/release/install_manifest.txt
```

## Usage

```bash
# Download a file from TFTP server
tftp -H hostname[:port] get <remote> <local>

# Upload a file to TFTP server
tftp -H hostname[:port] put <local> <remote>
```

### Options

- `-h, --help` - Display help message
- `-H, --host=<host[:port]>` - TFTP server hostname:port (required, default port: 69)
- `--mode=<netascii|octet>` - Transfer mode (default: octet)

### Examples

```bash
# Download with default port (69)
tftp -H localhost get /tmp/remote.txt ./local.txt

# Upload with custom port
tftp -H localhost:6969 put ./file.txt /tmp/file.txt

# Download using netascii mode
tftp -H example.com --mode=netascii get /path/to/file.txt download.txt
```

## License

GNU General Public License v3.0 or later. See [LICENSE](LICENSE) for details.
