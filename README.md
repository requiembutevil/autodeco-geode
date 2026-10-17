# Auto Deco AI (Geode iOS)

This project adds two editor buttons:

- **AUTO** uses the original local rule-based glow/shine decorator and does not need AI or network access.
- **AI** reads the prompt field and asks an OpenAI-compatible chat-completions endpoint for a placement plan. The mod then creates the requested decorative objects in the Geometry Dash editor.

## What prompts can control

The AI receives sampled solid-block positions from across the level. It can choose placement density, pattern, spacing, and offsets for two allowed decorative object types: glow (object ID 503) and shine (object ID 1021). It cannot create arbitrary object types or change block colors in this version. It is limited to 160 planned placements per request, and invalid coordinates/object types are ignored.

Example prompt:

> Put subtle glows just below the blocks, and add a tiny shine on every few sections. Keep it sparse and don't clutter the level.

## Local AI setup (no Homebrew or API key needed)

The defaults use Ollama running on your Mac. The model runs on the Mac, while the iPhone mod sends the text prompt and sampled block positions over your local Wi-Fi network.

1. Download and install Ollama from the [official macOS download page](https://ollama.com/download/mac). It supports macOS 14+ on Intel/x86 Macs, using the CPU. [Ollama macOS requirements](https://github.com/ollama/ollama/blob/main/docs/macos.mdx)
2. Open Terminal and download a small model:

   ```sh
   ollama pull qwen3:1.7b
   ```

3. Quit the Ollama app if it is currently running. In Terminal, start the server so your iPhone can reach it on your trusted home Wi-Fi:

   ```sh
   OLLAMA_HOST=0.0.0.0:11434 ollama serve
   ```

   Leave that Terminal window open while using the AI button. If macOS asks about incoming connections, allow it only on a network you trust. Do not expose this server to the public internet.

4. In another Terminal window, get your Mac's Wi-Fi IP:

   ```sh
   ipconfig getifaddr en0
   ```

   If this prints something like `192.168.1.25`, open Geode > Mods > Auto Deco AI settings on the iPhone and set **Chat completions endpoint** to:

   `http://192.168.1.25:11434/v1/chat/completions`

   Replace that example IP with the exact address printed on your Mac. Keep **Model** set to `qwen3:1.7b`, and leave **API key** blank.

5. Make sure the iPhone and Mac are on the same private Wi-Fi network. In the editor, type a prompt in the field next to **AI**, then tap **AI**. The Mac needs to stay awake and the Ollama server must keep running.

Local inference is free after the model download, but an older Intel Mac may respond slowly. If the request cannot connect, confirm both devices are on the same Wi-Fi, the Ollama server is still running, and the IP address is correct.

## Build on macOS for iOS

Requirements:

- Xcode with the iPhoneOS SDK installed
- Geode CLI 3.9.0 or newer
- Geode SDK matching the `geode` version in `mod.json` (currently 5.11.1)
- Ninja and CMake available on PATH

From this project folder:

```sh
geode sdk install-binaries --platform ios
rm -rf build-ios
geode build -p ios
find build-ios -name '*.geode' -print
```

The resulting `.geode` still has to be imported into the iOS Geode Launcher using the same import/signing method as your previous working build.
