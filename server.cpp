#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <string>
#include "sqlite3.h"
#define PORT 2018
using namespace std;
extern int errno;

sqlite3 *database;
sqlite3_stmt *statement;
int response=sqlite3_open("bazaDeDate.db",&database);
 fd_set fds,active_rfds,active_wfds;
 int maxfds;
typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
}thData;
struct thData *td; 

typedef struct infos{
	char username[30];
	char password[30];
}infos;

infos utilizator;

void query(char sql[],char res[], bool &ok, bool flag)
{
	char *db_errors={0};
	int size=strlen(res);
	memset(res,0,size);
	cout<<sql<<"\n";

	if(sqlite3_prepare_v2(database,sql,-1,&statement,NULL)==SQLITE_OK)
	{
		int column=sqlite3_column_count(statement);
		int result=0;
		if(!flag)
			ok=true;			
		while(1)
		{
			result=sqlite3_step(statement);
			if(result==SQLITE_ROW)
			{
				for(int i=0;i<column;i++)
				{
					strcat(res, (char*)sqlite3_column_text(statement,i));
					strcat(res," ");
					
				}
				if(flag)
						ok=true;
				cout<<res<<"\n";
				strcat(res, "\n");
			}
			else 
				break;
		}
		sqlite3_finalize(statement);
	}
	else
	{
		fprintf(stderr, "Nu s-au putut culege date.\n");
		fprintf(stderr, "Eroare SQL: %s\n", db_errors);

		sqlite3_free(db_errors);
	}
}

void registration(int tdcl)
{
	bool ok=false;
	char res[255];
	char sql[255];

	if(read(tdcl, &utilizator, sizeof(utilizator))<=0)
		perror("[server][registration]Eroare la read().");

	cout<<"[Thread"<<td->idThread<<"]utilizator incearca conectarea\n";

	memcpy(sql,"INSERT INTO users VALUES((SELECT COUNT(*)FROM users),'",sizeof("INSERT INTO users VALUES((SELECT COUNT(*)FROM users),'"));
	strcat(sql,utilizator.username);
	strcat(sql,"','");
	strcat(sql,utilizator.password);
	strcat(sql,"',datetime('now'));");

	query(sql,res,ok,false);
	
	ok=false;
	char sql_online[255];
	memcpy(sql_online,"INSERT INTO logged VALUES((SELECT id FROM users WHERE username='",sizeof("INSERT INTO logged VALUES((SELECT id FROM users WHERE username='"));
	strcat(sql_online,utilizator.username);
	strcat(sql_online,"'));");
	cout<<sql_online<<"\n";

	query(sql_online,res,ok,false);
	
	if(write(tdcl, &ok, sizeof(bool))<=0)
		perror("[server][registration]Eroare la write()");
}

void login(int tdcl)
{
	bool ok=false;
	bool logged_in=true;
	char *db_errors={0};
	char sql[255];
	char res[255]={0};

	if(read(tdcl,&utilizator,sizeof(utilizator))<=0)
		perror("[server][login]Eroare la read().");

	memcpy(sql,"SELECT username, password FROM users WHERE username='",255);
	strcat(sql,utilizator.username);
	strcat(sql,"' AND password='");
	strcat(sql,utilizator.password);
	strcat(sql,"';");

	query(sql,res,logged_in,true);

	ok=false;
	char sql_online[255];
	memcpy(sql_online,"INSERT INTO logged VALUES((SELECT id FROM users WHERE username='",255);
	strcat(sql_online,utilizator.username);
	strcat(sql_online,"'));");

	query(sql_online,res,ok,false);
	
	char sql_up[255];
	char cifra[10];
	sprintf(cifra,"%d",tdcl);
	memcpy(sql_up,"UPDATE users SET descriptor='",255);
	strcat(sql_up,cifra);
	strcat(sql_up,"' WHERE username='");
	strcat(sql_up,utilizator.username);
	strcat(sql_up,"';");

	ok=false;
	query(sql_up,res,ok,false);

	char offline[255];
	memcpy(offline,"SELECT user_from, message FROM messages WHERE user_to='",255);
	strcat(offline,utilizator.username);
	strcat(offline,"' AND seen='false';");

	bool ok_msg=false;
	query(offline,res,ok_msg,true);

	char offline_update[255];
	char res_offline[255];

	memcpy(offline_update,"UPDATE messages SET seen='true' WHERE  user_to='",255);
	strcat(offline_update,utilizator.username);
	strcat(offline_update,"';");

	ok=false;
	query(offline_update,res_offline,ok,false);

	if(write(tdcl,&ok_msg,sizeof(bool))<=0)
		perror("[server][login]Eroare la write().");

	if(ok_msg)
		if(write(tdcl,&res,sizeof(res))<=0)
			perror("[server][login]Eroare la write().");
	

	if(write(tdcl,&logged_in,sizeof(bool))<=0)
		perror("[server][login]Eroare la write().");

	cout<<"Raspuns login...\n";
}

