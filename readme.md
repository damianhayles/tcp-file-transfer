# TCP File Transfer Client and Server (C/C#)

## Overview

This project implements a basic TCP client-server application in C and C# that enables file transfers over a network. The client sends files to the server in chunks, and the server writes the received data to a file on disk.

- **Language(s)**: C, C#  
- **Protocol**: TCP/IP  
- **Transfer Method**: Custom chunked packet format (4-byte header + data)

## Build Instructions

### Prerequisites

- A C compiler (e.g. `gcc`)
- POSIX-compliant system (Linux/macOS)
- Optional: `make`

### Build with GCC

``` bash
gcc -o client client.c
gcc -o server server.c
```

### Start the server

``` bash
./server
```

### Run the client

``` bash
./client
```

### Building C# client

Create a new C# Console App and copy the contents of Program.cs over. In Bash run the following:

``` bash
cd csharpclient
```

Then to publish;
for Windows:

``` bash
dotnet publish -c Release -r win-x64 --self-contained true
```

For Linux:

``` bash
dotnet publish -c Release -r linux-x64 --self-contained true
```

For macOS:

``` bash
dotnet publish -c Release -r osx-x64 --self-contained true
```

run it with:

``` bash
./csharpclient

