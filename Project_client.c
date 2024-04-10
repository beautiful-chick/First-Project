/*********************************************************************************
 *      Copyright:  (C) 2024 linuxer<linuxer@email.com>
 *                  All rights reserved.
 *
 *       Filename:  Project_client.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(20/03/24)
 *         Author:  linuxer <linuxer@email.com>
 *      ChangeLog:  1, Release initial version on "20/03/24 12:37:34"
 *                 
 ********************************************************************************/
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include<time.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<getopt.h>
#include<libgen.h>
#include<sqlite3.h>
#include<netinet/tcp.h>
int			TIMEOUT;
float		get_temperature(float *temp);
int			get_name(char buf2[1024],size_t buf2_size);
char		*get_time();
int 		internet_connect();
int			del_database();
int			internet_read();
int			get_row();
static inline void print_usage(char *progname);
//char					buf2[1024];
char					*now_time;
int						conn_fd = -1;
char					sock_buf[1024];
int						rv = -1;
int						cli_port;
char   					*cli_ip;
struct sockaddr_in		serv_addr;
static char             snd_buf[1024] = {0};
static sqlite3			*db = NULL;
struct DS18B20_DATA
{
	char            d_time[64];
	float           d_temp;
	char            d_name[64];
}data;


int dev_sqlite3(struct DS18B20_DATA data);
char  		*read_data();
int			internet_write(struct DS18B20_DATA data, char *snd_buf);


int main(int argc, char **argv)
{
	int					opt;
	char				*progname = NULL;
	float 				dev_temp;
	char				dev_name[64];
	char				*dev_time = NULL;
	int					rv = -1;
	int					rc;
	float				temp;
	char				name_buf[64];
	progname = (char *)basename( argv[0] );
	struct option		long_options[] = 
	{
		{"port", required_argument, NULL, 'p'},
		{"IP Address", required_argument, NULL, 'i'},
		{"time", required_argument, NULL, 't'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}

	};
	while( (opt = getopt_long(argc,argv,"p:i:t:h",long_options,NULL)) != -1 )
	{
		switch(opt)
		{
			case 'p':
				cli_port = atoi(optarg);
				break;
			case 'i':
				cli_ip = optarg;
				break;
			case 't':
				TIMEOUT = atoi(optarg);
				break;
			case 'h':
				print_usage(progname);
				return 0;
			default:
				break;
		}
	}
	if( !TIMEOUT || !cli_ip || !cli_port)
	{
		print_usage(progname);
		return 0;
	}

	rc = sqlite3_open("dev_database", &db);
	printf("db pointer: %p\n", (void *)db);

	if( rc != SQLITE_OK)
	{
		fprintf(stderr,"Can't open database : %s\n",sqlite3_errmsg(db));
		sqlite3_close(db);
	}
	printf("Open the dev_database ok\n");
	while(1)
	{
		sleep(TIMEOUT);
		rv = internet_connect(data);
		if(rv < 0 )
		{	
			if( get_temperature(&dev_temp) == 0 )
			{
				printf("Temperature:%.2f\n",dev_temp);
			}
			else
			{
				printf("Error:%s\n",strerror(errno));

			}
			if( (get_name(dev_name,sizeof(dev_name))) != 0 )
			{
				printf("get name error:%s\n",strerror(errno));
			}
			else
			{
				printf("dev_name:%s\n",dev_name);
			}


			printf("1\n");
			dev_time = get_time();
			if( dev_time != NULL)
			{
				printf("Current time is : %s\n",dev_time);
			}
			else
			{
				printf("Get current time failure : %s\n",strerror(errno));
			}
			printf("2\n");
			snprintf(data.d_name, 32, dev_name);
			printf("3\n");
			snprintf(data.d_time, 32, dev_time);
			printf("4\n");
			data.d_temp = dev_temp;
			printf("5\n");
			dev_sqlite3(data);
		}
		else
		{	
			del_database();
			//dev_temp = get_temperature();
			//dev_name = get_name();
			dev_time = get_time();
			snprintf(data.d_name, 32, dev_name);
			snprintf(data.d_time, 32, dev_time);
			data.d_temp = dev_temp;
			internet_write(data, snd_buf);
			internet_read(data);
		}
	}
//	sqlite3_close(db);
	return 0;


}

