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


static sqlite3			*db = NULL;

struct DS18B20_DATA
{
	char            d_time[64];
	float           d_temp;
	char            d_name[64];
}data;


typedef struct socket_s
{
	int			conn_fd;
	char		host[64];
	int			cli_port;
} socket_t;
socket_t			so;
socket_t			*my_socket = &so;


float		get_temperature(float *temp);
int			get_name(char buf2[1024],size_t buf2_size);
char		*get_time(char *now_time);
int 		internet_connect();
int			del_database();
int			internet_read();
int			get_row();
static inline void print_usage(char *progname);
int 		insert_data(struct DS18B20_DATA data);
int 		dev_sqlite3();
char  		*read_data();
int			internet_write(struct DS18B20_DATA data, char *snd_buf);
int			extract_data(char *snd_buf);
int assign_data(char dev_name[64], char *dev_time, float dev_temp,struct DS18B20_DATA *data);

int main(int argc, char **argv)
{

	int					timeout;
	int					opt;
	char				*progname = NULL;
	float 				dev_temp;
	char				dev_name[64];
	char				*dev_time;
	int					rv = -1;
	int					rc;
	float				temp;
	char				name_buf[64];
	int					flag;
	time_t				now_time,last_time;
	struct tcp_info		optval;
	socklen_t 			optlen = sizeof(optval);
	int					ret;
	int					rv1;
	int					row;
	double				time_diff;
 	char     	        snd_buf[1024] = {0};
	//socket_t			my_socket;
	
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
				my_socket->cli_port = atoi(optarg);
				break;
			case 'i':
				strcpy(my_socket->host, optarg);
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			case 'h':
				print_usage(progname);
				return 0;
			default:
				break;
		}
	}


	if( !timeout || !my_socket->host[64] || !my_socket->cli_port)
	{
		print_usage(progname);
		return 0;
	}


	dev_sqlite3();//打开数据库，创建表（现在db是打开的）
	last_time = time(NULL);


	printf("-------------Start While---------------------\n");
	while(1)
	{
		flag = 0;
		/*判断是否到采样时间*/
		now_time = time(NULL);
		time_diff = difftime(now_time,last_time);


		if( time_diff >= timeout )
		{
			/*采样时间到了 开始采样*/
			printf("------------------Start get data--------------------\n");
			if( get_temperature(&dev_temp) == 0 )
			{
				printf("Temperature:%.2f\n",dev_temp);
			}
			else
			{
				printf("Temperature error:%s\n",strerror(errno));
			}
			if( (get_name(dev_name,sizeof(dev_name))) != 0 )
			{
				printf("Get name error:%s\n",strerror(errno));
			}
			else
			{
				printf("dev_name:%s\n",dev_name);
			}
			dev_time = get_time(NULL);
			if( dev_time  != NULL )
			{
				printf("Current time is:%s\n",dev_time);
			}
			else
			{
				printf("Get current time failure:%s\n",strerror(errno));
			}


			int m = assign_data(dev_name,dev_time,dev_temp,&data);
			if( m != 0)
			{
				printf("assign_data error\n");
			}
			else
			{
				printf("assign_data ok\n");
			}
/*  		snprintf(data.d_name,32,dev_name);
			printf("data.d_name:%s\n",data.d_name);
			snprintf(data.d_time,32,dev_time);
			printf("data.d_time:%s\n",data.d_time);
			data.d_temp = dev_temp;
			printf("data.d_temp:%.2f\n",data.d_temp);*/
			printf("after caiyang:data.d_name:%s------data.d_time:%s-----------data.d_temp:%.2f\n",data.d_name,data.d_time,data.d_temp);
			last_time = now_time;
			flag = 1;

			ret = getsockopt(my_socket->conn_fd, IPPROTO_TCP, TCP_INFO, &optval, &optlen);
	}
		sleep(2);
		/*判断是否有网络连接,ret不等于0代表没有网络连接*/
		//internet_connect();
		ret = getsockopt(my_socket->conn_fd, IPPROTO_TCP, TCP_INFO, &optval, &optlen);
		printf("ret:%d\n",ret);

		printf("if(ret != 0)\n");
		if( ret != 0 )
		{
			/*重连socket*/
			rv = internet_connect();
			printf("rv:%d\n",rv);
			printf("99999999999999999\n");
			printf("m11111111111111111111111111111\n");
		
		/*重连失败，采集的数据写到数据库中*/
			if(rv < 0 )
			{	

				close(my_socket->conn_fd);
				if( flag == 1 )
				{
					printf("The internet disconnedted\n");
					printf("before insert_data:data.d_name:%s------data.d_time:%s-----------data.d_temp:%.2f\n",
								data.d_name,data.d_time,data.d_temp);
					insert_data(data);	
					continue;
				}
			}
		}
		
			
			/*socket连上的情况*/	
			if( flag == 1 )
			{
				/*发送当前采样的数据*/
				rv1 = internet_write(data, snd_buf);
				printf("Send the data ok\n");
				internet_read(data);


				/*发送失败，将数据再存到数据库中*/
				if( rv1 < 0 )
				{
					dev_sqlite3(data);
					continue;
				}
			}
				
				/*判断数据库中是否有数据*/
				row = get_row();
				if(row != 0)
				{
					/*提取,发送一条数据*/
				    extract_data(snd_buf);
					/*删除一条数据*/
					del_database();
					
				}
	}
