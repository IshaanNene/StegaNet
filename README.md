# StegaNet

<table>
  <tr>
    <td>
      <img width="500" height="500" src="https://github.com/user-attachments/assets/42fa47b2-d3f7-426f-9991-427cebb4c2ec"  alt="StegaNetLogo"/>
    </td>
    <td>
      <blockquote>
        StegaNet is a steganography implementation written in C, combined with socket programming for secure message transmission. It supports hiding and exchanging text, images, and audio messages. If no media is provided, the system can generate random images or audio automatically. Audio can also be imported directly from YouTube links.
      </blockquote>
    </td>
  </tr>
</table>

## Features
- [x] Send normal messages over sockets  
- [x] Image Steganography (Encode & Decode)  
- [x] Audio Steganography (Encode & Decode)  
- [x] Download audio directly from YouTube


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
1. Start two terminals
Run ```make clean && make all && make run``` on one then wait, then run ```make run``` on the other terminal
2. Make sure one is set to client mode and the other in server mode, and also ensure they both are on the same port
3. Use these screenshots as reference
<img width="1174" height="461" alt="Screenshot 2025-09-09 at 9 56 56 AM" src="https://github.com/user-attachments/assets/3690dfec-6995-42ae-ace7-21ce90d642d0" />
<img width="1219" height="476" alt="Screenshot 2025-09-09 at 9 57 12 AM" src="https://github.com/user-attachments/assets/2e611b92-8fd7-457e-ac8d-8113e622b419" />
<img width="1184" height="465" alt="Screenshot 2025-09-09 at 9 57 51 AM" src="https://github.com/user-attachments/assets/c4f4a6bd-cfc1-424d-af22-58219d390a75" />
<img width="1076" height="421" alt="Screenshot 2025-09-09 at 9 58 04 AM" src="https://github.com/user-attachments/assets/f22cbee7-3233-438a-bd7c-03b031b0c254" />

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
      <td><a href="main.c"><code>main.c</code></a></td>
      <td>Entry point of the program. Initializes the UI and coordinates between networking, steganography, and utilities.</td>
    </tr>
    <tr>
      <td><a href="network.c"><code>network.c</code></a></td>
      <td>Handles socket programming: setting up client/server modes, sending, and receiving messages.</td>
    </tr>
    <tr>
      <td><a href="steganography.c"><code>steganography.c</code></a></td>
      <td>Encoding and Decoding logic for images and audio files.</td>
    </tr>
    <tr>
      <td><a href="utils.c"><code>utils.c</code></a></td>
      <td>Utility functions for file I/O, random media generation, and data validation.</td>
    </tr>
    <tr>
      <td><a href="ui.c"><code>ui.c</code></a></td>
      <td>Raylib-based user interface for displaying prompts, visual feedback, and logs.</td>
    </tr>
    <tr>
      <td><a href="yt_downloader.js"><code>yt_downloader.js</code></a></td>
      <td>Node.js script that uses <code>yt-dlp</code> and <code>ffmpeg</code> to download audio from YouTube in WAV format.</td>
    </tr>
    <tr>
      <td><a href="Makefile"><code>Makefile</code></a></td>
      <td>Build system with targets for compiling, running, cleaning, installing dependencies (Linux/macOS), and debugging.</td>
    </tr>
  </tbody>
</table>

## Future Implementation
- [ ] Encrypted messaging using various encryption algorithms  
- [ ] Connection support between two different IPs (multi-device)

## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

