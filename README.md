# TPReseau4IF
Le code ici présent correspond aux TP3 et 4 de communication entre clients et serveurs en C.
Nous sommes partis des codes présents dans "Exemple2" sur Moodle et avons ajouté des fonctionnalités supplémentaires que nous détaillerons plus bas. 

Voici les choses à savoir sur le bon fonctionnement du code : 

COMPILATION : 

Pour compiler, rendez-vous dans "TPReseau4IF-ANCEL-DADAMO" et tapez "make" pour exécuter le makefile.

LANCEMENT :

Tapez ensuite "cd executables" pour ouvrir le dossier contenant les exécutables "server" et "client".
Depuis ce terminal tapez "./server" pour lancer le server. 
Depuis autant de terminaux que vous souhaitez avoir de client rendez-vous dans "TPReseau4IF-ANCEL-DADAMO/executables" et tapez "./client adresseDeLaMachineDuServer nomDuClient", par exemple pour créer un client nommé Gwen en local cela donne "./client 127.0.0.1 Gwen".

FONCTIONNALITES :

Depuis les clients, vous pouvez taper les commandes suivantes :

-availables : affiche la liste d'utilisateurs actuellement connectés et de groupes actuellement actifs ainsi que le nombre de membres du groupe et s'il s'agit d'un groupe privé ou non.

-create public group NomDuGroupe : crée un groupe public de nom "NomDuGroupe" et ajoute l'utilisateur comme membre. 

-join public group NomDuGroupe : permet de rejoindre un groupe existant si l'on n'est pas déjà membre et que le groupe n'est pas plein. 
