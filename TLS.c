#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

/* openssl library */
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>

#define on_error(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX* create_context() {
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX* ctx) {
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int create_socket(int port) {
    int server_fd, err;
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    server_fd = socket(AF_INET, SOCK_STREAM, 0); // create socket
    if (server_fd < 0) on_error("Could not create socket\n");

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

    err = bind(server_fd, (struct sockaddr*) & server, sizeof(server)); // binding
    if (err < 0) on_error("Could not bind socket\n");

    err = listen(server_fd, 128);
    if (err < 0) on_error("Could not listen on socket\n");


    printf("Server is listening on %d\n", port);
}

int main(int argc, char* argv[]) {
    if (argc < 2) on_error("Usage: %s [port]\n", argv[0]); // input: ./TCPserver port-num

    int port = atoi(argv[1]);

    int client_fd;
    struct sockaddr_in client, server;
    char buf[1024];
    char temp[20];

    SSL_CTX* ctx;

    init_openssl();
    ctx = create_context();

    configure_context(ctx);
    int sock = create_socket(port);


    while (1) {
        socklen_t client_len = sizeof(client);
        client_fd = accept(sock, (struct sockaddr*) & client, &client_len);
        SSL* ssl;
        const char test[] = "test\n";

        inet_ntop(AF_INET, &client.sin_addr.s_addr, temp, sizeof(temp));
        printf("Server : %s client connected \n", temp);


        if (client_fd < 0) on_error("Could not establish new connection\n");

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) <= 0) {
            printf("SSL accept error");
        }
        else {
            SSL_write(ssl, test, strlen(test));
        }


        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_fd);
    }
    close(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();

    return 0;
}