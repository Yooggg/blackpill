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
uint8_t http_server_tx_buf[4096];
uint8_t http_server_rx_buf[4096];
//
uint8_t http_upload_page[] = "<!DOCTYPE html>\
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


//uint8_t http_explorer_page[] = "<!DOCTYPE html>\
//<html lang=\"en\">\
//<head>\
//    <meta charset=\"UTF-8\">\
//    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
//    <title>Dirs List</title>\
//    <style>\
//        body {\
//            font-family: Arial, sans-serif;\
//            margin: 20px;\
//        }\
//        h1 {\
//            color: #333;\
//        }\
//        ul {\
//            list-style: none;\
//            padding: 0;\
//        }\
//        ul li {\
//            padding: 10px;\
//            margin: 5px 0;\
//            background: #f4f4f4;\
//            border: 1px solid #ccc;\
//        }\
//    </style>\
//</head>\
//<body>\
//    <h1>Dirs</h1>\
//    <ul id=\"dirs-list\"></ul>\
//	<script>\
//    	const dirsList = document.getElementById('dirs-list');\
//        function loadDirs() {\
//            fetch('/api/dirs')\
//                .then(response => response.json())\
//                .then(data => {\
//                    dirsList.innerHTML = '';\
//                    data.dirs.forEach(dir => {\
//                        const li = document.createElement('li');\
//                        li.textContent = dir;\
//                        dirsList.appendChild(li);\
//                    });\
//                })\
//                .catch(error => {\
//                    console.error('Error fetching dirs:', error);\
//                    dirsList.innerHTML = '<li>Error loading dirs</li>';\
//                });\
//        }\
//        loadDirs();\
//    </script>\
//</body>\
//</html>";

