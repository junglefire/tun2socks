#include <sys/types.h>
#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>
#include <string>
#include <ctime>

#include <logger.h>
#include <uv.h>

#define ZLOG_CONF "etc/zlog.conf"
#define __CHECK__ 1

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
void main_fun_st();
void main_fun_mt();
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
	counter++;
	if (counter >= 10e6) {
		uv_idle_stop(handle);
	}
	if (counter % 100000 == 0) {
		if (time_a == 0) {
			time_a = now();
		} else {
			time_b = now();
			info("interval: %f", float(time_b-time_a)/1000000);
			time_a = time_b;
		}
	}
	sleep(1);
	info(">>>>>>>>>>>>>");
	uv_print_active_handles(uv_default_loop(), stdout);
	info("-------------");
}

void check_cb(uv_check_t* handle) {
	uv_print_active_handles(uv_default_loop(), stdout);
	info("<<<<<<<<<<<<<");
}

// Async句柄
void close_cb(uv_handle_t* handle) {
	const char* name = (const char*)handle->data;
	info("ASYNC#-`%s` closed", name);
}

void async_cb(uv_async_t* handle) {
	const char* name = (const char*)handle->data;
	info("ASYNC#-`%s` call...", name);
	uv_thread_t id = uv_thread_self();
	info("ASYNC#-`%s` thread id: %lu", name, id);
	uv_close((uv_handle_t*)handle, close_cb);
}

void timer_cb(uv_timer_t* handle) {
	time_t now = time(0);
	info("timeout: %s, counter: %d", ctime(&now), counter);
	sleep(3);
}

// 线程函数
void sub_thread(void* arg) {
	uv_thread_t id = uv_thread_self();
	info("sub thread id: %lu", id);
	uv_async_send((uv_async_t*)(arg));
}

void sub_thread_wait(void* arg) {
	uv_thread_t id = uv_thread_self();
	info("sub thread id: %lu", id);
	uv_async_send((uv_async_t*)(arg));
}

// 多线程测试Async
void main_fun_mt() {
	uv_loop_t* loop = uv_default_loop();

	// 创建IDLE处理器
	uv_idle_t idler;
	uv_idle_init(loop, &idler);
	uv_idle_start(&idler, idle_cb);

	// 创建Check处理器
#ifdef __CHECK__
	uv_check_t checker;
	uv_check_init(loop, &checker);
	uv_check_start(&checker, check_cb);
#endif

	// 定时器
	uv_timer_t timer;
	uv_timer_init(loop, &timer);
	uv_timer_start(&timer, timer_cb, 1000, 1000);

	uv_thread_t id = uv_thread_self();
	info("thread id: %lu", id);

	// 创建异步任务
	const char* async_task1 = "task1";
	const char* async_task2 = "task2";
	const char* async_task3 = "task3";

	uv_async_t async1;
	uv_async_t async2;
	uv_async_t async3;
	async1.data = (void*)async_task1;
	async2.data = (void*)async_task2;
	async3.data = (void*)async_task3;

	uv_async_init(loop, &async1, async_cb);
	uv_async_init(loop, &async2, async_cb);
	uv_async_init(loop, &async3, async_cb);

	//创建子线程
	uv_thread_t thread1;
	uv_thread_t thread2;
	uv_thread_t thread3;
	uv_thread_create(&thread1, sub_thread, (void*)&async1);
	uv_thread_create(&thread2, sub_thread, (void*)&async2);
	uv_thread_create(&thread3, sub_thread, (void*)&async3);

	uv_run(loop, UV_RUN_DEFAULT);
	uv_thread_join(&thread1);
	uv_thread_join(&thread2);
	uv_thread_join(&thread3);
}


// 单线程测试ASYNC
void main_fun_st() {
	uv_loop_t* loop = uv_default_loop();
	uv_thread_t id = uv_thread_self();

	info("thread id: %lu", id);

	uv_async_t async1;
	uv_async_t async2;

	uv_async_init(loop, &async1, async_cb);
	uv_async_init(loop, &async2, async_cb);

	uv_async_send(&async1);
	uv_async_send(&async2);
	uv_run(loop, UV_RUN_DEFAULT);
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

	if (args.mt) {
		main_fun_mt();
	} else {
		main_fun_st();
	}
	return 0;
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






