#ifndef __SHTTPD_H__
#define __SHTTPD_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>   /* for sockaddr_in */
#include <netdb.h>        /* for hostent */ 
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>        /* we want to catch some of these after all */
#include <unistd.h>       /* protos for read, write, close, etc */
#include <dirent.h>       /* for MAXNAMLEN */
#include <limits.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <stddef.h>


/*线程的状态值*/
enum{WORKER_INITED, WORKER_RUNNING,WORKER_DETACHING, WORKER_DETACHED,WORKER_IDEL};

// 配置文件的结构
struct conf_opts{
	char CGIRoot[128];		/*CGI跟路径*/
	char DefaultFile[128];		/*默认文件名称*/
	char DocumentRoot[128];	/*根文件路径*/
	char ConfigFile[128];		/*配置文件路径和名称*/
	int ListenPort;			/*侦听端口*/
	int MaxClient;			/*最大客户端数量*/
	int TimeOut;				/*超时时间*/
	int InitClient;				/*初始化线程数量*/
};



/* HTTP协议的方法 */
typedef enum SHTTPD_METHOD_TYPE{
	METHOD_GET, 		/*GET     方法*/
	METHOD_POST, 		/*POST   方法*/
	METHOD_PUT, 		/*PUT     方法*/
	METHOD_DELETE, 	/*DELETE方法*/
	METHOD_HEAD,		/*HEAD   方法*/
	METHOD_CGI,		/**CGI方法*/
	METHOD_NOTSUPPORT
}SHTTPD_METHOD_TYPE;

enum {HDR_DATE, HDR_INT, HDR_STRING};	/* HTTP header types		*/

typedef struct shttpd_method{
	SHTTPD_METHOD_TYPE type;
	int name_index;
	
}shttpd_method;

/*
 * For parsing. This guy represents a substring.
 */
// 
typedef struct vec 
{
	char	*ptr;                  // 字符串
	int			len;
	SHTTPD_METHOD_TYPE type;       // 字符串表示的类型
}vec;

/*
 * This thing is aimed to hold values of any type.
 * Used to store parsed headers' values.
 */
#define big_int_t long

struct http_header {
	int		len;		/* Header name length		*/
	int		type;		/* Header type			*/
	size_t		offset;		/* Value placeholder		*/
	char	*name;		/* Header name			*/
};

/*
 * This structure tells how HTTP headers must be parsed.
 * Used by parse_headers() function.
 */
#define	OFFSET(x)	offsetof(struct headers, x)

/*
 * This thing is aimed to hold values of any type.
 * Used to store parsed headers' values.
 */
union variant {
	char		*v_str;
	int		v_int;
	big_int_t	v_big_int;
	time_t		v_time;
	void		(*v_func)(void);
	void		*v_void;
	struct vec	v_vec;
};


#define	URI_MAX		16384		/* Default max request size	*/
/*
 * This guy holds parsed HTTP headers
 */
struct headers {
	union variant	cl;		/* Content-Length:	内容长度	*/
	union variant	ct;		/* Content-Type:	内容类型	*/
	union variant	connection;	/* Connection:  连接状态		*/
	union variant	ims;		/* If-Modified-Since:	最后的修改时间	*/
	union variant	user;		/* Remote user name		用户名称 */
	union variant	auth;		/* Authorization	权限	*/
	union variant	useragent;	/* User-Agent:		用户代理	*/
	union variant	referer;	/* Referer:	参考		*/
	union variant	cookie;		/* Cookie:		cookie	*/
	union variant	location;	/* Location:	位置		*/
	union variant	range;		/* Range:		范围	*/
	union variant	status;		/* Status:		状态值	*/
	union variant	transenc;	/* Transfer-Encoding:	编码类型	*/
};

struct cgi{
	int iscgi;
	struct vec bin;
	struct vec para;	
};

struct worker_ctl;

// 来管理线程的相关状态等
struct worker_opts{
	pthread_t th;			/*线程的ID号*/
	int flags;				/*线程状态*/
	pthread_mutex_t mutex;/*线程任务互斥*/

	struct worker_ctl *work;	/*本线程的总控结构*/
};

struct worker_conn ;
/*请求结构*/
struct conn_request{
	struct vec	req;		/*请求向量*/
	char *head;			/*请求头部\0'结尾*/
	char *uri;			/*请求URI,'\0'结尾*/
	char rpath[URI_MAX];	/*请求文件的真实地址\0'结尾*/

	int 	method;			/*请求类型*/

	/*HTTP的版本信息*/
	unsigned long major;	/*主版本*/
	unsigned long minor;	/*副版本*/

	struct headers ch;	/*头部结构*/

	struct worker_conn *conn;	/*连接结构指针*/
	int err;                      // 错误代码
};

/* 响应结构 */
struct conn_response{
	struct vec	res;		/*响应向量*/
	time_t	birth_time;	/*建立时间*/
	time_t	expire_time;/*超时时间*/

	int		status;		/*响应状态值*/
	int		cl;			/*响应内容长度*/

	int 		fd;			/*请求文件描述符*/
	struct stat fsate;		/*请求文件状态*/

	struct worker_conn *conn;	/*连接结构指针*/	
};

//  其中的 worker_conn 用于维护客户端请求和响应数据
struct worker_conn 
{
#define K 1024
	char		dreq[16*K];	/*请求缓冲区：接收到的客户端请求数据*/
	char		dres[16*K];	/*响应缓冲区：发送给客户端的数据 */

	int		cs;			/*客户端套接字文件描述符*/
	int		to;			/*客户端无响应时间超时退出时间，超时响应时间*/

	struct conn_response con_res;         //  维护响应结构
	struct conn_request con_req;          // 维护客服端的请求

	struct worker_ctl *work;	/*本线程的总控结构*/
};


//  其中的 worker_conn 用于维护客户端请求和响应数据
struct worker_ctl{
	struct worker_opts opts;               // 表示线程状态
	struct worker_conn conn;               // 表示客户端请求的状态和值
};
struct mine_type{
	char	*extension;           // 扩展名
	int 			type;          // 类型
	int			ext_len;          // 扩展名长度
	char	*mime_type;           //内容类型
};


void Para_Init(int argc, char *argv[]);

int Request_Parse(struct worker_ctl *wctl);
int Request_Handle(struct worker_ctl* wctl);


int Worker_ScheduleRun();
int Worker_ScheduleStop();
void Method_Do(struct worker_ctl *wctl);
void uri_parse(char *src, int len);

struct mine_type* Mine_Type(char *uri, int len, struct worker_ctl *wctl);



#define DBGPRINT printf


#endif /*__SHTTPD_H__*/

