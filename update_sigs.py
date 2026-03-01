import os
import re

funcs = [
    "InitializeConnection", "CloseConnection", "SendMessage", "SendFile",
    "DrawConnectionDialog", "DrawChat", "DrawDecodeDialog", "DrawYTDialog",
    "EncodeMessageInImage", "DecodeMessageFromImage", "EncodeMessageInAudio", "DecodeMessageFromAudio",
    "AddMessage", "ShowStatus", "GenerateRandomImage", "GenerateRandomAudio",
    "DownloadFromYoutube", "LoadImageForDisplay"
]

src_dir = "./src"

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    for func in funcs:
        # replace calls
        content = re.sub(rf'\b{func}\(', f"{func}(state, ", content)
        content = content.replace(f"{func}(state, )", f"{func}(state)")
        
        # fix definitions
        content = re.sub(rf'\b(void|char\*)\s+{func}\(state, ', rf'\1 {func}(AppState *state, ', content)
        content = re.sub(rf'\b(void|char\*)\s+{func}\(state\)', rf'\1 {func}(AppState *state)', content)

    # special case for ReceiveMessages
    content = content.replace("ReceiveMessages(state, ", "ReceiveMessages(")
    content = content.replace("ReceiveMessages(state)", "ReceiveMessages()")

    # Also fix pthread_create to pass `state` instead of `NULL`
    content = content.replace("pthread_create(&state->connection.receiveThread, NULL, ReceiveMessages, NULL)", 
                              "pthread_create(&state->connection.receiveThread, NULL, ReceiveMessages, state)")

    with open(filepath, 'w') as f:
        f.write(content)

for filename in os.listdir(src_dir):
    if filename.endswith(".c"):
        process_file(os.path.join(src_dir, filename))

print("Signatures updated.")
