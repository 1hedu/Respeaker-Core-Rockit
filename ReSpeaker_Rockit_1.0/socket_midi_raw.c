#include "socket_midi_raw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define MIDI_STATUS_NOTE_ON 0x90
#define MIDI_STATUS_NOTE_OFF 0x80
#define MIDI_STATUS_CC 0xB0
#define MESSAGE_SIZE 3 // Standard MIDI message size

static pthread_t th;
static int running = 0;
static void (*cb_note_on)(uint8_t) = NULL;
static void (*cb_note_off)(uint8_t) = NULL;
static void (*cb_cc)(uint8_t, uint8_t) = NULL;

static void parse_midi_bytes(const uint8_t* buffer) {
    uint8_t status = buffer[0] & 0xF0; // Get command (ignore channel)
    uint8_t data1 = buffer[1];
    uint8_t data2 = buffer[2];

    if (status == MIDI_STATUS_NOTE_ON) {
        // Note On with velocity 0 is treated as Note Off
        if (data2 > 0) { 
            if (cb_note_on) cb_note_on(data1);
        } else { 
            if (cb_note_off) cb_note_off(data1);
        }
    } else if (status == MIDI_STATUS_NOTE_OFF) {
        if (cb_note_off) cb_note_off(data1);
    } else if (status == MIDI_STATUS_CC) {
        if (cb_cc) cb_cc(data1, data2);
    }
}

static void* socket_thread(void* arg) {
    uint16_t port = *(uint16_t*)arg;
    free(arg); 
    
    int listenfd, connfd;
    struct sockaddr_in serv_addr;
    uint8_t recv_buf[MESSAGE_SIZE];
    int n;
    int optval = 1; 

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    // Set socket options and bind to 127.0.0.1
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Socket bind failed");
        close(listenfd); return NULL;
    }
    if (listen(listenfd, 1) < 0) {
        perror("Socket listen failed");
        close(listenfd); return NULL;
    }

    fprintf(stderr, "RAW MIDI TUNNEL: Listening on 127.0.0.1:%u (3-byte packets)\n", port);

    while (running) {
        // Blocks until a connection is received
        connfd = accept(listenfd, (struct sockaddr*)NULL, (socklen_t*)NULL);
        if (connfd < 0) {
            if (running) perror("Socket accept failed");
            continue;
        }

        // Read exactly 3 bytes (the raw MIDI message)
        n = read(connfd, recv_buf, MESSAGE_SIZE);
        if (n == MESSAGE_SIZE) {
            parse_midi_bytes(recv_buf);
        } else if (n > 0) {
            fprintf(stderr, "Warning: Received %d bytes, expected 3 (MIDI msg)\n", n);
        }

        close(connfd);
    }

    close(listenfd);
    return NULL;
}

int socket_midi_raw_start(uint16_t port,
                          void (*on_note_on)(uint8_t), 
                          void (*on_note_off)(uint8_t),
                          void (*on_cc)(uint8_t, uint8_t)) {
    if (running) return 0;
    
    cb_note_on = on_note_on; cb_note_off = on_note_off; cb_cc = on_cc;
    running = 1;
    
    uint16_t* port_arg = (uint16_t*)malloc(sizeof(uint16_t));
    if (!port_arg) return -1;
    *port_arg = port;
    
    if (pthread_create(&th, NULL, socket_thread, port_arg) != 0) {
        perror("Socket thread creation failed");
        free(port_arg); running = 0; return -1;
    }
    return 0;
}

void socket_midi_raw_stop(void) {
    if (running) {
        running = 0;
        pthread_join(th, NULL);
    }
}