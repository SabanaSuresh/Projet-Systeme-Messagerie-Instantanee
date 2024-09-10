#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>  

#define SOCKET_PATH "./MySock"
#define BUFFER_SIZE 1024

//Fonction pour le thread qui permet aux clients de recevoir les messages envoyés
void recv_handler(void *arg) {
    int client= *((int *)arg);
    char message[BUFFER_SIZE] = {};
    while (1) {
        int receive = recv(client, message, BUFFER_SIZE, 0);
        if (receive > 0) {
            // Dès qu'un client reçoit un message il l'affiche
            printf("%s", message);
            fflush(stdout); 
        } else if (receive == 0) {
            break;
        } else {
           
        }
        memset(message, 0, BUFFER_SIZE);
    }
    pthread_exit(NULL);
}

//Fonction pour le thread qui permet aux clients d'envoyer des messages 
void send_handler(void *arg) {
    int client= *((int *)arg);
    char message[BUFFER_SIZE] = {};
    char buffer[BUFFER_SIZE + 32] = {};

    while (1) {
        printf(">");
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = '\0';  

        if (strcmp(message, "exit") == 0) {
            break;
        } else {
            //Envoyer le message que le client vient d'écrire au serveur
            sprintf(buffer, "%s\n", message);
            send(client, buffer, strlen(buffer), 0);
        }

        memset(message, 0, BUFFER_SIZE);
        memset(buffer, 0, BUFFER_SIZE + 32);
    }
    shutdown(client,SHUT_RDWR);
    close(client);
    pthread_exit(NULL);
}

int main() {
    pthread_t receive_thread, send_thread;
    int client_socket; // Fichier descripteur pour le socket client 

   // Initialisation de la structure d'adresse du serveur
	struct sockaddr_un saddr = {0}; 	// Structure d'adresse du serveur
	saddr.sun_family = AF_UNIX;		// Addresse Family UNIX 
	strcpy(saddr.sun_path, "./MySock");	// Chemin vers le fichier socket


    // Création du socket client
    client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_socket== -1) {
        perror("Erreure lors de la création du socket");
        exit(EXIT_FAILURE);
    }
    // Changer les "flags" du sockets pour que les fonctions "send" et "recv" soient débloquées
    int flags = fcntl(client_socket, F_GETFL, 0);
    if (flags == -1) {
        perror("Erreure d'extraction des flags du socket.\n");
        exit(1);
    }
    if (fcntl(client_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("Erreure: socket pas mit en mode débloquant.\n");
        exit(1);
    }


    // Tentative de connexion au serveur, connect renvoi -1 en cas d'erreur
    if (connect(client_socket, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
        perror("Erreure de connexion");
        exit(EXIT_FAILURE);
    }

    printf("Connecté au serveur. Vous pouvez commencer à envoyer des messages.\n");
    printf("'Exit' pour se déconnecter\n");

    // Création des threads pour envoyer et recevoir des messages
    if (pthread_create(&send_thread, NULL, (void *)send_handler, &client_socket) != 0) {
        perror("Erreure lors de la création du thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&receive_thread, NULL, (void *)recv_handler, &client_socket) != 0) {
        perror("Erreure lors de la création du thread");
        exit(EXIT_FAILURE);
    }

    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

    // Fermeture du service et fermeture du socket/serveur
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);

    return 0;
}
