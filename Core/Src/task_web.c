#include "main.h"
#include "FXRTOS.h"
#include <stdio.h>

#include "../Drivers/Wiznet/Ethernet/wizchip_conf.h"
#include "../Drivers/Wiznet/Ethernet/socket.h"
#include "../Drivers/Wiznet/Internet/DHCP/dhcp.h"
#include "../Drivers/Wiznet/Internet/DNS/dns.h"
#include "../Drivers/Wiznet/Internet/httpServer/httpServer.h"

#define DHCP_SOCKET     0
#define DNS_SOCKET      1
#define HTTP_SOCKET     2

extern SPI_HandleTypeDef hspi2;
#define W5500_SPI hspi2
//#define W5500_SPI hspi2

//typedef enum {
//	WB_INIT,
//	WB_WAIT_LINK,
//	WB_WAIT_DHCP,
//	WB_WORK_INIT,
//	WB_WORK
//} WebState_t;
//
//uint8_t rx_tx_buff_sizes[] = {2, 2, 2, 2, 2, 2, 2, 2};
//
uint8_t http_server_socket_list[] = {2, 3, 4, 5};
//
uint8_t http_server_tx_buf[1024];
uint8_t http_server_rx_buf[1024];
//
uint8_t http_server_page[] = "<!DOCTYPE html>\
<html lang=\"en\">\
<head>\
    <meta charset=\"UTF-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
    <title>File Upload</title>\
</head>\
<body>\
    <h1>Upload File</h1>\
    <form action=\"/upload.cgi\" method=\"POST\" enctype=\"multipart/form-data\">\
        <input type=\"file\" name=\"file\" required>\
        <button type=\"submit\">Upload</button>\
    </form>\
</body>\
</html>";
//
//wiz_NetInfo net_info = {
//		.mac  = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0xEA },
//		.dhcp = NETINFO_DHCP
//};


//WebState_t webState = WB_INIT;
//
////volatile bool ip_assigned = false;
//volatile bool linkOn = false;
//volatile bool linkDisconnectedEvent = false;
//
//volatile uint8_t dhcpState = DHCP_STOPPED;
//volatile bool dhcpIpChangedEvent = false;
//
//volatile bool timer1secEvent = false;
//

// 1K should be enough, see https://forum.wiznet.io/t/topic/1612/2
uint8_t dhcp_buffer[1024];
// 1K seems to be enough for this buffer as well
uint8_t dns_buffer[1024];

//static fx_timer_t timer_1s;

fx_mutex_t mutex;

void Task_Web_Func();
//void Task_Flash_Check();
int Callback_Timer_1s();

void Task_Web_Init()
{
	static fx_thread_t task_web;
	static uint32_t stack[1024*30 / sizeof(uint32_t)];

	// Настроим трассировщик
//	trace_set_thread_log_lvl(&(task.trace_handle), THREAD_LOG_LVL_DEFAULT | TRACE_THREAD_PRIO_KEY | TRACE_THREAD_EVENT_KEY /*| TRACE_THREAD_CS_ONLY_KEY*/);

	fx_mutex_init(&mutex, FX_MUTEX_CEILING_DISABLED, FX_SYNC_POLICY_DEFAULT);

	// Основной таск
	fx_thread_init(&task_web, Task_Web_Func, NULL, 11, (void*)stack, sizeof(stack), false);
	//fx_thread_init(&task, Task_Flash_Check, NULL, 11, (void*)stack, sizeof(stack), false);
}

/*
void Callback_IPAssigned(void) {
    printf("Callback: IP assigned! Leased time: %lu sec\r\n", getDHCPLeasetime());
    ip_assigned = true;
}

void Callback_IPConflict(void) {
	printf("Callback: IP conflict!\r\n");
}
*/

