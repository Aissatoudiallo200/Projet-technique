#include <stdlib.h> // Pour pouvoir utiliser exit()
#include <stdio.h>  // Pour pouvoir utiliser printf()
#include <math.h>   // Pour pouvoir utiliser sin() et cos()
#include <string.h> // Pour les manipulations de chaînes de caractères
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
 
#include "GfxLib.h" // Seul cet include est nécessaire pour faire du graphique
#include "BmpLib.h" // Cet include permet de manipuler des fichiers BMP
#include "ESLib.h"  // Pour utiliser valeurAleatoire()
 
// Largeur et hauteur par défaut d'une image correspondant à nos critères
#define LargeurFenetre 800
#define HauteurFenetre 400
 
// Port série correct
const char *port = "/dev/ttyACM0"; // Assurez-vous que c'est le bon port pour votre Arduino
int uart0_filestream;
 
// Définition des états de l'interface graphique
enum EtatPage {
    PAGE_ACCUEIL,
    PAGE_CODE
};
 
// État actuel de l'interface graphique
enum EtatPage etatActuel = PAGE_ACCUEIL;
 
// Ouvre la connexion série
void ouvrirConnexionSerie() {
    uart0_filestream = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart0_filestream == -1) {
        printf("Erreur - Impossible d'ouvrir la connexion UART.\n");
        exit(EXIT_FAILURE);
    }
    struct termios options;
    tcgetattr(uart0_filestream, &options);
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);
}
 
// Ferme la connexion série
void fermerConnexionSerie() {
    close(uart0_filestream);
}
 
// Envoie une commande à l'Arduino via la connexion série
void envoyerCommande(const char *commande) {
    ssize_t result = write(uart0_filestream, commande, strlen(commande));
    if (result != (ssize_t)strlen(commande)) {
        perror("Erreur lors de l'envoi de la commande");
    }
}
 
// Fonction pour recevoir et traiter les informations série
void recevoirInformations() {
    if (uart0_filestream != -1) {
        unsigned char rx_buffer[256];
        int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);
 
        if (rx_length > 0) {
            rx_buffer[rx_length] = '\0'; // Terminer la chaîne
            if (strchr((char*)rx_buffer, 'M') != NULL) {
                printf("Réception du caractère 'M'. Changement de page.\n");
                etatActuel = PAGE_CODE; // Passer à l'état de la page demandant le code
            }
        }
    }
}
 
// Fonction de gestion des événements
void gestionEvenement(EvenementGfx evenement) {
    static char code[5] = "abcd";  // Le code connu
    static char chaine[5] = "\0";
    static int nbCharSaisi = 0;
 
    switch (evenement) {
        case Initialisation:
            // Initialisation du système
            ouvrirConnexionSerie();
            demandeTemporisation(20);
            break;
 
        case Temporisation:
            recevoirInformations(); // Appel de la fonction de réception d'informations série
            rafraichisFenetre();
            break;
 
        case Affichage:
            effaceFenetre(255, 255, 255); // Efface la fenêtre avec un fond blanc
 
            // Affichage en fonction de l'état actuel
            switch (etatActuel) {
                case PAGE_ACCUEIL:
                // Afficher la page d'accueil avec "Bonjour"
		        couleurCourante(134, 16, 83); // Bleu
		        epaisseurDeTrait(2);
 
 
		        // Afficher "Bonjour"
            afficheChaine("Bonjour !", 50, 300, 200); // Taille police, x, y
                    break;
                case PAGE_CODE:
                    // Afficher la page demandant le code
                    couleurCourante(0, 76, 153); // Bleu
                    epaisseurDeTrait(2);
                    afficheChaine("Entrez le code", 20, 20, hauteurFenetre() - 20); // Afficher le message de demande de code
                    break;
            }
            break;
        case Clavier:
            printf("%c : ASCII %d\n", caractereClavier(), caractereClavier());
            if (etatActuel == PAGE_CODE) {
                if (nbCharSaisi < 4) {
                    chaine[nbCharSaisi] = caractereClavier();
                    nbCharSaisi++;
                    chaine[nbCharSaisi] = '\0'; // Termine la chaîne
                }
 
                if (nbCharSaisi == 4) {
                    if (strncmp(chaine, code, 4) == 0) {  // Comparer les 4 premiers caractères
                        envoyerCommande("O");  // Envoyer la commande d'ouverture à l'Arduino
                        printf("Code bon. Porte ouverte.\n");
                    } else {
                        envoyerCommande("C");  // Envoyer la commande d'alarme à l'Arduino
                        printf("Mauvais code. Alarme déclenchée.\n");
                    }
                    nbCharSaisi = 0;
                    chaine[0] = '\0';  // Réinitialiser la chaîne
                    etatActuel = PAGE_ACCUEIL; // Revenir à la page d'accueil après avoir entré le code
                }
            }
            break;
 
        case BoutonSouris:
            switch (etatBoutonSouris()) {
                case GaucheAppuye:
                    printf("Bouton gauche appuyé en : (%d, %d)\n", abscisseSouris(), ordonneeSouris());
                    break;
                case GaucheRelache:
                    printf("Bouton gauche relâché en : (%d, %d)\n", abscisseSouris(), ordonneeSouris());
                    break;
                default:
                    break;
            }
            break;
 
        case Redimensionnement:
            printf("Largeur : %d\tHauteur : %d\n", largeurFenetre(), hauteurFenetre());
            break;
        default:
            break;
    }
}
 
// Fonction principale
int main(int argc, char **argv) {
    initialiseGfx(argc, argv);
    prepareFenetreGraphique("Interface", LargeurFenetre, HauteurFenetre);
    lanceBoucleEvenements();
    fermerConnexionSerie();
    return 0;
}