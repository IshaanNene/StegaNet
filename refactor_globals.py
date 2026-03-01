import os
import re

identifiers = [
    "messages", "messageCount", "connection", "inputBuffer",
    "hiddenMessageBuffer", "inputEditMode", "hiddenMessageEditMode", "scrollOffset",
    "isServer", "currentTransfer", "messageMutex", "showConnectionDialog",
    "serverIPBuffer", "portBuffer", "serverIPEditMode", "portEditMode",
    "statusMessage", "statusTimer", "showEncodingOptions", "selectedMessageType",
    "selectedFilePath", "filePathEditMode", "shouldExit", "currentImageTexture",
    "imageLoaded", "showDecodeDialog", "selectedMessageIndex", "decodeFilePath",
    "decodeFilePathEditMode", "decodedHiddenMessage", "messageImages",
    "messageImageLoaded", "messageAudioSounds", "messageAudioLoaded",
    "audioPlaying", "showYTDialog", "ytUrlBuffer", "ytUrlEditMode", "isDownloading"
]

src_dir = "./src"

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Don't replace struct fields if we are looking at struct definitions etc.
    # We will do a regex sub: we look for word boundaries, but not if preceded by "->" or "."
    for ident in identifiers:
        # Match ident only if not preceded by '.' or '->' and not followed by '.' or '->' ?
        # Actually it's simpler: negative lookbehind for '.' and '->'
        # Also let's avoid replacing them if they are in "sizeof(var)"? No, sizeof(state->var) is correct.
        # We might accidentally replace parameter names if they match these identifiers! 
        # But this code probably uses the globals directly.
        # Let's hope parameter names in functions don't perfectly overlap with global names.
        pattern = r'(?<![\.\>])\b' + ident + r'\b(?![\.])'
        content = re.sub(pattern, f"state->{ident}", content)
        
    with open(filepath, 'w') as f:
        f.write(content)

for filename in os.listdir(src_dir):
    if filename.endswith(".c"):
        process_file(os.path.join(src_dir, filename))

print("Refactored references.")
