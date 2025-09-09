#!/usr/bin/env node
const { spawn } = require("child_process");
const fs = require("fs");
const path = require("path");

if (process.argv.length < 3) {
    console.error("Usage: node yt_downloader.js <youtube_url>");
    process.exit(1);
}

const url = process.argv[2];
const outTemplate = "%(title)s [%(id)s].%(ext)s";

const dl = spawn("/Users/ishaannene/.pyenv/shims/yt-dlp", [
    "-x",
    "--audio-format", "wav",
    "--audio-quality", "0",
    "--postprocessor-args", "ffmpeg:-acodec pcm_s16le",
    "--output", outTemplate,
    url
]);


dl.stderr.on("data", data => {
    process.stderr.write(data);
});

dl.on("close", code => {
    if (code === 0) {
        const files = fs.readdirSync(".").filter(f => f.includes(".wav"));
        if (files.length > 0) {
            console.log(files[0]);
            process.exit(0);
        } else {
            console.error("Download completed but file not found");
            process.exit(1);
        }
    } else {
        console.error("yt-dlp failed with exit code " + code);
        process.exit(1);
    }
});