void logout(int tdcl)
{ 
	char sql[255];
	char sql_del[255];
	char res[255];
	bool ok=false;

	if(read(tdcl,&utilizator,sizeof(infos))<=0)
		perror("[server][login]Eroare la read().");

	if(response!=SQLITE_OK)
	{
		fprintf(stderr, "Baza de date nu poate fi accesata%s\n", sqlite3_errmsg(database));
		sqlite3_close(database); 
	}

	memcpy(sql, "UPDATE users SET last_seen=datetime('now') WHERE username='",sizeof("UPDATE TABLE users SET last_seen=datetime('now') WHERE username='"));
	strcat(sql, utilizator.username);
	strcat(sql, "';");

	query(sql,res,ok,false);

	memcpy(sql_del, "DELETE FROM logged WHERE id IN (SELECT  id FROM users where username='",sizeof("DELETE FROM logged WHERE id IN (SELECT  id FROM users where username='"));
	strcat(sql_del, utilizator.username);
	strcat(sql_del, "');");

	query(sql_del,res,ok,false);

	if(write(tdcl,&ok,sizeof(bool))<=0)
		perror("[server][login]Eroare la write().");
}

void whosOnline(int tdcl)
{
	char *db_errors={0};
	char sql[255];
	char res[255];
	bool ok=false;

	if(response!=SQLITE_OK)
	{
		fprintf(stderr,"Baza de date nu poate fi accesata %s\n", sqlite3_errmsg(database));
	}

	memcpy(sql, "SELECT DISTINCT id, username FROM users WHERE id IN (SELECT id FROM logged);", sizeof("SELECT DISTINCT id, username FROM users WHERE id IN (SELECT id FROM logged);"));

	query(sql, res, ok,true);

	if(write(tdcl,&ok,sizeof(bool))<=0)
		perror("[server][showUsers]Eroare la write().");

	if(read(tdcl,&res,sizeof(res))<=0)
		perror("[server][showUsers]Eroare la write().");
}

void showUsers(int tdcl)
{
	char *db_errors={0};
	char sql[255];
	char res[255]={0};
	bool ok=false;

	if(response!=SQLITE_OK)
	{
		fprintf(stderr,"Baza de date nu poate fi accesata %s\n", sqlite3_errmsg(database));
	}
	
	memcpy(sql,"SELECT username, id FROM users;",sizeof("SELECT username, id FROM users;"));
	
	query(sql,res,ok,true);
	
	if(write(tdcl,&res,sizeof(res))<=0)
		perror("[server][showUsers]Eroare la write().");

	if(write(tdcl,&ok,sizeof(bool))<=0)
		perror("[server][showUsers]Eroare la write().");
}

void history_user(int tdcl) 
{
	char target[30];
	char sql[255];
	char res[255];
	bool ok=false;

	if(read(tdcl,&utilizator,sizeof(utilizator))<=0)
		perror("[server][history]Eroare la write().\n");

	memcpy(sql,"SELECT ROWID, user_from,message FROM messages WHERE user_to='",255);
	strcat(sql,utilizator.username);
	strcat(sql,"';");

	query(sql,res,ok,true);

	if(write(tdcl,&res,sizeof(res))<=0)
		perror("[server][history]Eroare la write().\n");
}

