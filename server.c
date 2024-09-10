#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/un.h>

#define MAX_USERS 10
#define MAX_MESSAGES 100
#define BUFFER_SIZE 1024
#define SOCKET_PATH "./MySock"
pthread_mutex_t mutex_id = PTHREAD_MUTEX_INITIALIZER;

// Matrice pour stocker les messages et le nombre de messages 
typedef struct {
    char messages[MAX_MESSAGES][BUFFER_SIZE];
    int nbr_message;
} UserMessages;


// Déclaration du tableau des sockets clients
int client_sockets[MAX_USERS];

// Nombre de clients
int num_clients = 0;

// Tableau des messages par utilisateur
UserMessages user_messages[MAX_USERS];

// Fonction pour envoyer les messages à tous
void send_to_all(char *message, int indice) {
    char full_message[BUFFER_SIZE];
    sprintf(full_message, "User %d: %s", indice, message);
    for (int j = 0; j < num_clients; j++) {
        //sauf au client qui a émis le message
        if (j != indice) {
            send(client_sockets[j], full_message, strlen(full_message) + 1, 0);
        }
    }
}

//Fonction pour sauvegarder tous les messages qui ont été envoyés lors d'une session.
void historique(indice) {
    FILE *fp;
    fp = fopen("Historique.txt", "a+");
    if (fp == NULL) {
        perror("Erreure lors de l'ouverture du fichier.\n");
        return;
    }

    fprintf(fp, "Messages du client %d:\n", indice);
     for (int j = 0; j < user_messages[indice].nbr_message; j++) {
        fprintf(fp, "%s\n", user_messages[indice].messages[j]);
    }
    fprintf(fp, "\n");
    fclose(fp);
}

//Fonction pour les threads
void *client_handler(void *arg) {
    int client_socket = *((int *)arg);
    char message[BUFFER_SIZE];
    // Utilisation d'un mutex pour qu'un seul client puisse modifier le nombre de clients
    pthread_mutex_lock(&mutex_id); 
    int client_index = num_clients;
    client_sockets[client_index] = client_socket;
    num_clients++;
    pthread_mutex_unlock(&mutex_id);

   // Communication avec le client
    while (strcmp(message,"exit")!=0) {
        memset(message, '\0', BUFFER_SIZE);
        int recu = recv(client_socket, message, BUFFER_SIZE, 0);
        if (recu <= 0) {
            printf("Client déconnecté.\n");
            break;
        }
        printf("Message recu: %s\n", message);
        strncpy(user_messages[client_index].messages[user_messages[client_index].nbr_message], message, BUFFER_SIZE);
        user_messages[client_index].nbr_message++;
        // Envoyer le message à tout le monde
        send_to_all(message,client_index);
       
    }
    // Fermeture du socket client
    historique(client_index);
    shutdown(client_socket,SHUT_RDWR);
    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    pthread_t threads[MAX_USERS];
    socklen_t caddrlen;  
    // Vider le socket à chaque nouvelle session
    unlink(SOCKET_PATH);

    // Sauvegarder les messages dans un fichier et indiquer à chaque fois lorsqu'il y a une nouvelle connexion
    FILE *fp = fopen("Historique.txt", "a+");
    if (fp == NULL) {
        perror("Erreure lors de l'ouverture du fichier.\n");
        exit(1);
    }
    fprintf(fp, "Nouvelle session de connexion:\n");
    fclose(fp); 
    // Initialiser les structures de messages par utilisateur
    for (int i = 0; i < MAX_USERS; i++) {
        user_messages[i].nbr_message = 0;
    }

    // Initialisation de la structure d'adresse du serveur
    struct sockaddr_un saddr = {0};     
    saddr.sun_family = AF_UNIX;
    strcpy(saddr.sun_path, SOCKET_PATH);


    // Initialisation de la structure d'adresse du client
    struct sockaddr_un caddr = {0};     

    // Création du socket serveur
    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_socket == -1) {
        perror("Erreure de création de socket.\n");
        exit(1);
    }   

    // Connexion du socket à l'adresse serveur
    bind(server_socket, (struct sockaddr *)&saddr,  sizeof(saddr));
    //Socket en écoute
    listen(server_socket, 5);

    printf("Serveur en attente de connexions...\n");
    sleep(5);

    while (1) {
        // Accepte les connexions 
        caddrlen = sizeof(caddr);       
        client_socket = accept(server_socket, (struct sockaddr *)&caddr ,&caddrlen);
        if (client_socket == -1) {
            perror("Erreure lors de la connexion.\n");
            continue;
        }
        printf("Nouvelle connexion acceptée.\n");
        // Création du thread de la connexion
        if (pthread_create(&threads[num_clients], NULL, client_handler, &client_socket) != 0) {
            perror("Erreure de création de thread.\n");
            close(client_socket);
            continue;
        }

    
    }
    
    for (int i = 0; i < num_clients; i++) {
        pthread_join(threads[i], NULL);
    }

    // Fermerture du socket du serveur
    close(server_socket);
    unlink(SOCKET_PATH);
    return 0;
}