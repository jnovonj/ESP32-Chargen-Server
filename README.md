# ESP32 Chargen Server

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

> A simple and efficient implementation of the Character Generator Protocol (Chargen, RFC 864) for the ESP32 microcontroller.

This project turns an ESP32 into a Chargen server. Once connected to your Wi-Fi network, it listens on port 19 for TCP connections. When a client connects, the server sends a continuous stream of ASCII characters, making it a great tool for network testing or learning about TCP servers on embedded systems.

This implementation uses the `AsyncTCP` library to handle clients asynchronously, allowing it to serve multiple clients without blocking.

## Table of Contents

- Features
- Hardware & Software
- Installation
- Usage
- Contributing
- License
- Contact

## Features

-   ✅ **RFC 864 Compliant**: Implements the Character Generator Protocol.
-   ⚡ **Asynchronous**: Built with [`AsyncTCP`](https://github.com/ESP32Async/AsyncTCP/) to handle multiple clients efficiently without blocking the main loop.
-   🌐 **Wi-Fi Ready**: Easily configure your Wi-Fi credentials to get it online.

## Hardware & Software

### Required Hardware
*   An ESP32 development board (e.g., NodeMCU-32S, ESP32-DevKitC).

### Project Dependencies
*   Arduino Framework for ESP32
*   AsyncTCP for ESP32 [`AsyncTCP`](https://github.com/ESP32Async/AsyncTCP/)

*PlatformIO will automatically install these dependencies when you build the project.*

## Installation

Follow these steps to get the project running on your ESP32.

1.  **Clone the repository:**
    ```sh
    git clone https://github.com/jnovonj/ESP32-Chargen-Server.git
    cd ESP32-Chargen-Server
    ```

2.  **Configure Wi-Fi Credentials:**
    ```sh
    const char *Hostname = "ChargenServer";
    const char *ssid = "YOUR_WIFI_SSID";
    const char *password = "YOUR_WIFI_PASSWORD";
    ```

## Usage

1.  **Power On & Monitor:**
    After uploading, open the **Serial Monitor** in PlatformIO (plug icon). The ESP32 will attempt to connect to your Wi-Fi. Once connected, it will print its IP address to the serial monitor.

    ```
    Connecting to WiFi...
    WiFi connected.
    IP address: 192.168.1.123
    Chargen Server started on port 19
    ```

2.  **Connect to the Server:**
    From any computer on the same network, use a `telnet` client or `netcat` to connect to the ESP32's IP address on port 19.

    ```sh
    telnet 192.168.1.123 19
    ```
    Or using `netcat`:
    ```sh
    nc 192.168.1.123 19
    ```

    You should immediately see a continuous stream of ASCII characters printed to your terminal. To disconnect, close the `telnet` or `nc` session (usually with `Ctrl+]` then `quit` for telnet, or `Ctrl+C` for nc).

## Contributing

Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

Please read `CONTRIBUTING.md` for details on our code of conduct and the process for submitting pull requests.

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0). See the `LICENSE` file for full terms.

## Contact

jnovonj

Project Link: https://github.com/jnovonj/ESP32-Chargen-Server
