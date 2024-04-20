#include <sys/types.h>
#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>
#include <string>
#include <ctime>

#include <logger.h>
#include <uv.h>

#define ZLOG_CONF 		"etc/zlog.conf"
#define DEFAULT_PORT 	4444
#define DEFAULT_BACKLOG 128
#define __CHECK__ 		1

/*********************************************************************************/
/**																			 	**/
/** async 																	 	**/ 
/** - 测试libuv的async task														**/ 
/**																			 	**/ 
/*********************************************************************************/

// 命令行参数
typedef struct _args {
	bool mt;
} args_t;

uv_async_t async;

static void idle_cb(uv_idle_t*); 
static void check_cb(uv_check_t*); 
static void close_cb(uv_handle_t*);
static void async_cb(uv_async_t*, int);
static void async_cb1(uv_async_t*, int);
static void async_cb2(uv_async_t*, int);
static void timer_cb(uv_timer_t*);
int parse_args(int, char**, args_t&);
void on_new_connection(uv_stream_t*, int);
void read_cb(uv_stream_t*, ssize_t, const uv_buf_t*);
void alloc_buffer(uv_handle_t*, size_t, uv_buf_t*);
int64_t now();

// 获取当前时间
int64_t now() {
	struct timeval tv; 
	struct timezone tz; 
	gettimeofday(&tv, &tz);
	return tv.tv_sec*1000000+tv.tv_usec;
}

// IDLE句柄
static int64_t counter = 0;
static int64_t time_a = 0;
static int64_t time_b = 0;

void idle_cb(uv_idle_t* handle) {
	sleep(3);
#ifdef A
	debug(">>>>>>>>>>>>>");
	uv_print_active_handles(uv_default_loop(), stdout);
	debug("-------------");
#endif
}

void check_cb(uv_check_t* handle) {
	uv_print_active_handles(uv_default_loop(), stdout);
	info("<<<<<<<<<<<<<");
}

void timer_cb(uv_timer_t* handle) {
	time_t now = time(0);
	char* now_s = ctime(&now);
	now_s[strlen(now_s)-1] = 0;
	info("timeout: %s, counter: %d", now_s, counter++);
	if (counter % 10 == 0) {
		uv_timer_stop(handle);
	}
}

int main(int argc, char* argv[]) {
	// 初始化参数列表
	args_t args = {
		false, 		// 是否多线程
	};	

	int ret = parse_args(argc, argv, args);
	if (ret != 0) {
		return -1;
	}

#ifdef ZLOG_MESSAGE
	ret = dzlog_init(ZLOG_CONF, "sniffer");
	if (ret != 0) {
		fprintf(stderr, "zlog load configuration file `%s` failed\n", ZLOG_CONF);
		return -1;
	}
#endif //ZLOG_MESSAGE

	uv_loop_t* loop = uv_default_loop();

	// create `idle` handler
	uv_idle_t idler;
	uv_idle_init(loop, &idler);
	uv_idle_start(&idler, idle_cb);

	// setup timer
#ifdef TIMER
	uv_timer_t timer;
	uv_timer_init(loop, &timer);
	uv_timer_start(&timer, timer_cb, 1000, 1000);
#endif
	
	// setup tcp server
	struct sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr);

	uv_tcp_t server;
	uv_tcp_init(loop, &server);
	uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
	ret = uv_listen((uv_stream_t*)&server, DEFAULT_BACKLOG, on_new_connection);
	if (ret) {
		info("listen failed, error: %s", uv_strerror(ret));
		return -1;
	}

	uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}

void on_new_connection(uv_stream_t *server, int status) {
	if (status < 0) {
		error("get new connection failed, error: %s", uv_strerror(status));
		return;
	}

	info("get a new connection!");

	uv_tcp_t* client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), client);
	if (uv_accept(server, (uv_stream_t*) client) == 0) {
		uv_read_start((uv_stream_t *) client, alloc_buffer, read_cb);
	} else {
		uv_close((uv_handle_t *) client, NULL);
	}
}

void read_cb(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
	if (nread > 0) {
		char buffer[1024];
		strcpy(buffer, buf->base);
		buffer[nread-1] = 0;
		info("recv:%s", buffer);
		uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
		uv_buf_t uvBuf = uv_buf_init(buf->base, nread);
		uv_write(req, client, &uvBuf, 1, nullptr);
		return;
	} else if (nread < 0) {
		if (nread != UV_EOF) {
			error("read failed, error: %s", uv_err_name(nread));
		} else {
			error("client disconnect");
		}
		uv_close((uv_handle_t *) client, nullptr);
	}
	if (buf->base != NULL) {
		free(buf->base);
	}
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

// 命令行参数解析
int parse_args(int argc, char* argv[], args_t& args) {
	std::string usage = "[Usage] async [-m|--mt]";
	const char* optstr = "i:w:c:vh";

	struct option opts[] = {
		{"mt", 0, NULL, 'v'},
		{"help", 0, NULL, 'h'},
		{0, 0, 0, 0},
	};
	
	int opt;
	while((opt = getopt_long(argc, argv, optstr, opts, NULL)) != -1){
		switch(opt) {
		case 'v':
			args.mt = true;
			break;
		case 'h':
			fprintf(stderr, "%s\n", usage.c_str());
			return -1;
		case '?':
			if(strchr(optstr, optopt) == NULL){
				fprintf(stderr, "unknown option '-%c'\n", optopt);
			}else{
				fprintf(stderr, "option requires an argument '-%c'\n", optopt);
			}
			return -1;
		}
	}
	return 0;
}






