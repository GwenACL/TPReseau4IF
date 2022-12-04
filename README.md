# TPReseau4IF
Le code ici présent correspond aux TP3 et 4 de communication entre clients et serveurs en C.
Nous sommes partis des codes présents dans "Exemple2" sur Moodle et avons ajouté des fonctionnalités supplémentaires que nous détaillerons plus bas. 
-> les trois fonctions permettant d'implémenter itoa() ont été prises sur Internet.

Voici les choses à savoir sur le bon fonctionnement du code : 

COMPILATION : 

Pour compiler, rendez-vous dans "TPReseau4IF-ANCEL-DADAMO" et tapez "make" pour exécuter le makefile.

LANCEMENT :

Tapez ensuite "cd executables" pour ouvrir le dossier contenant les exécutables "server" et "client".
Depuis ce terminal tapez "./server" pour lancer le server. 
Depuis autant de terminaux que vous souhaitez avoir de client rendez-vous dans "TPReseau4IF-ANCEL-DADAMO/executables" et tapez "./client adresseDeLaMachineDuServer nomDuClient", par exemple pour créer un client nommé Gwen en local cela donne "./client 127.0.0.1 Gwen".

FONCTIONNALITES :

Depuis les clients, vous pouvez taper les commandes suivantes :

    INFOS :

-availables : affiche la liste d'utilisateurs actuellement connectés et de groupes actuellement actifs ainsi que le nombre de membres du groupe et s'il s'agit d'un groupe privé ou non.

    GESTION DES GROUPES :

-create public group NomDuGroupe : crée un groupe public de nom "NomDuGroupe" et ajoute l'utilisateur comme membre. 

-join public group NomDuGroupe : permet de rejoindre un groupe public existant si l'on n'est pas déjà membre et que le groupe n'est pas plein. 

-create private group NomDuGroupe MotDePasseDuGroupe : crée un groupe privé de nom "NomDuGroupe" et de mot de passe "MotDePasseDuGroupe" et ajoute l'utilisateur comme membre. 

-join private group NomDuGroupe MotDePasseDuGroupe : permet de rejoindre un groupe privé existant si l'on a le bon mot de passe et que l'on n'est pas déjà membre et que le groupe n'est pas plein. 

    ENVOIE DE MESSAGES :

-@UserName Message : envoie le message "Message" à l'utilisateur de nom UserName s'il est connecté. Sinon l'enregistre quand même dans l'historique. 

-~GroupName Message : envoie le message "Message" à tous les utilisateurs membres du groupe (sauf envoyeur) si le groupe existe et que l'on en fait bien parti.

    BONUS :

-on ne peut pas se connecter en tant que client avec un username déjà pris.