float get_temperature(float *temp)
{


	int					fd;
	char				buf[1024];
	char				*ptr;
	//float				temp;
	DIR					*dirp = NULL;
	char				w1_path[64] = "/sys/bus/w1/devices/";
	char				ds18b20_path[164];
	struct dirent		*direntp = NULL;
	char				chip_sn[32];
	int					found = 0;


	dirp = opendir(w1_path);
	if( !dirp )
	{
		printf("Open the floder failure : %s\n",strerror(errno));
		return -1;
	}

	while( NULL != (direntp = readdir(dirp)) )
	{

		if( strstr(direntp -> d_name,"28-") )
		{
			strncpy(chip_sn, direntp -> d_name, sizeof(chip_sn));
			found = 1;
		}
	}
	closedir(dirp);

	if( !found )
	{
		printf("can not find ds18b20 chipset\n");
		return -2;
	}



	/*获取全路径到ds18b20_path中去*/
	strncat(w1_path, chip_sn, sizeof(w1_path)-strlen(w1_path));
	strncat(w1_path, "/w1_slave", sizeof(w1_path)-strlen(w1_path));
	fd = open(w1_path, O_RDONLY);
	if( fd < 0 )
	{
		printf("Open the file failure : %s\n",strerror(errno));
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	if(read(fd, buf, sizeof(buf)) < 0 )
	{
		printf("Read data from fd = %d failure : %s\n",fd,strerror(errno));
		return -2;
	}
	ptr = strstr(buf, "t=");

	/*判断buf是否为空*/

	if( NULL == ptr )
	{
		printf("Can not find t = String\n");
		return -1;
	}
	ptr += 2;
	*temp = atof(ptr)/1000;
	return 0;
}


int	get_name(char buf2[1024],size_t buf2_size)
{
	int				fd2;
	char			w2_path[64] = "/sys/bus/w1/devices/";
	char			chip_sn1[256];
	DIR				*dirp1 = NULL;
	struct dirent	*direntp1 = NULL;
	int				found1 = 0;
	dirp1 = opendir(w2_path);
	if( !dirp1 )
	{
		printf("Open the floder failure : %s\n",strerror(errno));
		return -1;
	}

	while( NULL != (direntp1 = readdir(dirp1)))
	{
		if(strstr(direntp1 -> d_name,"28-"))
		{
			strncpy(chip_sn1, direntp1->d_name, sizeof(chip_sn1));
			found1 = 1;
		}
	}

	closedir(dirp1);
	if(!found1)
	{
		printf("Can not find name chipset\n");
		return -2;
	}

strncat(w2_path, chip_sn1,sizeof(w2_path)-strlen(w2_path));
strncat(w2_path, "/name",sizeof(w2_path)-strlen(w2_path));
fd2 = open(w2_path, O_RDONLY);
if( fd2 < 0 )
{
	printf("Open the file about name failure : %s\n",strerror(errno));
	return -3;
}
memset(buf2, 0, buf2_size);
if( read(fd2, buf2, buf2_size) < 0 )
{
	printf("get devices number failure : %s\n",strerror(errno));
	return -4;
}
printf("buf2:%s\n",buf2);

return 0;
}

char *get_time()
{
	time_t			timer;
	struct tm		*Now = NULL;
	char			*now_time = NULL;
	setenv("TZ", "Asia/Shanghai", 1);
	tzset();
	printf("6\n");
	time( &timer );
	printf("7\n");
	Now = localtime( &timer );
	if( Now == NULL)
	{
		printf("localtime() Error: %s\n",strerror(errno));
		return NULL;
	}
	printf("11\n");
	printf("8\n");
	now_time = asctime(Now);
	if(now_time == NULL)
	{
		printf("asctime() Error : %s\n",strerror(errno));
		return NULL;
	}
	printf("9\n");
	return now_time;
}

int internet_connect(struct DS18B20_DATA data)
{
	conn_fd = socket(AF_INET,SOCK_STREAM,0);	
	if( conn_fd < 0 )
	{
		printf("Create socket failure : %s\n",strerror(errno));
		close(conn_fd);
		return -1;
	}
	printf("create socket ok\n");
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(cli_port);
	inet_aton(cli_ip,&serv_addr.sin_addr);

	if( connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Connect to server[%s:%d] failure : %s\n",cli_ip, cli_port, strerror(errno));
		return -1;
	}

	return 0;
}


int internet_write(struct DS18B20_DATA data, char *snd_buf)
{
	snprintf(snd_buf, 2048, "%s,%s,%.2f", data.d_time, data.d_name, data.d_temp);
	printf("internet_write snd_buf:%s\n", snd_buf);
	if( write(conn_fd, snd_buf, strlen(snd_buf)) < 0 )
	{
		printf("Wirte data to server[%s:%d] failure : %s\n", cli_ip, cli_port, strerror(errno));	
	}
	return 0;
}

int internet_read()
{
	char 			sock_buf[1024];

	memset(sock_buf, 0, sizeof(sock_buf));
	rv = read(conn_fd, sock_buf, sizeof(sock_buf));
	if( rv < 0 )
	{
		printf("Read data from server failure : %s\n", strerror(errno));
		close(conn_fd);
	}
	else if( 0 == rv)
	{
		printf("Client connect to server failure : %s\n", strerror(errno));
	}
	printf("Read %d bytes data from server : '%s'\n", rv, sock_buf);
	return 0;
}


int del_database(char *snd_buf)
{

	struct tcp_info				optval;
	socklen_t 					optlen = sizeof(optval);
	//static sqlite3				*db;
	static int					rc;
	int							ret;
	int							m;
	char                		*err_msg;
	int							row;

	ret = getsockopt(conn_fd, IPPROTO_TCP, TCP_INFO, &optval, &optlen);

	//sqlite3_open("dev_database.db", &db);		
	row = get_row();
	while(row > 1)
	{
		row = get_row();
		printf("Now row:%d\n", row);
		snd_buf = read_data();
		write(conn_fd, snd_buf, strlen(snd_buf));
		/*删除数据*/
		printf("Start to delete data\n");
		//	sqlite3_open("dev_database.db", &db);
		char *sql = "DELETE FROM dev_mesg WHERE ROWID IN (SELECT ROWID FROM dev_mesg LIMIT 1);";
		printf("3\n");
		rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
		if(rc != SQLITE_OK)
		{
			fprintf(stderr, "delete data error : %s\n", sqlite3_errmsg(db));
			sqlite3_free(err_msg);
		}
	}

	//sqlite3_close(db);

	return 0;
}


char  *read_data()

{
	char				*snd_buf;
	int					rc;
	static int			row_count; 
	//sqlite3				*db;
	char				*errmsg;
	sqlite3_stmt		*stmt;

	//rc = sqlite3_open("dev_database.db",&db);
	/*  	if( rc != SQLITE_OK)
			{
			fprintf(stderr, "Can't open database : %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			return NULL;
			}
			printf("Open the database ok\n");*/

	const char *sql = "SELECT * FROM dev_mesg LIMIT 1;";
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if( rc != SQLITE_OK)
	{
		fprintf(stderr, "Failed to prepare statement : %s\n", sqlite3_errmsg(db));
		//	sqlite3_close(db);
		return NULL;
	}

	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		const char *de_name = (const char *)sqlite3_column_text(stmt,0);
		const char *de_time = (const char *)sqlite3_column_text(stmt,1);
		double de_temp = sqlite3_column_double(stmt,2);
		printf("de_name: %s, de_time: %s, de_temp: %.2f\n", de_name, de_time, de_temp);
		memset(snd_buf,0,strlen(snd_buf));
		snprintf(snd_buf,2048,"%s,%s,%.2f", de_time, de_name, de_temp);
		printf("%s", snd_buf);

	}
	sqlite3_finalize(stmt);
	return snd_buf;


}