sqlite3_close(db);
return 0;


}


float get_temperature(float *temp)
{


	int					fd;
	char				buf[1024];
	char				*ptr;
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

return 0;
}



char *get_time(char *now_time)
{
	time_t			timer;
	struct tm		*Now = NULL;


	setenv("TZ", "Asia/Shanghai", 1);
	tzset();
	time( &timer );
	Now = localtime( &timer );


	if( Now == NULL)
	{
		printf("localtime() Error: %s\n",strerror(errno));
		return NULL;
	}


	now_time = asctime(Now);


	if(now_time == NULL)
	{
		printf("asctime() Error : %s\n",strerror(errno));
		return NULL;
	}
	printf("In the get time now_time:%s\n",now_time);
	return now_time;
}



int internet_connect(struct DS18B20_DATA data)
{
	struct sockaddr_in		serv_addr;
	//char					(*ptr)[64] = &my_socket.host;


	my_socket->conn_fd = socket(AF_INET,SOCK_STREAM,0);	

	if( my_socket->conn_fd < 0 )
	{
		printf("Create socket failure : %s\n",strerror(errno));
		close(my_socket->conn_fd);
		return -1;
	}
	printf("create socket ok\n");


	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	printf("1111111\n");
	serv_addr.sin_port = htons(my_socket->cli_port);
	printf("2222222\n");
	if( inet_aton(my_socket->host, &serv_addr.sin_addr) == 0 )
	{
		printf("Invalid IP address: %s\n", my_socket->host);
		close(my_socket->conn_fd);
		return -1;
	}
	printf("3333333333333333\n");


	if( connect(my_socket->conn_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Connect to server[%s:%d] failure : %s\n",my_socket->host, my_socket->cli_port, strerror(errno));
		return -1;
	}
	printf("44444444444444444444\n");
	return 0;
}


int internet_write(struct DS18B20_DATA data, char *snd_buf)
{
	snprintf(snd_buf, 2048, "%s,%s,%.2f", data.d_time, data.d_name, data.d_temp);
	printf("internet_write snd_buf:%s\n", snd_buf);


	if( write(my_socket->conn_fd, snd_buf, strlen(snd_buf)) < 0 )
	{
		printf("Wirte data to server[%s:%d] failure : %s\n", my_socket->host[64], my_socket->cli_port, strerror(errno));	
	}
	return 0;
}

int internet_read()
{
	char 			sock_buf[1024];
	int				rv = -1;


	memset(sock_buf, 0, sizeof(sock_buf));
	rv = read(my_socket->conn_fd, sock_buf, sizeof(sock_buf));


	if( rv < 0 )
	{
		printf("Read data from server failure : %s\n", strerror(errno));
		close(my_socket->conn_fd);
	}
	else if( 0 == rv)
	{
		printf("Client connect to server failure : %s\n", strerror(errno));
	}


	printf("Read %d bytes data from server : '%s'\n", rv, sock_buf);
	return 0;
}


int extract_data(char *snd_buf)
{
	snd_buf = read_data();

	printf("In the extract_data() snd_buf:%s\n",snd_buf);
	write(my_socket->conn_fd, snd_buf, strlen(snd_buf));
	return 0;
}

int del_database(char *snd_buf)
{

	struct tcp_info				optval;
	socklen_t 					optlen = sizeof(optval);
	static int					rc;
	int							ret;
	int							m;
	char                		*err_msg;
	int							row;		
	char						*sql;

	row = get_row();
	printf("Now row:%d\n", row);

	/*删除数据*/
	printf("Start to delete data\n");

	sql = "DELETE FROM dev_mesg WHERE ROWID IN (SELECT ROWID FROM dev_mesg LIMIT 1);";
	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr, "delete data error : %s\n", sqlite3_errmsg(db));
		sqlite3_free(err_msg);
	}

	return 0;
}