void history_with_user(int tdcl)
{
	char target[30];
	char res[255];
	bool ok=false;

	if(read(tdcl,&target,sizeof(target))<=0)
		perror("[server][history_with_user]Eroare la read().\n");

	char sql[255];

	memcpy(sql,"SELECT ROWID, user_from, message FROM messages WHERE user_to='",255);
	strcat(sql,utilizator.username);
	strcat(sql,"' AND user_from='");
	strcat(sql,target);
	strcat(sql,"';");

	query(sql, res, ok, true);

	if(write(tdcl,&ok,sizeof(bool))<=0)
		perror("[server][history_with_user]Eroare la write().\n");

	if(ok)
		if(write(tdcl,&res,sizeof(res))<=0)
			perror("[server][history_with_user]Eroare la write().\n");

}

void reply(int tdcl)
{
	char nr_message[30];
	char target[30];
	char sql[255];
	char sql_reply[255];
	char sql_target[255];
	char target_res[255];
	char message[255];
	bool ok=false;
	char res[255];
	char quer_res[255];

	if(read(tdcl,&nr_message,sizeof(nr_message))<=0)
		perror("[server][reply]Eroare la write().");

	if(read(tdcl, &message, sizeof(message))<=0)
		perror("[server][reply]Eroare la read().");	

	if(response!=SQLITE_OK)
	{
		fprintf(stderr,"Baza de date nu poate fi accesata %s\n", sqlite3_errmsg(database));
		sqlite3_close(database);
	}

	char nr[10];

	memcpy(sql_reply,"SELECT message FROM messages WHERE ROWID='",255);
	strcat(sql_reply,nr_message);
	strcat(sql_reply,"';");

	query(sql_reply, quer_res, ok, true);

	memcpy(sql_target,"SELECT user_from FROM messages WHERE ROWID='",255);
	strcat(sql_target,nr_message);
	strcat(sql_target,"';");

	query(sql_target, target_res, ok, true);

	memcpy(sql_reply,message,255);
	strcat(sql_reply,"< REPLY TO: ");
	strcat(sql_reply,quer_res);
	strcat(sql_reply," >");

	memcpy(sql,"INSERT INTO messages VALUES('",255);
	strcat(sql,utilizator.username);
	strcat(sql,"',trim('");
	strcat(sql,target_res);
	strcat(sql,"'),'");
	strcat(sql,sql_reply);
	strcat(sql,"','false',datetime('now'));");

	query(sql, res, ok,false);

	if(write(tdcl,&ok,sizeof(bool))<=0)
		perror("[server][reply]Eroare la write().\n");
}

void send(int tdcl)
{
	char sql[255];
	char *db_errors={0};
	char target_message[285];
	char target[30];
	char message[255];
	bool ok=false;
	char res[255];

	if(response!=SQLITE_OK)
	{
		fprintf(stderr,"Baza de date nu poate fi accesata %s\n", sqlite3_errmsg(database));
		sqlite3_close(database);
	}
	if(read(tdcl,&utilizator,sizeof(utilizator))<0)
		perror("[server][send]Eroare la read().\n");

	if(read(tdcl,&target,sizeof(target))<0)
		perror("[server[send]Eroare la read().\n");

	if(read(tdcl,&message,sizeof(message))<0)
		perror("[server][send]Eroare la read().\n");


	memcpy(sql,"INSERT INTO messages VALUES('",sizeof("INSERT INTO messages VALUES('"));
	strcat(sql,utilizator.username);
	strcat(sql,"','");
	strcat(sql,target);
	strcat(sql,"','");
	strcat(sql,message);
	strcat(sql,"','false',datetime('now'));");

	query(sql, res, ok,false);

	ok=false;
	char sql_send[255];
	memcpy(sql_send,"SELECT descriptor FROM users WHERE username='",255);
	strcat(sql_send,target);
	strcat(sql_send,"';");

	query(sql_send, res, ok, true);

	if(!ok)
		return;
	else
	{
		int send_cl=atoi(res);
		//char msg[255];
		//memcpy(msg, utilizator.username,255);
		//strcat(msg, ": ");
		//strcat(msg, message);
		//memcpy(message,msg,255);

		if(write(send_cl, &message, sizeof(message))<=0)
			perror("[server][send]Eroare la write().\n");
	}
}