//void W5500_CriticalEnter()
//{
//    fx_mutex_acquire(&mutex, NULL);
//}
//
//void W5500_CriticalExit()
//{
//    fx_mutex_release(&mutex);
//}
//
//void W5500_ChipSelect()
//{
//	__HAL_SPI_ENABLE(&W5500_SPI);
//}
//
//void W5500_ChipDeselect()
//{
//	__HAL_SPI_DISABLE(&W5500_SPI);
//}
//
//uint8_t W5500_ReadByte(void) {
//    uint8_t data;
//    HAL_SPI_Receive(&W5500_SPI, &data, 1, HAL_MAX_DELAY);
//    return data;
//}
//
//void W5500_WriteByte(uint8_t data) {
//    HAL_SPI_Transmit(&W5500_SPI, &data, 1, HAL_MAX_DELAY);
//}
//
//void W5500_ReadBuff(uint8_t* buff, uint16_t len) {
//    HAL_SPI_Receive(&W5500_SPI, buff, len, HAL_MAX_DELAY);
//}
//
//void W5500_WriteBuff(uint8_t* buff, uint16_t len) {
//    HAL_SPI_Transmit(&W5500_SPI, buff, len, HAL_MAX_DELAY);
//}

//int Callback_Timer_1s()
//{
//	timer1secEvent = true;
//	return 0;
//}

volatile bool ip_assigned = false;

void Callback_IPAssigned(void) {
    printf("Callback: IP assigned! Leased time: %d sec\r\n",
                getDHCPLeasetime());
    ip_assigned = true;
}

void Callback_IPConflict(void) {
    printf("Callback: IP conflict!\r\n");
}

void W5500_Select(void) {
    HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin,
                      GPIO_PIN_RESET);
}

void W5500_Unselect(void) {
    HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin,
                      GPIO_PIN_SET);
}

void W5500_ReadBuff(uint8_t* buff, uint16_t len) {
    HAL_SPI_Receive(&hspi2, buff, len, HAL_MAX_DELAY);
}

void W5500_WriteBuff(uint8_t* buff, uint16_t len) {
    HAL_SPI_Transmit(&hspi2, buff, len, HAL_MAX_DELAY);
}

uint8_t W5500_ReadByte(void) {
    uint8_t byte;
    W5500_ReadBuff(&byte, sizeof(byte));
    return byte;
}

void W5500_WriteByte(uint8_t byte) {
    W5500_WriteBuff(&byte, sizeof(byte));
}

void Task_Web_Func()
{

	printf("Registering W5500 callbacks...\r\n");
	reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
	reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
	reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);

	printf("Calling wizchip_init()...\r\n");
	uint8_t rx_tx_buff_sizes[] = {2, 2, 2, 2, 2, 2, 2, 2};
	wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);

	printf("Calling DHCP_init()...\r\n");
	wiz_NetInfo net_info = {
	    .mac  = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0xEA },
	    .dhcp = NETINFO_DHCP
	};
	// set MAC address before using DHCP
	setSHAR(net_info.mac);
	DHCP_init(DHCP_SOCKET, dhcp_buffer);

	printf("Registering DHCP callbacks...\r\n");
	reg_dhcp_cbfunc(
	    Callback_IPAssigned,
	    Callback_IPAssigned,
	    Callback_IPConflict
	);

	printf("Calling DHCP_run()...\r\n");
	// actually should be called in a loop, e.g. by timer
	uint32_t ctr = 10000;
	while((!ip_assigned) && (ctr > 0)) {
	    DHCP_run();
	    ctr--;
	}
	if(!ip_assigned) {
	    printf("\r\nIP was not assigned :(\r\n");
	    return;
	}

	getIPfromDHCP(net_info.ip);
	getGWfromDHCP(net_info.gw);
	getSNfromDHCP(net_info.sn);

	uint8_t dns[4];
	getDNSfromDHCP(dns);

	printf(
	    "IP:  %d.%d.%d.%d\r\n"
	    "GW:  %d.%d.%d.%d\r\n"
	    "Net: %d.%d.%d.%d\r\n"
	    "DNS: %d.%d.%d.%d\r\n",
	    net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3],
	    net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3],
	    net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3],
	    dns[0], dns[1], dns[2], dns[3]
	);

	printf("Calling wizchip_setnetinfo()...\r\n");
	wizchip_setnetinfo(&net_info);




















