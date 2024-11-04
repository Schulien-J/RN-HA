 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>
 #include <string.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <netdb.h>

 void init_hints(struct addrinfo *hints){

    memset(hints, 0, sizeof (struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_PASSIVE;

}

 int main(int argc, char *argv[]) {
    if (argc != 3) {
        exit(EXIT_FAILURE);
    }

    char *ip_address = argv[1];
    char *port = argv[2];
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int yes = 1;
    int rv;

    init_hints(&hints);  // Speicher auf 0 gesetzt  und addrinfo auf die für uns relevanten tools gesetzt

    if ((rv = getaddrinfo(ip_address, port, &hints, &servinfo)) != 0) { //füllt die addrinfo aus
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {    // iteriert durch alle geufundenen Infos bis eine funzt
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue; //wenn das failt ist der rest auch egal
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            close(sockfd);
            exit(2);  // not quite sure ob ich hier nicht auch continuen sollte aber es geht so bisher
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("bind");
            close(sockfd);

            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {   // nix gefindet ?
        exit(3);
    }

    if (listen(sockfd, 100) == -1) {   //backlog = 100
        perror("listen");
        close(sockfd);
        exit(4);
    }

    while (1) { // server bleibt laufen für die tests

        struct sockaddr_storage client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size); //neuer socket nur für diese verbindung // neue verbindung
        if (new_fd == -1) {
            perror("accept");
            continue;
        }


        char buffer[9000];  // random gorße Zahö
        ssize_t bytes_received = recv(new_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == -1) {
            perror("recv");
            close(new_fd);
            continue;
        }

        const char *response = "Reply";
        ssize_t bytes_sent = send(new_fd, response, strlen(response), 0);  // was passiert wenn wir es iwie nicht schaffen das Paket zu senden ? ist sehr sehr sehr unlikely sp I didn't give a shit
        if (bytes_sent == -1) {
            perror("send");
        }

        close(new_fd);
    }
    close(sockfd);
    return 0;
}