# Project 2

## Capabilities:

- [x] Interact with one client and send QR code
- [x] Log files
- [x] Args
- [x] Error checking: doesn't scan past given size, has upper limit.
- [x] Timeouts? (untested)

## Bugs:

- [x] DID NOT WORK IN SECNET, worked on macOS, endianness?
- [x] Sometimes, server doesn't receive first 28 bytes of the image and ends up terminating.
- [x] Server can create multiple connections, but usually only first connection gets the QR code. Second connection becomes unresponsive.
- [x] No rate limiting.

## To Run:

- `make`
- `./QServer` or `./QServer --port <p> --rate <r> --max <m> --timeout <t>`
- `./QClient [img_path] [server_ip] [server_port]`

## Typescripts:

contains terminal output

- `cat server`
- `cat client1`
- `cat client2`
