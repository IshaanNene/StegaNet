# StegaNet

<table>
  <tr>
    <td>
      <img width="600" height="600" alt="StegaNetLogo" src="https://github.com/user-attachments/assets/27e37890-005b-4f46-8840-e8b9271a35db" />
    </td>
    <td>
      <blockquote>
        StegaNet is a steganography implementation written in C, combined with socket programming for secure message transmission. It supports hiding and exchanging text, images, and audio messages. If no media is provided, the system can generate random images or audio automatically. Audio can also be imported directly from YouTube links.
      </blockquote>
    </td>
  </tr>
</table>

## Features
- [x] Send structured messages over TCP sockets with length-prefixed framing  
- [x] Image Steganography (Encode & Decode up to 4KB capacity across R,G,B channels)  
- [x] Audio Steganography (Encode & Decode utilizing LSB manipulation)  
- [x] Download audio directly from YouTube
- [x] AES-256-CBC Payload Encryption & CRC32 Integrity Checks
- [x] Resilient Network Protocol (Keep-alive Pings, Timeouts, Latency Measurement)
- [x] Leveled Rolling Logs (`steganet.log`) for system observation
- [x] Complete UI experience (Search/Filter, Drag & Drop, Notification sounds)

## Architecture & Security
StegaNet utilizes a centralized `AppState` model to orchestrate multi-threaded networking away from the Raylib UI thread safely using mutexes. Every message transiting the network can optionally be **encrypted** statically via OpenSSL using AES-256-CBC, and guaranteed through a custom CRC32 packet checksum signature.

## How to run 
### 1. Clone the repo
```bash
git clone git@github.com:IshaanNene/StegaNet.git
```
### 2. Install dependencies

For Linux:
```bash
make install-deps
```
For macOS:
```bash
make install-deps-mac
```
Additionally, install Node.js dependencies (for YouTube audio download):
```bash
npm install
```

### 3. Run the project
1. Build the source binaries (with or without ASan)
```bash
make clean && make asan && make run
```
2. For testing over two nodes, configure one instance on port `8888` under "Server" and the other pointing to the server's IP address.
3. You can utilize `Ctrl+Enter` to send, Drag/Drop valid images (.png, .jpg) or audio (.wav, .mp3), and observe connection latency via the header UI indicators.

### 4. File Overview
<table>
  <thead>
    <tr>
      <th>File</th>
      <th>Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><a href="src/main.c"><code>src/main.c</code></a></td>
      <td>Entry point of the program. Initializes the window, components, UI rendering loops, and the application state.</td>
    </tr>
    <tr>
      <td><a href="src/network.c"><code>src/network.c</code></a></td>
      <td>Handles length-prefixed protocol streams, socket setups, background thread reception, and timeout pings.</td>
    </tr>
    <tr>
      <td><a href="src/steganography.c"><code>src/steganography.c</code></a></td>
      <td>Multi-channel embedding and extraction algorithms for images (RGB) and WAV files (LSB).</td>
    </tr>
    <tr>
      <td><a href="src/crypto.c"><code>src/crypto.c</code></a></td>
      <td>Interface interfacing with OpenSSL providing `EncryptData` and `DecryptData` (AES-256/XOR fallback).</td>
    </tr>
    <tr>
      <td><a href="src/logging.c"><code>src/logging.c</code></a></td>
      <td>Secure, thread-friendly rotating Leveled logger generating standard outputs in `.log` extensions.</td>
    </tr>
    <tr>
      <td><a href="src/utils.c"><code>src/utils.c</code></a></td>
      <td>Utility routines executing file I/O operations, populating the GUI message list, and YouTube handling.</td>
    </tr>
    <tr>
      <td><a href="src/ui.c"><code>src/ui.c</code></a></td>
      <td>Raylib UI view component drawing message bubbles, search inputs, connection indicators, and file droppers.</td>
    </tr>
  </tbody>
</table>

## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

