#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define on_error(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

int main(int argc, char* argv[]) {
    if (argc < 2) on_error("Usage: %s [port]\n", argv[0]); // input: ./TCPserver port-num

    int port = atoi(argv[1]);

    int server_fd, client_fd, err;
    struct sockaddr_in server, client;
    char buf[1024];
    char temp[20];

    server_fd = socket(AF_INET, SOCK_STREAM, 0); // create socket
    if (server_fd < 0) on_error("Could not create socket\n");

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

    err = bind(server_fd, (struct sockaddr*)&server, sizeof(server)); // binding
    if (err < 0) on_error("Could not bind socket\n");

    err = listen(server_fd, 128);
    if (err < 0) on_error("Could not listen on socket\n");

    printf("Server is listening on %d\n", port);

    while (1) {
        socklen_t client_len = sizeof(client);
        client_fd = accept(server_fd, (struct sockaddr*)&client, &client_len);

        inet_ntop(AF_INET, &client.sin_addr.s_addr, temp, sizeof(temp));
        printf("Server : %s client connected \n", temp);

        if (client_fd < 0) on_error("Could not establish new connection\n");

        while (1) {
            int read = recv(client_fd, buf, 1024, 0);
            printf("Received message: %s\n", buf); // print received message from client

            if (!read) break; // reading end
            if (read < 0) on_error("Client read failed\n");

            err = send(client_fd, buf, read, 0);
            if (err < 0) on_error("Client write failed\n");
        }

        printf("Server : %s client closed \n", temp); // connection end
    }


    return 0;
}
