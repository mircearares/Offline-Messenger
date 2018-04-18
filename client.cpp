#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include "sqlite_library/sqlite3.h"
using namespace std;

extern int errno; /* codul de eroare returnat de anumite apeluri */
int port; /* portul de conectare la server*/

typedef struct infos
{
	char username[30];
	char password[30];
}infos;

int sd;
bool valid_query=false;	
infos utilizator;		

bool logged_ok=false;
int pid;

void registration()
{
	char command[255]="signup";

	if(write(sd, &command, sizeof(command))<=0)
		perror("[client][registration] Eroare la write().\n");

	cout<<"Creati-va un cont\n"
		  "Introduceti un username: ";
	cin>>utilizator.username;

	bool ok_pass=false;
	while(!ok_pass)
	{
		cout<<"Introduceti parola     : ";
		cin>>utilizator.password;
		if(strlen(utilizator.password)<5)
			cout<<"Parola aleasa de dvs. este prea scurta\n"
				  "Va rugam alegeti alta.";
		else
			ok_pass=true;
	}

	if(write(sd, &utilizator, sizeof(utilizator))<=0)
		perror("[client][registration]Eroare la write().\n");

	if(read(sd,&logged_ok,sizeof(bool))<=0)
		perror("[client][registration] Eroare la read().\n");

	if(logged_ok)
		cout<<"Contul a fost creat!\n";
	else
	{
		cout<<"Contul deja este dat in folosinta\n"
			  "Alegeti alt username! \n";
		registration();
	}
}

void login()
{
	char command[255]="login";
	int siz=5;

	if(write(sd, &command, siz)==0)
		perror("[client][registration] Eroare la write().\n");

	cout<<"Introduceti-va datele pentru a va loga la aplicatie!\n"
	      "Introduceti username: ";cin>>utilizator.username;
	cout<<"Introduceti parola  : ";cin>>utilizator.password;

	if(write(sd,&utilizator,sizeof(utilizator))<=0)
		perror("[client][login]Eroare la write().");

	bool ok;
	bool ok_msg;

	if(read(sd,&ok_msg,sizeof(bool))<0)
		perror("[client][login]Eroare la read().");

	if(!ok_msg)
		cout<<"Nu aveti mesaje primite!\n";
	else
	{
		char res[255];
		cout<<"Ati primit urmatoarele mesaje!\n";
		if(read(sd,&res,sizeof(res))<=0)
			perror("[client][login]Eroare la read().");
		cout<<res;
	}

	if(read(sd,&logged_ok,sizeof(bool))<0)
		perror("[client][login]Eroare la read().");

	if(logged_ok)
		cout<<"V-ati logat cu succes.\n";
	else
	{
		char opt[30];
		cout<<"Date acces inexistente/gresite...Alegeti urmatoarele optiuni!\n"
			  "1. Inregistrare\n"
			  "2. Incercati din nou\n";cin>>opt;

		if(strcmp("signup",opt)==0)
			registration();
		else if(strcmp("login",opt)==0)
			login();
	}
}

void logout(infos utilizator)
{
	char command[255]="logout";

	if(write(sd,&command,sizeof(command))<=0)
		perror("[client][logout]Eroare la write().\n");

	if(write(sd,&utilizator,sizeof(utilizator))<=0)
		perror("[client][logout]Eroare la write().\n");

	bool ok;
	if(read(sd, &ok, sizeof(bool))<=0)
		perror("[client][logout]Eroare la read().\n");

	if(ok)
	{
		cout<<"V-ati delogat cu succes\n";
		close(sd);
	}
}

void whosOnline()
{
	char command[255]="online";
	bool ok=true;

	if(write(sd,&command,sizeof(command))<=0)
		perror("[client][whosOnline]Eroare la write().\n");

	char res[255];

	if(read(sd,&ok,sizeof(bool))<=0)
		perror("[client][whosOnline]Eroare la write().\n");

	if(read(sd,&res,sizeof(res))<=0)
		perror("[client][whosOnline]Eroare la write().\n");

	cout<<res<<"\n";
}

void showUsers()
{
	char command[255]="all";
	char res[255];
	bool ok;

	if(write(sd,&command,sizeof(command))<=0)
		perror("[client][showUsers]Eroare la write().\n");

	if(read(sd,&res,sizeof(res))<=0)
		perror("[client][showUsers]Eroare la write().");

	if(read(sd,&ok,sizeof(bool))<=0)
		perror("[client][showUsers]Eroare la write().");

	if(ok)
		cout<<res<<"\n";
	else
		cout<<"Eroare la citirea userilor\n";
}