char  *read_data()

{
	char				read_buf[1024];
	char				*snd_buf  = read_buf;
	int					rc;
	static int			row_count; 
	char				*errmsg;
	sqlite3_stmt		*stmt;
	const char			*sql;

	sql = "SELECT * FROM dev_mesg LIMIT 1;";

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if( rc != SQLITE_OK)
	{
		fprintf(stderr, "Failed to prepare statement : %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return NULL;
	}

	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW)
	{
		const char *de_name = (const char *)sqlite3_column_text(stmt,0);
		const char *de_time = (const char *)sqlite3_column_text(stmt,1);
		double de_temp = sqlite3_column_double(stmt,2);
		printf("de_name: %s, de_time: %s, de_temp: %.2f\n", de_name, de_time, de_temp);
		memset(snd_buf,0,sizeof(read_buf));
		snprintf(read_buf,2048,"%s,%s,%.2f", de_time, de_name, de_temp);
		printf("%s", read_buf);

	}
	sqlite3_finalize(stmt);
	return snd_buf;


}

int get_row()
{
	static int 			row_count;
	int					rc;
	sqlite3_stmt		*stmt;
	const char			*sql;

	sql = "SELECT COUNT(*) FROM dev_mesg;";


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
	return row_count;	
}


int dev_sqlite3()
{
	int 		rc;
	char 		*err_msg = 0;
	char		*sql = 0;


	rc = sqlite3_open("dev_database.db", &db);
	printf("db pointer(in the dev_sqlite3): %p\n", (void *)db);
	if( rc != SQLITE_OK)
	{
		fprintf(stderr,"Can not open database : %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}


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
	return 0;
}



int insert_data(struct DS18B20_DATA data)
{
	char			*sql;
	char			*err_msg = 0;
	int				rc;
	printf("data.d_name:%s------data.d_time:%s-----------data.d_temp:%.2f\n",
												data.d_name,data.d_time,data.d_temp);


	sql = sqlite3_mprintf("INSERT INTO dev_mesg (de_name,de_time,de_temp) VALUES ('%s','%s',%.2f);"
												,data.d_name,data.d_time,data.d_temp);


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

int assign_data(char dev_name[64], char *dev_time, float dev_temp,struct DS18B20_DATA *data)
{
	printf("---------------------------------------------------\n");
	printf("dev_name: %s\n", dev_name);
	printf("dev_time: %s\n", dev_time);
	printf("dev_temp: %.2f\n", dev_temp);

	snprintf(data->d_name,32,dev_name);
	printf("data->d_name:%s\n",data->d_name);
	snprintf(data->d_time,32,dev_time);
	printf("data->d_time:%s\n",data->d_time);
	data->d_temp = dev_temp;
	printf("data->d_temp:%.2f\n",data->d_temp);
	return 0;

}