int get_row()
{
	static int 			row_count;
	int					rc;
	sqlite3_stmt		*stmt;
	//sqlite3				*db;

	//rc = sqlite3_open("dev_database.db",&db);
	const char *sql = "SELECT COUNT(*) FROM dev_mesg;";
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "Failed to prepare statement:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
	rc = sqlite3_step(stmt);
	if( rc == SQLITE_ROW)
	{
		row_count = sqlite3_column_int(stmt,0);
	}
	else
	{
		fprintf(stderr,"Error executing query: %s\n",sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
	//sqlite3_close(db);
	return row_count;	
}


int dev_sqlite3(struct DS18B20_DATA data)
{
	int 		rc;
	//sqlite3		*db;
	char 		*err_msg = 0;
	char		*sql = 0;
	printf("db pointer(in the dev_sqlite3): %p\n", (void *)db);

	rc = sqlite3_open("dev_database.db", &db);
	/* 	if( rc != SQLITE_OK)
	{
		fprintf(stderr,"Can not open database : %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}*/
	const char *sql_stmt = 
		"CREATE TABLE IF NOT EXISTS dev_mesg ("
		"de_name TEXT NOT NULL,"
		"de_time TEXT NOT NULL,"
		"de_temp REAL NOT NULL);";
	rc = sqlite3_exec(db, sql_stmt, 0, 0, &err_msg);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "create table error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return -1;
	}
	printf("Table create successfully\n");
	sql = sqlite3_mprintf("INSERT INTO dev_mesg (de_name,de_time,de_temp) VALUES ('%s','%s',%.2f);",data.d_name,data.d_time,data.d_temp);
	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	if (rc != SQLITE_OK) 
	{
		fprintf(stderr, "give values error: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return -1;
	}
	printf("Record inserted successfully\n");
	//sqlite3_close(db);

	return 0;
}

static inline void print_usage(char *progname)
{
	printf("Usage: %s [OPTION]...\n", progname);
	printf("%s is a socket server program,which used to verify client and echo back string from it\n", progname);
	printf("\nMandatory arguments to long options are mandatory for short options too:\n");
	printf("-i[IP Address]\n");
	printf("-p[Port]\n");
	printf("-t[time]\n");
	return ;
}
