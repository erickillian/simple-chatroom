# Simple Chatroom programmed in C

## Server

To run a chatroom server run the following commands
```
make
./server
```
This will by default run the server on port 8080

To run the server on a specified port run 
```
make
./server {host} {port}
```
{host} is the ip address the server will run on
{port} is the port that the server will run on

## Client

This program is designed to use a telnet client

To connect to the server run 
```
telnet {ip} {port}
```
where {ip}, {port} is the designated ip address, port of the server

## Commands

Once connected through a telnet client run the command
```
JOIN {ROOM} {USERNAME}
```
This will allow you to join a room with name {ROOM} with a username of {USERNAME}

You can then freely communicate in the chat room with other users in the room