void raspunde(void * arg,int tdcl)
{
	char command[255];
    int nr=0, i=0;
	td= ((struct thData*)arg);
		
	while (read (tdcl, &command,sizeof(command)) >= 0)
	{
		cout<<command<<"\n";
		if(strcmp(command,"signup")==0)
			registration(tdcl);
		else if(strcmp(command,"login")==0)
			login(tdcl);
		else if(strcmp(command,"online")==0)
			whosOnline(tdcl);
		else if(strcmp(command,"all")==0)
			showUsers(tdcl);
		else if(strcmp(command,"logout")==0)
		{
			logout(tdcl);
			close ((intptr_t)arg);
		}
		else if(strcmp(command,"reply")==0)
			reply(tdcl);
		else if(strcmp(command,"send")==0)
			send(tdcl);
		else if(strcmp(command,"history_user")==0)
			history_user(tdcl);
		else if(strcmp(command,"history_with_user")==0)
			history_with_user(tdcl);
	}
	
}

void *treat(void * arg)
{   
    td= (struct thData*)arg;  
    printf ("[thread]- %d - Asteptam mesajul...\n", td->idThread);
    fflush (stdout);     
    pthread_detach(pthread_self());   
    raspunde((struct thData*)arg, td->cl);

    close ((intptr_t)arg);
    return(NULL); 
      
}


int main()
{
 	struct sockaddr_in server;	// structura folosita de server
  	struct sockaddr_in from;	
  	int nr;		//mesajul primit de trimis la client 
  	int sd;		//descriptorul de socket 
  	int pid;
  	pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;
  

  	/* crearea unui socket */
  	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
    	perror ("[server]Eroare la socket().\n");
      	return errno;
    }
  	/* utilizarea optiunii SO_REUSEADDR */
  	int on=1;
  	setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  	/* pregatirea structurilor de date */
  	bzero (&server, sizeof (server));
  	bzero (&from, sizeof (from));
  
  	/* umplem structura folosita de server */
    server.sin_family = AF_INET;   	/* stabilirea familiei de socket-uri */	
    server.sin_addr.s_addr = htonl (INADDR_ANY);   	/* acceptam orice adresa */
    server.sin_port = htons (PORT);   	/* utilizam un port utilizator */
 
  	/* atasam socketul */
  	if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
   	  	perror ("[server]Eroare la bind().\n");
      	return errno;
    }

  	/* punem serverul sa asculte daca vin clienti sa se conecteze */
  	if (listen (sd, 2) == -1)
    {
      	perror ("[server]Eroare la listen().\n");
      	return errno;
    }
  	/* servim in mod concurent clientii...folosind thread-uri */
  	while (1)
    {
      	int client;
      	thData * td; //parametru functia executata de thread     
      	socklen_t length = sizeof (from);

      	printf ("[server]Asteptam la portul %d...\n",PORT);
      	fflush (stdout);
      	/* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      	if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
		{
	  		perror ("[server]Eroare la accept().\n");
	  		continue;
		}
		int idThread; //id-ul threadului
		int cl; //descriptorul intors de accept

		td=(struct thData*)malloc(sizeof(struct thData));	
		td->idThread=i++;
		td->cl=client;

		FD_ZERO(&active_rfds);
		FD_ZERO(&active_wfds);
		maxfds=max(maxfds,client);

		int rv;
		bcopy (  &fds, &active_wfds,sizeof(fd_set));
		bcopy (  &fds, &active_rfds, sizeof(fd_set));

		FD_SET(client, &active_rfds);
		rv=select(maxfds+1,&active_rfds,&active_wfds,NULL,NULL);

		FD_SET(client, &active_wfds);
		if(!FD_ISSET(td->cl, &active_rfds))
			cout<<"nu e activ boss";
    	
		pthread_create(&th[i], NULL, &treat, td);	
		FD_SET(client, &fds);					
	}   
};				