void reply(infos utilizator)
{
	bool ok;
	char nr_message[30];
	char target[30];
	char message[255];
	char command[255]="reply";

	if(write(sd,&command,sizeof(command))<=0)
		perror("[client][reply]Eroare la write().\n");

	cin.getline( nr_message, 30, '.');
	cin.getline( message, 255, '.' );

	if(write(sd,&nr_message,sizeof(nr_message))<=0)
		perror("[client][reply]Eroare la write().");

	if(write(sd,&message,sizeof(message))<=0)
		perror("[client][reply]Eroare la write().");

	if(read(sd,&ok,sizeof(bool))<=0)
		perror("[client][reply]Eroare la write().\n");

	if(ok)
		cout<<"\n Raspuns trimis cu succes.\n";
	else
		cout<<"\n Raspunsul nu a fost trimis cu succes \n";
}

void history_user(infos utilizator)
{
	char target[30];
	char res[255];
	char command[255]="history_user";

	if(write(sd,&command,sizeof(command))<=0)
		perror("[client][history]Eroare la write().\n");

	if(write(sd,&utilizator,sizeof(utilizator))<=0)
		perror("[client][history]Eroare la write().\n");

	if(read(sd,&res,sizeof(res))<=0)
		perror("[client][history]Eroare la write().\n");

	cout<<res<<"\n";
}

void send(infos utilizator)
{
	bool ok;
	char target_message[258];
	char target[30];
	char message[255];
	char command[255]="send";

	if(write(sd,&command,sizeof(command))<=0)
		perror("[client][send]Eroare la write().\n");

	if(write(sd,&utilizator,sizeof(utilizator))<=0)
		perror("[client][send]Eroare la write().\n");

	cin>>target;

	if(write(sd,&target,sizeof(target))<=0)
		perror("[client][send]Eroare la write().\n");

	cin.getline( message, 255, '.' );

	if(write(sd,&message,strlen(message))<=0)
		perror("[client][send]Eroare la write().\n");

}

void history_with_user(infos utilizator)
{
	char target[30];
	char res[255];
	char command[255]="history_with_user";
	bool ok=false;

	if(write(sd,&command,sizeof(command))<=0)
		perror("[client][history_with_user]Eroare la write().\n");

	cin>>target;

	if(write(sd,&target,sizeof(target))<=0)
		perror("[client][history_with_user]Eroare la write().\n");

	if(read(sd,&ok,sizeof(bool))<=0)
		perror("[client][history_with_user]Eroare la read().\n");

	if(ok)
	{
		cout<<"Exista Mesaje\n";
		if(read(sd,&res,sizeof(res))<=0)
			perror("[client][history_with_user]Eroare la read().\n");
		cout<<res<<"\n";
	}
	else 
		cout<<"Nu exista!\n";


}

void main_menu()
{
	char option_1[255];
	char option_2[255];
	cout<<"          Bine ati venit pe Offline Messenger\n"
	      "          \n                                 \n"
	      "Pentru a continua, trebuie sa va logati sau inregistrati\n"
	      "1 ...... Inregistrare(signup)\n"
	      "2 ...... Login(login)\n";cin>>option_1;


	while(!logged_ok)
	{
		if(strcmp("signup",option_1)==0)
			registration();
	
		else if(strcmp("login",option_1)==0)
			login();			
	}
	

	cout<<"          Alegeti una din urmatoarele comenzi\n"
	      "1 ...... Istoricul(history_user)                           \n"
	      "2 ...... Persoane online(online)                    \n"
	      "3 ...... Trimitere mesaj(send)                     \n"
	      "4 ...... Raspuns mesaj(reply)                       \n"
	      "5 ...... Stergere Istoric(history_with_user)   \n"
	      "6 ...... Afisare toti utilizatori(all)                \n"
	      "7 ...... Logout(logout)                          \n"
	      "Comanda dvs?: ";

	pid=fork();	

	while(1)
	{
    	if(pid==0)
    	{
    		char res[255];
    		read(sd, &res, 255);
    		cout<<res<<'\n';
    	}
    	else
    	{
	        cin>>option_2;
			if(strcmp("history_user",option_2)==0)
				history_user(utilizator);
			else if(strcmp("online",option_2)==0)
				whosOnline();
			else if(strcmp("send",option_2)==0)
				send(utilizator);
			else if(strcmp("all",option_2)==0)
				showUsers();
			else if(strcmp("logout",option_2)==0)
				logout(utilizator);
			else if(strcmp("reply",option_2)==0)
				reply(utilizator);
			else if(strcmp("history_with_user",option_2)==0)
				history_with_user(utilizator);

			bzero(option_2,sizeof(option_2));
		}
	}     
}

int main (int argc, char *argv[])
{
    struct sockaddr_in server;	// structura folosita pentru conectare 
    char buf[10];

    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi (argv[2]);     /* stabilim portul */

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)  /* cream socketul */
    {
        perror ("Eroare la socket().\n");
        return errno;
    }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    server.sin_family = AF_INET;    /* familia socket-ului */
    server.sin_addr.s_addr = inet_addr(argv[1]); /* adresa IP a serverului */
    server.sin_port = htons (port);     /* portul de conectare */
  
    /* ne conectam la server */
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
        perror ("[client]Eroare la connect().\n");
        return errno;
    }
   
   		 main_menu();

    close (sd); 
}
