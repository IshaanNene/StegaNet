#!/usr/bin/env python3
import sys
import subprocess
import os
import re

def is_valid_youtube_url(url):
    """
    Validate YouTube URL format
    """
    # Regex patterns for various YouTube URL formats
    youtube_patterns = [
        r'^(https?://)?(www\.)?(youtube\.com/watch\?v=|youtu\.be/)[a-zA-Z0-9_-]+',
        r'^(https?://)?(www\.)?(youtube\.com/embed/)[a-zA-Z0-9_-]+',
        r'^(https?://)?(www\.)?(youtube\.com/v/)[a-zA-Z0-9_-]+'
    ]
    
    return any(re.match(pattern, url) for pattern in youtube_patterns)

def download_audio(url):
    """
    Download audio from YouTube using yt-dlp.
    
    Args:
        url (str): YouTube video URL
    
    Returns:
        str: Path to the downloaded audio file or error message
    """
    # Ensure full YouTube URL
    if not url.startswith(('http://', 'https://')):
        url = f"https://www.youtube.com/{url}"

    try:
        # Ensure the output is a WAV file
        command = [
            "yt-dlp",
            "-f", "ba[ext=wav]/ba",  # Prefer WAV format
            "--extract-audio",
            "--audio-format", "wav",
            "--output", "youtube_audio.%(ext)s",
            url
        ]
        
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        
        # Check if file exists
        if os.path.exists("youtube_audio.wav"):
            print("youtube_audio.wav")
            return 0
        else:
            print("ERROR: Failed to download audio")
            return 1
    
    except subprocess.CalledProcessError as e:
        print(f"ERROR: {e.stderr.strip()}")
        return 1
    except Exception as e:
        print(f"ERROR: {str(e)}")
        return 1

def main():
    if len(sys.argv) < 2:
        print("ERROR: No URL provided")
        return 1
    
    url = sys.argv[1]
    return download_audio(url)

if __name__ == "__main__":
    sys.exit(main())