/**
 * Fast C-based MIDI Bridge for ReSpeaker Rockit
 * Replaces slow Python HTTP server with lightweight C implementation
 *
 * Listens on HTTP port 8090, forwards MIDI to TCP port 50000
 * Minimal HTTP parsing for maximum performance on embedded MIPS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

#define HTTP_PORT 8090
#define MIDI_PORT 50000
#define MIDI_HOST "127.0.0.1"
#define BUFFER_SIZE 2048

static int http_sock = -1;
static int midi_sock = -1;
static volatile int running = 1;

void signal_handler(int sig) {
    running = 0;
}

// Connect to MIDI TCP server
int connect_midi(void) {
    struct sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("MIDI socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MIDI_PORT);
    inet_pton(AF_INET, MIDI_HOST, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("MIDI connect");
        close(sock);
        return -1;
    }

    fprintf(stderr, "Connected to MIDI server on %s:%d\n", MIDI_HOST, MIDI_PORT);
    return sock;
}

// Send 3-byte raw MIDI message
int send_midi(int sock, unsigned char status, unsigned char data1, unsigned char data2) {
    unsigned char msg[3] = {status, data1, data2};
    if (send(sock, msg, 3, MSG_NOSIGNAL) != 3) {
        perror("MIDI send");
        return -1;
    }
    return 0;
}

// Parse query parameter from URL
int get_param(const char *url, const char *name, int default_val) {
    char search[32];
    snprintf(search, sizeof(search), "%s=", name);

    const char *p = strstr(url, search);
    if (!p) return default_val;

    p += strlen(search);
    return atoi(p);
}

// Check if URL starts with given path
int url_match(const char *url, const char *path) {
    return strncmp(url, path, strlen(path)) == 0;
}

// Handle HTTP request
void handle_request(int client_sock, const char *request) {
    char response[256];

    // Handle OPTIONS method for CORS preflight
    if (strncmp(request, "OPTIONS", 7) == 0) {
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n\r\n");
        send(client_sock, response, strlen(response), MSG_NOSIGNAL);
        return;
    }

    // Find GET/POST line
    const char *line_end = strchr(request, '\n');
    if (!line_end) return;

    // Extract URL (between first space and second space)
    const char *url_start = strchr(request, ' ');
    if (!url_start) return;
    url_start++;

    const char *url_end = strchr(url_start, ' ');
    if (!url_end) return;

    char url[256];
    int url_len = url_end - url_start;
    if (url_len >= sizeof(url)) url_len = sizeof(url) - 1;
    memcpy(url, url_start, url_len);
    url[url_len] = '\0';

    // Handle different endpoints
    if (url_match(url, "/cc?")) {
        int cc = get_param(url, "cc", -1);
        int value = get_param(url, "value", -1);

        if (cc >= 0 && cc <= 127 && value >= 0 && value <= 127) {
            if (midi_sock >= 0) {
                // MIDI CC: Status=0xB0 (channel 0), CC#, Value
                if (send_midi(midi_sock, 0xB0, cc, value) == 0) {
                    snprintf(response, sizeof(response),
                        "HTTP/1.1 200 OK\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 2\r\n\r\nOK");
                    send(client_sock, response, strlen(response), MSG_NOSIGNAL);
                    return;
                }
            }
        }
    }
    else if (url_match(url, "/note?")) {
        int note = get_param(url, "note", -1);
        int velocity = get_param(url, "velocity", 100);

        // Check for action=on or action=off
        int is_on = strstr(url, "action=on") != NULL;

        if (note >= 0 && note <= 127 && velocity >= 0 && velocity <= 127) {
            if (midi_sock >= 0) {
                unsigned char status = is_on ? 0x90 : 0x80;  // Note On/Off
                if (send_midi(midi_sock, status, note, velocity) == 0) {
                    snprintf(response, sizeof(response),
                        "HTTP/1.1 200 OK\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 2\r\n\r\nOK");
                    send(client_sock, response, strlen(response), MSG_NOSIGNAL);
                    return;
                }
            }
        }
    }
    else if (url_match(url, "/status")) {
        const char *status = (midi_sock >= 0) ? "OK" : "disconnected";
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n\r\n%s",
            strlen(status), status);
        send(client_sock, response, strlen(response), MSG_NOSIGNAL);
        return;
    }
    else if (url_match(url, "/panic")) {
        // Send Note Off for all 128 MIDI notes
        if (midi_sock >= 0) {
            int i;
            for (i = 0; i < 128; i++) {
                send_midi(midi_sock, 0x80, i, 0);  // Note Off for note i
            }
        }
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 2\r\n\r\nOK");
        send(client_sock, response, strlen(response), MSG_NOSIGNAL);
        return;
    }

    // Default 404
    snprintf(response, sizeof(response),
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n\r\n");
    send(client_sock, response, strlen(response), MSG_NOSIGNAL);
}

int main(void) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    fprintf(stderr, "Rockit MIDI Bridge (C) starting...\n");

    // Connect to MIDI server
    midi_sock = connect_midi();
    if (midi_sock < 0) {
        fprintf(stderr, "Warning: MIDI server not ready yet\n");
    }

    // Create HTTP server socket
    http_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (http_sock < 0) {
        perror("HTTP socket");
        return 1;
    }

    // Allow address reuse
    int opt = 1;
    setsockopt(http_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(HTTP_PORT);

    if (bind(http_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("HTTP bind");
        fprintf(stderr, "Port %d may already be in use. Kill existing process?\n", HTTP_PORT);
        close(http_sock);
        return 1;
    }

    if (listen(http_sock, 5) < 0) {
        perror("HTTP listen");
        close(http_sock);
        return 1;
    }

    fprintf(stderr, "HTTP server listening on port %d\n", HTTP_PORT);
    fprintf(stderr, "Forwarding MIDI to %s:%d\n", MIDI_HOST, MIDI_PORT);
    fprintf(stderr, "Press Ctrl+C to stop...\n");

    while (running) {
        client_len = sizeof(client_addr);
        int client_sock = accept(http_sock, (struct sockaddr*)&client_addr, &client_len);

        if (client_sock < 0) {
            if (running) perror("accept");
            continue;
        }

        // Reconnect MIDI if disconnected
        if (midi_sock < 0) {
            midi_sock = connect_midi();
        }

        // Read HTTP request (non-blocking would be better but this is simple)
        ssize_t n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            handle_request(client_sock, buffer);
        }

        close(client_sock);
    }

    fprintf(stderr, "\nShutting down...\n");
    if (http_sock >= 0) close(http_sock);
    if (midi_sock >= 0) close(midi_sock);

    return 0;
}
