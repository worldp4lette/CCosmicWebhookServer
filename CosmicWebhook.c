#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 8192

void handle_webhook(int client_fd, struct sockaddr_in6 *client_addr)
{
	printf("Handling request...\n");
	
	char buffer[BUFFER_SIZE];
	int bytes_read;

	// Receive data
	bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
	if (bytes_read < 0)
	{
		perror("recv");
		close(client_fd);
		return;
	}

	// Validate request and execute git pull
	buffer[bytes_read] = '\0';
	if (strstr(buffer, "X-GitHub-Event") != NULL)
	{
		char client_ip[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &client_addr->sin6_addr, client_ip, INET_ADDRSTRLEN);
		
		// TODO: IPv4 IP should also be dealt
		printf("Client IP: %s, Port: %d\n", client_ip, ntohs(client_addr->sin6_port));

		system("cd /home/ubuntu/Cosmic && git pull");
		
		printf("Successfully pulled from repository\n");
	}
}

int main()
{
	// Socket creation
	int sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
	if (sock_fd == -1)
	{
		perror("socket");
		return EXIT_FAILURE;		
	}

	// Socket configuration - Set SO_REUSEADDR flag
	int optval = 1;
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
	{
		perror("setsockopt SO_REUSEADDR");
		close(sock_fd);
		return EXIT_FAILURE;
	}

	// Socket configuration - Allow IPv4 connection too
	optval = 0;
	if (setsockopt(sock_fd, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval)) == -1)
	{
		perror("setsockopt IPV6_V6ONLY");
		close(sock_fd);
		return EXIT_FAILURE;
	}

	// Initialize socket address structure and bind socket to the address
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(PORT);
	addr.sin6_addr = in6addr_any;

	if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("bind");
		close(sock_fd);
		return EXIT_FAILURE;
	}
	
	// Listen on socket
	if (listen(sock_fd, 5) == -1)
	{
		perror("listen");
		close(sock_fd);
		return EXIT_FAILURE;
	}

	// Accept client connection request
	int webhook_client_sock_fd;
	struct sockaddr_in6 webhook_client_addr;
	socklen_t addr_len = sizeof(webhook_client_addr);

	while (true)
	{
		printf("Waiting for a connection...\n");
		
		webhook_client_sock_fd 
			= accept(sock_fd, 
				(struct sockaddr *)&webhook_client_addr, 
				&addr_len);
		if (webhook_client_sock_fd == -1)
		{
			perror("accept");
			continue;
		}

		handle_webhook(webhook_client_sock_fd, &webhook_client_addr);

		close(webhook_client_sock_fd);
	}

	close(sock_fd);

	return 0;
}
