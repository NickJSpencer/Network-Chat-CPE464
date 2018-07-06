# Chat Program

__Commands__
* %M: Send message
* %B: Block messages from a specific handle
* %U: Unblock messages from a specific handle
* %L: Get list of all connected clients to server
* %E: Exit

__Header Format__
* 2 bytes for packet length
* 1 byte for flag

__Flags__
1. client to server: initial packet; includes handle name
2. Server to client: confirmation; accepts handle
3. Server to client: error; denies handle
4. N/A
5. Client to server to client: message packet (%M)
6. N/A
7. Server to client: error; destination handle does not exist
8. Client to server: client exiting
9. Server to client: receipt of client exit (ACK)
10. Client to server: request list of handles (%L)
11. Server to client: # of handles
12. Server to client: one packet per handle on server
13. Server to client: handles done sending
