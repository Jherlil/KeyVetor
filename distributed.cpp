#include "distributed.h"
#include "secp256k1/Int.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET sock_t;
#define CLOSESOCK closesocket
static void net_init(){ WSADATA ws; WSAStartup(MAKEWORD(2,2), &ws); }
static void net_cleanup(){ WSACleanup(); }
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
typedef int sock_t;
#define CLOSESOCK close
static void net_init(){}
static void net_cleanup(){}
#endif

static void send_range(int sock, const char *s, const char *e) {
    char buf[160];
    snprintf(buf, sizeof(buf), "RANGE %s %s\n", s, e);
    send(sock, buf, strlen(buf), 0);
}

static void send_done(int sock) {
    const char *msg = "DONE\n";
    send(sock, msg, strlen(msg), 0);
}

static Int g_next;
static Int g_end;
static Int g_step;

static void init_ranges(const char *start_hex,
                        const char *end_hex,
                        unsigned shard_bits) {
    g_next.SetBase16(start_hex);
    g_end.SetBase16(end_hex);
    g_step.SetInt64(1ULL << shard_bits);
}

static int allocate_range(char **start_hex, char **end_hex) {
    if(g_next.IsGreater(&g_end))
        return 0;

    Int start(&g_next);
    Int end(&g_next);
    end.Add(&g_step);
    end.SubOne();
    if(end.IsGreater(&g_end))
        end.Set(&g_end);

    g_next.Set(&end);
    g_next.AddOne();

    *start_hex = start.GetBase16();
    *end_hex = end.GetBase16();
    return 1;
}

int run_coordinator(const char *port_str,
                    const char *start_hex,
                    const char *end_hex,
                    unsigned shard_bits) {
    init_ranges(start_hex, end_hex, shard_bits);

    net_init();
    int port = atoi(port_str);
    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    if(srv == (sock_t)-1) { perror("socket"); net_cleanup(); return 1; }
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if(bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if(listen(srv, 8) < 0) { perror("listen"); return 1; }
    printf("[+] Coordinator listening on port %d\n", port);
    while(1) {
        sock_t cli = accept(srv, NULL, NULL);
        if(cli == (sock_t)-1) continue;
        char *s = NULL, *e = NULL;
        if(allocate_range(&s, &e)) {
            send_range(cli, s, e);
        } else {
            send_done(cli);
        }
        free(s);
        free(e);
        CLOSESOCK(cli);
    }
    net_cleanup();
    return 0;
}

static void parse_hp(const char *hp, char *host, int *port) {
    const char *colon = strchr(hp, ':');
    if(colon) {
        size_t n = colon - hp;
        strncpy(host, hp, n);
        host[n] = 0;
        *port = atoi(colon+1);
    } else {
        strcpy(host, hp);
        *port = 0;
    }
}

int run_worker(const char *host_port,
               const char *default_port,
               char **out_start,
               char **out_end) {
    char host[64]; int port;
    parse_hp(host_port, host, &port);
    if(port == 0) port = atoi(default_port);
    net_init();
    sock_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == (sock_t)-1) { perror("socket"); net_cleanup(); return 1; }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(inet_aton(host, &addr.sin_addr) == 0) { fprintf(stderr, "invalid host %s\n", host); return 1; }
    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); return 1; }

    char buf[160] = {0};
    int n = recv(sock, buf, sizeof(buf)-1, 0);
    if(n <= 0) { perror("recv"); CLOSESOCK(sock); net_cleanup(); return 1; }
    buf[n] = '\0';

    if(strncmp(buf, "RANGE", 5) == 0) {
        char *tok = strtok(buf + 6, " ");
        if(tok) {
            *out_start = strdup(tok);
            tok = strtok(NULL, " \n");
            if(tok)
                *out_end = strdup(tok);
        }
        printf("[+] Range %s - %s\n", *out_start ? *out_start : "?", *out_end ? *out_end : "?");
    } else if(strncmp(buf, "DONE", 4) == 0) {
        printf("[+] Coordinator reports completion\n");
    } else {
        fprintf(stderr, "[E] Unknown message %s\n", buf);
    }

    CLOSESOCK(sock);
    net_cleanup();
    return 0;
}
