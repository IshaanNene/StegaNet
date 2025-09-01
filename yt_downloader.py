#!/usr/bin/env python3
import sys
import subprocess
import shutil
import os
from typing import Optional

def download_audio(url: str, output_dir: Optional[str] = None) -> Optional[str]:
    """
    Download audio from a YouTube URL using yt-dlp.
    
    Args:
        url (str): YouTube video URL
        output_dir (str, optional): Directory to save the downloaded audio
    
    Returns:
        Optional[str]: Path to the downloaded audio file or None if download fails
    """
    # Check if yt-dlp is installed
    if not shutil.which("yt-dlp"):
        print("ERROR: yt-dlp not installed. Please install using: pip install yt-dlp")
        return None

    # Use provided output directory or current directory
    output_dir = output_dir or os.getcwd()
    os.makedirs(output_dir, exist_ok=True)

    # Construct output path template
    output_template = os.path.join(output_dir, "youtube_audio_%(title)s.%(ext)s")

    # Comprehensive yt-dlp command
    command = [
        "yt-dlp",
        "-f", "ba[ext=wav]/ba[ext=m4a]/ba",  # Prefer WAV, then M4A
        "--extract-audio",
        "--audio-format", "wav",
        "--audio-quality", "0",  # Best quality
        "--no-playlist",  # Avoid downloading entire playlists
        "--max-filesize", "100M",  # Limit file size to 100MB
        "--output", output_template,
        url
    ]

    try:
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        
        # Find the output file from the logs
        for line in result.stdout.split('\n'):
            if line.startswith('[download] Destination:'):
                output_file = line.split(': ')[1].strip()
                print(f"SUCCESS: {output_file}")
                return output_file
        
        print("SUCCESS: Audio downloaded")
        return None

    except subprocess.CalledProcessError as e:
        print(f"ERROR: Download failed - {e.stderr}")
        return None

def main():
    """Main function to handle script execution"""
    if len(sys.argv) < 2:
        print("ERROR: No URL provided. Usage: python yt_downloader.py <youtube_url>")
        sys.exit(1)

    url = sys.argv[1]
    output_file = download_audio(url)
    
    if output_file:
        print(output_file)
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()