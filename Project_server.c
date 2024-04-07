/*********************************************************************************
 *      Copyright:  (C) 2024 LiYi<1751425323@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  Project_server.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(02/04/24)
 *         Author:  LiYi <1751425323@qq.com>
 *      ChangeLog:  1, Release initial version on "02/04/24 15:01:37"
 *                 
 ********************************************************************************/
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<getopt.h>
#include<ctype.h>
#include<sqlite3.h>
/*  int socket_server();
	int socket_bind();
	struct sockaddr_in	servaddr;
	int					rv = -1;
	int					sockfd;
	char					*ip;
	int						port = 0;*/
char		buf[1024];
int save_data(char *buf);
int dev_sqlite3();
void print_usage(char *progname)
{
	printf("%s usage : \n",progname);
	printf("-p(--port):sepcify server listen port.\n");
	printf("-i(--ip address):specify server listen ip address.\n");
	printf("-h(--help):print this help information.\n");
	return ;
}
struct DS18B20_DATA
{
	char            d_time[64];
	char            d_temp[64];
	char            d_name[64];
}data;

int main(int argc,char **argv)
{
	int						sockfd = -1;
	int						rv = -1;
	struct sockaddr_in		servaddr;
	struct sockaddr_in		cliaddr;
	socklen_t				len;
	int						port = 0;
	int						clifd;
	int						ch;
	pid_t					pid;
	int						on = 1;
	char					*ip;
	//	int						port;
	struct option opts[] = {
		{"ip address",required_argument,NULL,'i'},
		{"port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};


	while( (ch = getopt_long(argc,argv,"i:p:h",opts,NULL)) != -1 )
	{
		switch(ch)
		{
			case 'i':
				ip = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'h':
				print_usage(argv[0]);
				return 0;

		}
	}

	printf("%d\n",port);
	printf("%s\n",ip);
	if( !ip || !port )
	{
		print_usage(argv[0]);
		return 0;
	}
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if( sockfd < 0 )
	{
		printf("Create socket failure : %s\n",strerror(errno));
		return -1;
	}
	printf("Create socket[%d] successfully\n",sockfd);
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	inet_aton(ip,&servaddr.sin_addr);
	rv = bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	if( rv < 0 )
	{
		printf("Socket[%d] bind on port [%d] failure : %s\n",sockfd,port,strerror(errno));
		return -2;
	}
	listen(sockfd,13);
	printf("Start to listen on port[%d]\n",port);
	while(1)
	{
		printf("Start accept new client coming...\n");
		len = sizeof(struct sockaddr_in);

		clifd = accept(sockfd,(struct sockaddr *)&cliaddr,&len);
		printf("clifd:%d\n",clifd);
		if( clifd < 0 )
		{
			printf("Accept new client failure : %s\n",strerror(errno));
			continue;
		}
		printf("Accept new client[%s : %d] successfully!\n",inet_ntoa(cliaddr.sin_addr),
				ntohs(cliaddr.sin_port));
		pid = fork();
		if( pid < 0 )
		{
			printf("fork() create child process failure : %s\n",strerror(errno));
			close(clifd);
			continue;
		}
		else if( pid > 0 )
		{
			close(clifd);
			continue;
		}
		else if( 0 == pid )
		{

			int				i;
			printf("Child process start to commuicate with socket client...\n");
			close(sockfd);

			while(1)
			{
				memset(buf,0,sizeof(buf));
				rv = read(clifd,buf,sizeof(buf));
				if( rv < 0 )
				{
					printf("Read data from client sockfd[%d] failure : %s\n",clifd,strerror(errno));
					close(clifd);
					exit(0);
				}
				else if(rv == 0)
				{
					printf("Socket[%d] get disconnected\n",clifd);
					close(clifd);
					exit(0);
				}
				else if( rv > 0 )
				{
					printf("Read %d bytes data from Server : %s\n",rv,buf);
				}
				save_data(buf);
				for(i = 0;i<rv;i++)
				{
					buf[i] = toupper(buf[i]);
				}
				rv = write(clifd,buf,rv);
				if( rv < 0 )
				{
					printf("Write to client by sockfd[%d] failure : %s\n",clifd,strerror(errno));
					close(clifd);
					exit(0);
				}
			}
		}
	}
	close(sockfd);
	return 0;

}


/*int server_socket()
  {

  int						on = 1;
  int						sockfd = -1;
  struct sockaddr_in		servaddr;

  sockfd = socket(AF_INET,SOCK_STREAM,0);
  if(sockfd < 0 )
  {
  printf("Create socket failure : %s\n",strerror(errno));
  return -1;
  }
  printf("Create socket[%d] successfully!\n",sockfd);
  setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

  memset(&servaddr,0,sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);
  inet_aton(ip,&servaddr.sin_addr);
  return sockfd;
  }				

  int server_bind()
  {
  rv = bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
  if( rv < 0 )
  {
  printf("Socket[%d] bind on port[%d] failure : %s\n",sockfd,port,strerror(errno));
  return -2;
  }
  listen(sockfd,13);
  printf("Start to listen on port [%d]\n",port);

  return 0;

  }
  */



int dev_sqlite3(struct DS18B20_DATA data)
{
	printf("Start sqlite\n");
	int						rc;
	sqlite3					*db;
	char					*err_msg = 0;
	char					*sql = NULL;
	rc = sqlite3_open("ser_database.db",&db);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr,"Can not open database:%s\n",sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
	const char *sql_stmt = 
		"CREATE TABLE IF NOT EXISTS dev_mesg ("
		"de_name TEXT NOT NULL,"
		"de_time TEXT NOT NULL,"
		"de_temp TEXT NOT NULL);";
	rc = sqlite3_exec(db,sql_stmt,0,0,&err_msg);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr,"Create table error : %s\n",err_msg);
		sqlite3_free(err_msg);
		return -1;
	}
	printf("Table create successfully!\n");
	printf("after data.d_temp:%s\n",data.d_temp);
	sql = sqlite3_mprintf("INSERT INTO dev_mesg (de_name,de_time,de_temp) VALUES ('%s','%s',%s);",data.d_name,data.d_time,data.d_temp);
	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	if (rc != SQLITE_OK) 
	{
		fprintf(stderr, "give values error: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return -1;
	}
	sqlite3_close(db);
}



int save_data(char *buf)

{

	printf("In the save_data buf : %s\n",buf);
	const char		delim[] = ",";
	char			*saveptr1,*saveptr2,*saveptr3,*endptr;
	char			*token;
	float			value;
	token = strtok_r(buf, delim, &saveptr1);
	snprintf(data.d_time,32,token);
	printf("data.d_time:%s\n",data.d_time);
	token = strtok_r(saveptr1,delim,&saveptr2);
	printf("saveptr2:%s\n",saveptr2);
	snprintf(data.d_name,32,token);
	printf("data.d_name:%s\n",data.d_name);
	token = strtok_r(saveptr2,delim,&saveptr3);
	printf("token: %s\n",token);
	value = strtod(token, &endptr);
	printf("value:%f\n",value);
	snprintf(data.d_temp, sizeof(data.d_temp), "%f", value);

	//strncpy(data.d_temp,saveptr2,strlen(saveptr2));
	//snprintf(data.d_temp,32,saveptr2);
	printf("1\n");
	//strncpy(data.d_temp,token,strlen(token));
	//snprintf(data.d_temp, sizeof(data.d_temp), "%s", token);
	//snprintf(data.d_temp,64,"%s",token);
	printf("data.d_temp:%s\n",data.d_temp);
	printf("2\n");
	dev_sqlite3(data);
	printf("3\n");
	return 0;

}

