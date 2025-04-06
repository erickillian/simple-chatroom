# Simple Chatroom programmed in C

![Build Status](https://img.shields.io/badge/build-passing-brightgreen) ![License](https://img.shields.io/badge/license-MIT-blue)

A simple chatroom application written in C that allows multiple users to communicate in real-time using a telnet client. This application is designed to run on Linux systems.

---

## Table of Contents

- [Server](#server)
    - [How to Run the Server](#running-the-server)
    - [Using a Custom Port and Host](#custom-port-and-host)
- [Client](#client)
    - [How to Connect to the Server](#connecting-to-the-server)
- [Available Commands](#commands)

---

## Server

### Running the Server

To run the chatroom server, execute the following commands:

```bash
make
./server
```

By default, the server will run on port `8080`.

### Custom Port and Host

To run the server on a specified host and port, use:

```bash
make
./server {host} {port}
```

- `{host}`: The IP address the server will bind to.
- `{port}`: The port number the server will listen on.

---

## Client

This program is designed to use a telnet client.

### Connecting to the Server

To connect to the server, run:

```bash
telnet {ip} {port}
```

- `{ip}`: The IP address of the server.
- `{port}`: The port number of the server.

---

## Commands

Once connected through a telnet client, use the following command to join a chatroom:

```bash
join {roomname} {username}
```

- `{roomname}`: The name of the chatroom you want to join.
- `{username}`: Your desired username.

After joining, you can freely communicate with other users in the same chatroom.

---

Enjoy chatting!