
# Awalé — Projet Réseau

Projet qui instancie un server single-thread pour jouer à l'Awalé en réseau (C, POSIX sockets).

## Construction
Dépendances : compilateur C compatible C11 (gcc), make, outils POSIX.

- Construire le serveur et le client
  - make
  - Produits : `bin/server` et `bin/client`

- Nettoyage
  - make clean

## Exécution
1. Lancer le serveur (terminal ou ordinateur A) :
   - ./bin/server
   - Message attendu : `Server listening on port 1977` (ou le port dans #define)

2. Lancer un client (terminal ou ordinateur B) :
   - ./bin/client 127.0.0.1
   - Message attendu : `Connected to server 127.0.0.1`


## Commandes client (exemples)
- !signup <username> <password>  — créer un compte
- !login <username> <password>   — se connecter
- !commands                       — lister les commandes disponibles
- !challenge <playername>         — défier un joueur
- !accept                         — accepter un défi reçu
- /play <1..6>                    — jouer un coup (numéro relatif selon votre camp)
- !private / !public              — activer/désactiver le mode privé
- !friends / !friends reqs        — gérer amis

Remarque : la syntaxe exacte (préfixes `!` ou `/`) doit être respectée par le client.

## Affichage du plateau
- Le serveur imprime le plateau toujours dans le même format :
  - Ligne du haut : pits 12 11 10 9 8 7 (affichage en ordre voulu)
  - Séparateur
  - Ligne du bas : pits 1 2 3 4 5 6

Les fonctions gérant le plateau : `boardStartGame`, `boardIsMoveLegal`, `boardMove`. Le serveur synchronise `ServerMatch.joueur` avec `Board.whoseTurn` après chaque coup.

## Structure du dépôt (principaux fichiers)
- server/
  - server.c / server.h     — serveur réseau et gestion clients
  - client_server_side.h    — structure Client
- client/
  - client.c / client.h     — client réseau
- server_match.c / server_match.h — adaptation des matchs pour le serveur
- board.c / board.h - gestion de la logique du jeu AWALE
- Makefile

## Tests rapides
- Démarrer serveur ; deux clients se connectent ; l'un défie l'autre ; accepter ; jouer `/play 1`..`/play 6` en respectant les tours.

# Utilisation d'IA
L'IA à été utilisée, dans le cadre de notre projet pour:
- La création du README
- La génération de quelques commentaires dans le code
- La cŕeation du Makefile
- La CONFIRMATION de la logique du code (rarement du développement)
- La recherche des fonctions et constantes à utiliser (par exemple pour écrire et read dans les sockets, quels paramètres attendus, etc.)

Nous avons à un maximum évité d'utiliser l'IA sans comprendre et relire ligne à ligne ce qui était envoyé, étant donné que le but du TP c'est d'apprendre en pratique les concepts (mais non pas forcément les fonctions et paramètres exactes) de la Programmation Réseau.

# Difficultés rencontrées
- Le temps, particulièrement l'organisation des séances (qui étaient à un jour d'interval, et ce jour était les RIF)

- L'organisation de la structure du code. Au début, nous avions developpé un player.c et un match.c sans trop réfléchir à comment ça changerait d'envoyer les données sur le réseau. Ce TP nous a appris que les façons de faire en local et réseau peuvent entraîner un reworking du code qu'on avait pas forcément prévu.