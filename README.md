# Tema2_PC
Client-Server Application

Protocoale de Comunicatii Tema 2

Aplicatia client-server din cadrul temei 2 a fost realizata complet. Sunt indeplinite toate
functionalitatile de baza descrise in enunt, si partea de server si partea de client, si partea
de store and forward. Ideia de implementare a fost asemanatoare design-paternului Decorator.
Am inceput de la functionalitatea de baza, de a primi mesaje de la clientul udp si de a transmite
acest mesaj tuturor clientilor tcp. Am adaugat posibilitatea de a citi din stdin a instructiunilor
introduse, am adaugat functionalitatea de exit, si de ctrl+c pentru server. Dupa am adaugat 
partea de topicuri, in care un client se poate abona la un singur topic. Am dezvoltat aceasta
parte astfel incat clientul sa se poata abona la mai multe topicuri. Am adaugat partea 
de unsubscribe. Deasemenea in server am realizat salvarea tuturor id-urilor clientilor 
pentru a sti daca clientul conectat este unul nou, sau daca s-a reconectat. Partea complicata
a temei a fost SF si gasirea structurilor care o sa permita memorarea tuturor datelor importante.
Pentru implementare am folosit o structura care memoreaza toti utilizatorii existenti. In cazul
in care un client este deconectat se salveaza toate mesajele transmise acestui client. La 
reconectarea lui se transmit toate mesajele salvate. In final am implementat partea cu algoritmul
Neagle si tratarea cazurilor de eroare.
