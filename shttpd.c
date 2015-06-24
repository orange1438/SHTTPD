#include "shttpd.h"

// 命令行的解析结构
struct conf_opts conf_para={
	/*CGIRoot*/		"/usr/local/var/www/cgi-bin/",
	/*DefaultFile*/	"index.html",
	/*DocumentRoot*/"/usr/local/var/www/",
	/*ConfigFile*/	"/etc/sHTTPd.conf",
	/*ListenPort*/	8080,
	/*MaxClient*/	4,	
	/*TimeOut*/		3,
	/*InitClient*/		2
};

// 建立一个结构数组，将各自结构放入
struct vec _shttpd_methods[] = {
	{"GET",		3, METHOD_GET},
	{"POST",		4, METHOD_POST},
	{"PUT",		3, METHOD_PUT},
	{"DELETE",	6, METHOD_DELETE},
	{"HEAD",		4, METHOD_HEAD},
	{NULL,		0}                // 结尾
};

/*SIGINT信号截取函数*/
// 用户Ctrl+C后，调用调度终止函数
static void sig_int(int num)
{
	Worker_ScheduleStop();

	return;
}

/*SIGPIPE信号截取函数*/
static void sig_pipe(int num)
{
	return;
}
int do_listen()
{
	struct sockaddr_in server;
	int ss = -1;
	int err = -1;
	int reuse = 1;
	int ret = -1;

	/* 初始化服务器地址 */
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port = htons(conf_para.ListenPort);   
	
	/* 信号截取函数 */
	signal(SIGINT,  sig_int);
	signal(SIGPIPE, sig_pipe);

	/* 生成套接字文件描述符 */
	ss = socket (AF_INET, SOCK_STREAM, 0);
	if (ss == -1)
	{
		printf("socket() error\n");
		ret = -1;
		goto EXITshttpd_listen;
	}

	/* 设置套接字地址和端口复用 */
	err = setsockopt (ss, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if (err == -1)
	{
		printf("setsockopt SO_REUSEADDR failed\n");
	}

	/* 绑定IP和套接字描述符 */
	err = bind (ss, (struct sockaddr*)  &server, sizeof(server));
	if (err == -1)
	{
		printf("bind() error\n");
		ret = -2;
		goto EXITshttpd_listen;
	}

	/* 设置服务器侦听队列长度 */
	err = listen(ss, conf_para.MaxClient*2);
	if (err)
	{
		printf ("listen() error\n");
		ret = -3;
		goto EXITshttpd_listen;
	}

	ret = ss;
EXITshttpd_listen:
	return ret;
}


int main(int argc, char *argv[])
{
	signal(SIGINT, sig_int);       // 挂接信号

	Para_Init(argc,argv);         // 参数初始化

	int s = do_listen();       //   套接字初始化

	Worker_ScheduleRun(s);      // 任务调度

	return 0;
}
