# TCP Chat Server

A multi-threaded chat server and client written in C using raw TCP sockets and POSIX threads (pthreads). Multiple clients can connect to the server at the same time and broadcast messages to each other in real time.

## How it works

- The server listens for incoming connections and spins up a dedicated thread for each client that connects, allowing many clients to be handled concurrently.
- A shared list of connected clients is protected with a mutex, so multiple threads can safely add, remove, and broadcast to clients without race conditions.
- When one client sends a message, the server broadcasts it to every other connected client.
- The client itself runs two threads: one that reads user input and sends it, and a separate one that listens for incoming messages — so typing a new message is never blocked while waiting on a reply.

## Features

- Concurrent client handling via threads (one thread per client on the server)
- Thread-safe shared state using a mutex-protected client list
- Non-blocking chat experience on the client (independent send/receive threads)
- Graceful handling of client disconnects

## Tech

- C
- POSIX sockets (`socket`, `bind`, `listen`, `accept`, `connect`)
- POSIX threads (`pthread_create`, `pthread_mutex_t`)

## How to run

Compile the server and client (both need `-pthread` since both use threads):

```bash
gcc server.c -o server -pthread
gcc client.c -o client -pthread
```

Start the server in one terminal:

```bash
./server
```

Start the client in one or more additional terminals:

```bash
./client
```

Type a message in any client and press Enter — it will be broadcast to every other connected client. Type `quit` to disconnect.

## What I'd add next

- Message framing (e.g. newline-delimited messages) to prevent two messages sent close together from merging into one on the receiving end, since TCP is a byte stream with no built-in message boundaries
- Usernames, so broadcast messages show who sent them
- Chat rooms/channels
- Persistent message history

## What this project demonstrates

Built this while learning network programming fundamentals from scratch — socket creation, the TCP connection lifecycle, and concurrency with threads and mutexes to safely handle multiple simultaneous clients.
