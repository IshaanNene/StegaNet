#!/usr/bin/env python3
import sys
import subprocess
import os
import re

def sanitize_url(url):
    """
    Sanitize and normalize the YouTube URL
    """
    # Remove any surrounding whitespace
    url = url.strip()
    
    # If it's a partial URL, prepend full YouTube domain
    if url.startswith('www.youtube.com/'):
        url = f'https://{url}'
    elif url.startswith('youtube.com/'):
        url = f'https://www.{url}'
    
    # Handle cases where only video ID is provided
    if re.match(r'^[a-zA-Z0-9_-]+$', url):
        url = f'https://www.youtube.com/watch?v={url}'
    
    return url

def is_valid_youtube_url(url):
    """
    Validate YouTube URL format, including full URLs, shorts, and various formats
    
    Supports:
    - Regular YouTube video URLs
    - YouTube Shorts URLs
    - Embedded video URLs
    - Shortened youtu.be URLs
    """
    youtube_patterns = [
        # Standard watch URLs
        r'^(https?://)?(www\.)?youtube\.com/watch\?v=[a-zA-Z0-9_-]+',
        
        # Shortened youtu.be URLs
        r'^(https?://)?(www\.)?youtu\.be/[a-zA-Z0-9_-]+',
        
        # Embedded video URLs
        r'^(https?://)?(www\.)?youtube\.com/embed/[a-zA-Z0-9_-]+',
        
        # YouTube Shorts URLs
        r'^(https?://)?(www\.)?youtube\.com/shorts/[a-zA-Z0-9_-]+',
        
        # Alternative Shorts URL format
        r'^(https?://)?(www\.)?youtube\.com/watch\?v=[a-zA-Z0-9_-]+&feature=shorts',
        
        # Shorts on youtu.be
        r'^(https?://)?(www\.)?youtu\.be/[a-zA-Z0-9_-]+\?feature=shorts'
    ]
    
    return any(re.match(pattern, url, re.IGNORECASE) for pattern in youtube_patterns)

def download_audio(url):
    """
    Download audio from YouTube using yt-dlp.
    
    Args:
        url (str): YouTube video URL
    
    Returns:
        str: Path to the downloaded audio file or error message
    """
    # Sanitize and validate URL
    url = sanitize_url(url)
    
    if not is_valid_youtube_url(url):
        print(f"ERROR: Invalid YouTube URL format: {url}")
        return 1

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
    print(f"Received URL: {url}")  # Debug print
    return download_audio(url)

if __name__ == "__main__":
    sys.exit(main())