const char* http_explorer_page =
"<!DOCTYPE html>"
"<html lang=\"ru\">"
"<head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"<title>File Manager</title>"
"<style>"
"body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5}"
".container{max-width:900px;margin:0 auto;background:white;padding:20px;border:1px solid #ccc}"
"h1{margin:0 0 20px 0;font-size:20px}"
".toolbar{margin-bottom:15px;padding-bottom:15px;border-bottom:1px solid #ddd}"
".toolbar button{padding:8px 15px;margin-right:10px;background:#fff;border:1px solid #999;cursor:pointer}"
".toolbar button:hover{background:#f0f0f0}"
".breadcrumb{margin-bottom:15px;padding:8px;background:#f9f9f9;border:1px solid #ddd;font-size:14px}"
"table{width:100%;border-collapse:collapse}"
"th{background:#f0f0f0;padding:10px;text-align:left;border:1px solid #ccc;font-weight:normal}"
"td{padding:8px;border:1px solid #ddd}"
"tr:hover{background:#f9f9f9}"
".folder{cursor:pointer;color:#0066cc}"
".folder:hover{text-decoration:underline}"
".delete-btn{padding:3px 8px;background:#fff;border:1px solid #999;cursor:pointer;font-size:12px}"
".delete-btn:hover{background:#ffcccc}"
".modal{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.5)}"
".modal.active{display:flex;align-items:center;justify-content:center}"
".modal-content{background:white;padding:20px;border:1px solid #999;min-width:400px}"
".modal-content h3{margin:0 0 15px 0;font-size:16px}"
".modal-content input{width:100%;padding:6px;margin-bottom:15px;border:1px solid #999}"
".modal-content button{padding:6px 15px;margin-right:10px;background:#fff;border:1px solid #999;cursor:pointer}"
".empty{padding:40px;text-align:center;color:#999}"
"#upload-area{border:2px dashed #999;padding:30px;text-align:center;margin-bottom:15px;cursor:pointer}"
"#upload-area:hover{background:#f9f9f9}"
"#file-input{display:none}"
"</style>"
"</head>"
"<body>"
"<div class=\"container\">"
"<h1>File Manager</h1>"
"<div class=\"toolbar\">"
"<button onclick=\"goBack()\">← Back</button>"
"<button onclick=\"showCreateFolderModal()\">New Folder</button>"
"<button onclick=\"showUploadModal()\">Upload File</button>"
"<button onclick=\"refreshList()\">Refresh</button>"
"</div>"
"<div class=\"breadcrumb\">Path: <span id=\"current-path\">/</span></div>"
"<table id=\"file-table\">"
"<thead><tr><th>Name</th><th>Type</th><th width=\"80\">Action</th></tr></thead>"
"<tbody id=\"file-list\"><tr><td colspan=\"3\" class=\"empty\">Loading...</td></tr></tbody>"
"</table>"
"</div>"
"<div class=\"modal\" id=\"create-folder-modal\">"
"<div class=\"modal-content\">"
"<h3>Create New Folder</h3>"
"<input type=\"text\" id=\"folder-name\" placeholder=\"Folder name\">"
"<button onclick=\"createFolder()\">Create</button>"
"<button onclick=\"hideCreateFolderModal()\">Cancel</button>"
"</div>"
"</div>"
"<div class=\"modal\" id=\"upload-modal\">"
"<div class=\"modal-content\">"
"<h3>Upload File</h3>"
"<div id=\"upload-area\" onclick=\"document.getElementById('file-input').click()\">"
"Click to select file or drag and drop here"
"</div>"
"<input type=\"file\" id=\"file-input\" onchange=\"uploadFile()\">"
"<button onclick=\"hideUploadModal()\">Cancel</button>"
"</div>"
"</div>"
"<script>"
"let currentPath='/';"
"async function loadFileList(path){"
"try{"
"const response=await fetch(`/list.cgi?path=${encodeURIComponent(path)}`);"
"const data=await response.json();"
"const fileList=document.getElementById('file-list');"
"fileList.innerHTML='';"
"document.getElementById('current-path').textContent=path;"
"if(data.folders.length===0&&data.files.length===0){"
"fileList.innerHTML='<tr><td colspan=\"3\" class=\"empty\">Empty folder</td></tr>';"
"return;"
"}"
"data.folders.forEach(folder=>{"
"const row=document.createElement('tr');"
"row.innerHTML=`<td class=\"folder\" onclick=\"navigateTo('${path}${folder}/')\">${folder}</td>"
"<td>Folder</td>"
"<td><button class=\"delete-btn\" onclick=\"deleteItem('${path}${folder}',event)\">Delete</button></td>`;"
"fileList.appendChild(row);"
"});"
"data.files.forEach(file=>{"
"const row=document.createElement('tr');"
"row.innerHTML=`<td>${file}</td>"
"<td>File</td>"
"<td><button class=\"delete-btn\" onclick=\"deleteItem('${path}${file}',event)\">Delete</button></td>`;"
"fileList.appendChild(row);"
"});"
"}catch(error){"
"document.getElementById('file-list').innerHTML="
"`<tr><td colspan=\"3\" class=\"empty\">Error: ${error.message}</td></tr>`;"
"}"
"}"
"function navigateTo(path){currentPath=path;loadFileList(path);}"
"function goBack(){"
"if(currentPath==='/') return;"
"const parts=currentPath.split('/').filter(p=>p);"
"parts.pop();"
"currentPath='/'+parts.join('/')+(parts.length>0?'/':'');"
"loadFileList(currentPath);"
"}"
"function refreshList(){loadFileList(currentPath);}"
"function showCreateFolderModal(){"
"document.getElementById('create-folder-modal').classList.add('active');"
"document.getElementById('folder-name').value='';"
"}"
"function hideCreateFolderModal(){"
"document.getElementById('create-folder-modal').classList.remove('active');"
"}"
"async function createFolder(){"
"const folderName=document.getElementById('folder-name').value.trim();"
"if(!folderName){alert('Enter folder name');return;}"
"const fullPath=currentPath+folderName;"
"try{"
"const response=await fetch(`/api/mkdir.cgi?name=${encodeURIComponent(fullPath)}`);"
"const result=await response.text();"
"if(result==='OK'||result==='EXISTS'){"
"hideCreateFolderModal();"
"refreshList();"
"}else{alert('Error creating folder');}"
"}catch(error){alert('Error: '+error.message);}"
"}"
"function showUploadModal(){"
"document.getElementById('upload-modal').classList.add('active');"
"}"
"function hideUploadModal(){"
"document.getElementById('upload-modal').classList.remove('active');"
"}"
"async function uploadFile(){"
"const fileInput=document.getElementById('file-input');"
"const file=fileInput.files[0];"
"if(!file) return;"
"const formData=new FormData();"
"formData.append('file',file);"
"try{"
"const response=await fetch(`/api/upload.cgi?path=${encodeURIComponent(currentPath)}`,{method:'POST',body:formData});"
"const result=await response.text();"
"if(result==='OK'){hideUploadModal();refreshList();}else{alert('Upload error');}"
"}catch(error){alert('Error: '+error.message);}"
"}"
"async function deleteItem(path,event){"
"event.stopPropagation();"
"if(!confirm(`Delete ${path}?`)) return;"
"try{"
"const response=await fetch(`/api/delete.cgi?path=${encodeURIComponent(path)}`);"
"const result=await response.text();"
"if(result==='OK'){refreshList();}else{alert('Delete error');}"
"}catch(error){alert('Error: '+error.message);}"
"}"
"const uploadArea=document.getElementById('upload-area');"
"uploadArea.addEventListener('dragover',(e)=>{e.preventDefault();});"
"uploadArea.addEventListener('drop',(e)=>{"
"e.preventDefault();"
"const files=e.dataTransfer.files;"
"if(files.length>0){"
"document.getElementById('file-input').files=files;"
"uploadFile();"
"}"
"});"
"window.onload=()=>{loadFileList(currentPath);};"
"</script>"
"</body>"
"</html>";



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


	uint8_t buffer[2048];

	httpServer_init(http_server_tx_buf, http_server_rx_buf, 4, http_server_socket_list);
	//reg_httpServer_webContent("upload_file.html", http_upload_page);
	reg_httpServer_webContent("index.html", http_explorer_page);
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