//	printf("Calling DNS_init()...\r\n");
//	DNS_init(DNS_SOCKET, dns_buffer);
//
//	uint8_t addr[4];
//	{
//	    char domain_name[] = "some.me";
//	    printf("Resolving domain name \"%s\"...\r\n", domain_name);
//	    int8_t res = DNS_run(dns, (uint8_t*)&domain_name, addr);
//	    if(res != 1) {
//	        printf("DNS_run() failed, res = %d", res);
//	        return;
//	    }
//	    printf("Result: %d.%d.%d.%d\r\n",
//	                addr[0], addr[1], addr[2], addr[3]);
//	}
//
//
//	printf("Creating socket...\r\n");
//	uint8_t http_socket = HTTP_SOCKET;
//	uint8_t code = socket(http_socket, Sn_MR_TCP, 10888, 0);
//	if(code != http_socket) {
//	    printf("socket() failed, code = %d\r\n", code);
//	    return;
//	}
//
//	printf("Socket created, connecting...\r\n");
//	code = connect(http_socket, addr, 80);
//	if(code != SOCK_OK) {
//	    printf("connect() failed, code = %d\r\n", code);
//	    close(http_socket);
//	    return;
//	}
//
//	char req[] = "GET / HTTP/1.0\r\nHost: some.me\r\n\r\n";
//	uint16_t len = sizeof(req) - 1;
//	uint8_t* buff = (uint8_t*)&req;
//	while(len > 0) {
//	    printf("Sending %d bytes...\r\n", len);
//	    int32_t nbytes = send(http_socket, buff, len);
//	    if(nbytes <= 0) {
//	        printf("send() failed, %d returned\r\n", nbytes);
//	        close(http_socket);
//	        return;
//	    }
//	    printf("%d bytes sent!\r\n", nbytes);
//	    len -= nbytes;
//	}
//
//
//	char buff1[32];
//	for(;;) {
//	    int32_t nbytes = recv(http_socket,
//	                          (uint8_t*)&buff1, sizeof(buff1)-1);
//	    if(nbytes == SOCKERR_SOCKSTATUS) {
//	        printf("\r\nConnection closed.\r\n");
//	        break;
//	    }
//
//	    if(nbytes <= 0) {
//	        printf("\r\nrecv() failed, %d returned\r\n", nbytes);
//	        break;
//	    }
//
//	    buff1[nbytes] = '\0';
//	    printf("%s", buff1);
//	}
//
//	printf("Closing socket.\r\n");
//	close(http_socket);










	uint8_t buffer[2048];

	httpServer_init(http_server_tx_buf, http_server_rx_buf, 4, http_server_socket_list);
	reg_httpServer_webContent("test_page.html", http_server_page);
	while(1){
		//fx_thread_yield();
		httpServer_run(http_server_socket_list[0]);
		if (getSn_SR(0) == SOCK_ESTABLISHED) {
		        int16_t len = getSn_RX_RSR(0); // Получаем количество данных
		        if (len > 0) {
		            len = recv(0, buffer, sizeof(buffer)); // Читаем данные
		            handle_post_request(buffer, len); // Обрабатываем POST-запрос
		        }
		    } else if (getSn_SR(0) == SOCK_CLOSE_WAIT) {
		        disconnect(0);
		        close(0);
		    }
	}
	//printf("\n\r HTTP server is working\r\n");
















