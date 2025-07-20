import sys
from pytube import YouTube

def download_audio(yt_url):
    try:
        yt = YouTube(yt_url)
        audStream = yt.streams.filter(only_audio=True).first()
        if audStream:
            print(f"Downloading audio: {yt.title}")
            audStream.download(filename=f"{yt.title}.mp3")
            print("Download complete!")
        else:
            print("No audio stream found for this video.")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    if len(sys.argv) > 1:
        youtube_url = sys.argv[1]
        download_audio(youtube_url)
    else:
        print("Please provide a YouTube link as an argument.")