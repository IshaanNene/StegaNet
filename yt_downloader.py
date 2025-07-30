# yt_downloader.py
import sys
import subprocess
import shutil

def download_audio(url: str) -> int:
    if not shutil.which("yt-dlp"):
        print("ERROR: yt-dlp not installed")
        return 1

    command = [
        "yt-dlp",
        "-f", "ba[ext=wav]/ba[ext=m4a]/ba",
        "--extract-audio",
        "--audio-format", "wav",
        "--output", "youtube_audio.%(ext)s",
        url
    ]

    try:
        subprocess.run(command, check=True)
        print("SUCCESS")
        return 0
    except subprocess.CalledProcessError as e:
        print("ERROR: Failed to download")
        return 2

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("ERROR: No URL provided")
        sys.exit(3)

    url = sys.argv[1]
    sys.exit(download_audio(url))