//	wiz_PhyConf phyconf;
//
//	fx_thread_sleep(1500);
//
//	W5500_ChipDeselect();
//
//	printf("Registering W5500 callbacks...\r\n");
//	reg_wizchip_cris_cbfunc(W5500_CriticalEnter, W5500_CriticalExit);
//	reg_wizchip_cs_cbfunc(W5500_ChipSelect, W5500_ChipDeselect);
//	reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
//	reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);
//
//	fx_timer_init(&timer_1s, Callback_Timer_1s, NULL);
//	fx_timer_set_abs(&timer_1s, 1000, 1000);
//
//	while (1)
//	{
//		fx_thread_yield();
//
//		// События 1 раз всекунду
//		if (timer1secEvent)
//		{
//			timer1secEvent = false;
//
//			httpServer_time_handler();
//			httpServer_run(http_server_socket_list[0]);
//
//			DHCP_time_handler();
//			dhcpState = DHCP_run();
//			if (dhcpState == DHCP_IP_CHANGED)
//			{
//				dhcpIpChangedEvent = true;
//			}
//
//			bool linkOn_new = wizphy_getphylink() == PHY_LINK_ON ? true : false;
//			if (linkOn != linkOn_new)
//			{
//				// состояние изменилось
//				if (linkOn_new)
//				{
//					// Connect
//				}
//				else
//				{
//					// Disconnect
//					linkDisconnectedEvent = true;
//				}
//
//				linkOn = linkOn_new;
//			}
//		}
//
//		// Если нет связи - перезапускаемся
//		if (linkDisconnectedEvent)
//		{
//			linkDisconnectedEvent = false;
//			webState = WB_INIT;
//
//			// Тут надо закрыть все соединения и сервисы
//			DHCP_stop();
//			printf("Link Off\r\n");
//		};
//
//		// Отработаем текущее состояние
//		switch (webState) {
//
//			case WB_INIT:
//				printf("Calling wizchip_init()...\r\n");
//				wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);
//
//				webState = WB_WAIT_LINK;
//				break;
//
//			case WB_WAIT_LINK:
//				if (linkOn)
//				{
//					dhcpIpChangedEvent = false;
//					printf("\r\nLink On:");
//
//					wizphy_getphystat(&phyconf);
//					printf(" %s Mbit/s,", phyconf.speed == PHY_SPEED_100 ? "100" : "10");
//					printf(" %s duplex", phyconf.duplex == PHY_DUPLEX_FULL ? "Full" : "Half");
//
//					printf("\r\n\r\n");
//
//					printf("Calling DHCP_init()...\r\n");
//					// set MAC address before using DHCP
//					setSHAR(net_info.mac);
//					DHCP_init(DHCP_SOCKET, dhcp_buffer);
//					webState = WB_WAIT_DHCP;
//				};
//				break;
//
//			case WB_WAIT_DHCP:
//				if (dhcpState == DHCP_IP_LEASED)
//				{
//					// получили IP - выходим на рабочий цикл
//			        printf("DHCP IP LEASED\r\n");
//
//			        getIPfromDHCP(net_info.ip);
//			        getGWfromDHCP(net_info.gw);
//			        getSNfromDHCP(net_info.sn);
//			        getDNSfromDHCP(net_info.dns);
//
//			        printf(
//			  	      "IP:  %d.%d.%d.%d\r\n"
//			  	      "GW:  %d.%d.%d.%d\r\n"
//			  	      "Net: %d.%d.%d.%d\r\n"
//			  	      "DNS: %d.%d.%d.%d\r\n",
//			  	      net_info.ip[0],  net_info.ip[1],  net_info.ip[2],  net_info.ip[3],
//			  	      net_info.gw[0],  net_info.gw[1],  net_info.gw[2],  net_info.gw[3],
//			  	      net_info.sn[0],  net_info.sn[1],  net_info.sn[2],  net_info.sn[3],
//					  net_info.dns[0], net_info.dns[1], net_info.dns[2], net_info.dns[3]
//			        );
//
//			        printf("Calling wizchip_setnetinfo()...\r\n");
//			        wizchip_setnetinfo(&net_info);
//
//			        webState = WB_WORK_INIT;
//				}
//				break;
//
//			case WB_WORK_INIT:
//				// Инициализация сервисов, например, HTTP-сервера
//				httpServer_init(http_server_tx_buf, http_server_rx_buf, 4, http_server_socket_list);
//				reg_httpServer_webContent("test_page.html", http_server_page);
//				httpServer_run(http_server_socket_list[0]);
//
//				webState = WB_WORK;
//				break;
//
//			case WB_WORK:
//				break;
//
//			default:
//				break;
//		};
//
//	};

//		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
//		fx_thread_sleep(700);
//
//		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
//		fx_thread_sleep(100);
//	}
}

void handle_post_request(uint8_t* buffer, uint16_t len) {
    // Проверяем заголовок на наличие "multipart/form-data"
    if (strstr((char*)buffer, "Content-Type: multipart/form-data")) {
        char* boundary = strstr((char*)buffer, "boundary=");
        if (boundary) {
            boundary += 9; // Пропускаем "boundary="
            char boundary_str[128] = {0};
            sscanf(boundary, "%127s", boundary_str); // Считываем boundary

            // Найти начало данных файла
            char* file_data = strstr((char*)buffer, "\r\n\r\n");
            if (file_data) {
                file_data += 4; // Пропускаем заголовки

                // Открыть файл на запись
                FILE* fp = fopen("uploaded_file.bin", "wb");
                if (fp) {
                    fwrite(file_data, 1, len - (file_data - (char*)buffer), fp);
                    fclose(fp);
                }
            }
        }
    }

    // Отправить ответ клиенту
    const char* response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 2\r\n"
                           "\r\n"
                           "OK";
    send(0, (uint8_t*)response, strlen(response));
}
