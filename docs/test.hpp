@startuml

skinparam dpi 300

interface IDevice
{
	+ int open()
	+ void close()
	+ int get_fd()
}

class XTunDevice
{
	+ int open()
	+ void close()
	+ int get_fd()
	- int tun_id
	- int tun_fd
	- const char* ip
	- int mtu
}

class Tun2Socks {
	+ Tun2Socks(int, uv_loop_t*, LWIPStack& lwip)
	+ int start()
	- read_device(uv_poll_s* handle, int status, int events)
	- write_device()
	- int tun_fd
	- uv_loop_t* loop
	- uv_poll_t* poll
	- int status
	- LWIPStack& lwip
}

class LWIPStack {
	+ int init()
	+ int input(u8_t* data, int len)
	- ipver __peek_ipver(u8_t*)
	- proto __peek_nextproto(ipver, u8_t*, int)
	- u8_t __more_frags(ipver, u8_t*)
	- u16_t __frag_offset(ipver, u8_t*)
	- struct tcp_pcb* tpcb
	- struct udp_pcb* upcb
	- static err_t tcp_accept_new_client_cb(void*, struct tcp_pcb*, err_t)
	- static void udp_recv_new_message_cb(void*, struct udp_pcb*, struct pbuf*)
}

class LWIPProxy {
	- int connect_remote_server(struct tcp_pcb*, NatTable*)
	- int disconnect_remote_server()
	- err_t tcp_recv_cb(struct pbuf*)
	- err_t tcp_sent_cb(u16_t)
	- void tcp_error_cb(err_t)
	- err_t tcp_poll_cb()
	- RemoteServer rs
	- raw_api_status_e raw_api_status
	- int retries
	- struct tcp_pcb* newpcb
	- struct pbuf* pbuf_ptr
	- int connect_retries
	- void __tcp_raw_api_close()
	- void __tcp_raw_state_free()
	- int __tcp_raw_send()
}

class RemoteServer {
	+ int connect(struct tcp_pcb*, NatTable*)
	+ int disconnect()
	+ int send(const char*, size_t)
	+ remote_server_status_e get_status()
	- static void __alloc_cb(uv_handle_t*, size_t, uv_buf_t*)
	- int __lwip_to_realserver()
	- remote_server_status_e status
	- struct tcp_pcb* newpcb
	- std::string cache_buffer
	- std::string server_ip
	- int server_port
	- uv_loop_t* loop
	- uv_tcp_t uv_client;
	- uv_connect_t uv_connect
	- uv_write_t uv_write_req
}

class LWIPTimer {
	+ int start()
	+ void restart()
	+ static void check_timeouts(uv_timer_t*)
	- uv_loop_t* loop
	- uv_timer_t timeout_handler
}

IDevice <|-- XTunDevice
XTunDevice <-- Tun2Socks
LWIPStack *-- Tun2Socks
LWIPProxy <-- "factory function" LWIPStack
LWIPProxy <-- RemoteServer

@enduml