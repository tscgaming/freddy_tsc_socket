/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/
#include "parse.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define ECHO_PORT 9999
#define BUF_SIZE 4096

const char* bad400 = "HTTP/1.1 400 Bad request\r\n\r\n";
const char* not501 = "HTTP/1.1 501 Not Implemented\r\n\r\n";
const char* ok200 = "HTTP/1.1 200 OK\r\n";
const char* not404 = "HTTP/1.1 404 Not Found\r\n\r\n";
const char* not505 = "HTTP/1.1 505 HTTP Version not supported\r\n\r\n";

int write_log(char *message){
    FILE *f = fopen(".\log.txt", "at+");
    if(f == NULL){
        fprintf(stderr, "Failed writing log file.\n");
    }
    time_t t = time(NULL);
    struct tm *local_time = localtime(&t);
    fprintf(f, "Local time: %d-%d-%d %d:%d:%d\n", 
        local_time->tm_year + 1900, 
        local_time->tm_mon + 1, 
        local_time->tm_mday, 
        local_time->tm_hour, 
        local_time->tm_min, 
        local_time->tm_sec);
    fprintf(f, "%s \r\n\r\n", message);
    fclose(f);
    return 1;
}

const char *get_content_type(const char *file_name) {
    const char *ext = strrchr(file_name, '.'); // 找到最后一个点（.），获取文件扩展名
    if (ext == NULL) {
        return "application/octet-stream";  // 如果没有扩展名，则默认返回通用的二进制类型
    }
    // 转换扩展名为小写字母，避免大小写问题
    ext++;  // 跳过点（.）
    if (strcasecmp(ext, "html") == 0) {
        return "text/html";
    } else if (strcasecmp(ext, "css") == 0) {
        return "text/css";
    } else if (strcasecmp(ext, "js") == 0) {
        return "application/javascript";
    } else if (strcasecmp(ext, "json") == 0) {
        return "application/json";
    } else if (strcasecmp(ext, "png") == 0) {
        return "image/png";
    } else if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(ext, "gif") == 0) {
        return "image/gif";
    } else if (strcasecmp(ext, "txt") == 0) {
        return "text/plain";
    } else if (strcasecmp(ext, "xml") == 0) {
        return "application/xml";
    } else if (strcasecmp(ext, "pdf") == 0) {
        return "application/pdf";
    } else {
        return "application/octet-stream"; // 默认二进制流
    }
};


int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int send_response(int client_sock, const char *response, const char *body, size_t body_size){
    char* print = (char*)malloc(body_size + strlen(ok200));
    int result = (strlen(response) + body_size);
    sprintf(print, "%s%s", response, body);
    write_log(print);
    send(client_sock, print, result, 0);
    free(print);
    return result; 
};

void send_file(int client_sock, const char *path, int ishead) {
    FILE* sdfile = fopen(path, "rb");
    if (sdfile == NULL) {
        write_log("Failed finding file.");
        send_response(client_sock, not404, "", strlen(not404));
        fprintf(stderr, "Failed finding file.\n");
        return;
    }
    char* content_type = get_content_type(path);
    struct stat file_stat;
    if (stat(path, &file_stat) != 0) {
        write_log(not404);
        send_response(client_sock, not404, "", strlen(not404));
        fclose(sdfile);
        fprintf(stderr, "Failed to stat file: %s\n", path);
        return;
    }
    char header[512];
    snprintf(header, sizeof(header),
        "%s"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: keep-alive\r\n\r\n",
        ok200, content_type,file_stat.st_size);
    if(ishead == 1){
    send(client_sock, header, strlen(header), 0);
    fclose(sdfile);  // 就发送头部
    return;
    }
    //执行GET剩余部分
    
    ssize_t bytes_read;
    int bytes_sent;
    // bytes_read = fread(file_buff, BUF_SIZE, file_stat.st_size, sdfile);
    // send(client_sock, file_buff, bytes_read, 0);
    char file_buff[BUF_SIZE];
    bytes_read = fread(file_buff, 1, BUF_SIZE, sdfile);
    send_response(client_sock, header, file_buff, BUF_SIZE);
    fclose(sdfile);
    return ;
    
};

int main(int argc, char* argv[])
{
    int sock, client_sock;
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    char buf[BUF_SIZE];

    fprintf(stdout, "----- Echo Server -----\n");
    
    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {   
        write_log("Failed creating socket");
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ECHO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* servers bind sockets to ports---notify the OS they accept connections */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET,SO_REUSEADDR, (const void *)&opt, sizeof(opt) );
    
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        //associate that socket with a port on host
        close_socket(sock);
        write_log("Failed binding socket");
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }
    
    if (listen(sock, 5))
    {
        close_socket(sock);
        write_log("Error listening on socket.");
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    /* finally, loop waiting for input and then write it back */
    while (1)
    {
        cli_size = sizeof(cli_addr);
        if ((client_sock = accept(sock, (struct sockaddr *) &cli_addr,
                                    &cli_size)) == -1)
        {
            close(sock);
            fprintf(stderr, "Error accepting connection.\n");
            return EXIT_FAILURE;
        }

        readret = 0;
        memset(buf, 0, BUF_SIZE);
        readret = recv(client_sock, buf, BUF_SIZE, 0);
        Request* request = parse(buf, readret);
        
        int backlength;
        if (request == NULL)
        {
            write_log(bad400);
            send(client_sock, bad400, strlen(bad400), 0);
            close_socket(client_sock);
            continue;
        }
        if (strcmp(request -> http_version, "HTTP/1.1") != 0){
            write_log(not505);
            send(client_sock, not505, strlen(not505), 0);
            close_socket(client_sock);
            continue;
        }
        if (strcmp(request -> http_method, "HEAD") == 0)
        {
            char *path;
            if (strcmp(request->http_uri, "/") == 0) {
                path =  "./static_site/index.html";  // 默认页面
            }
            else {
                path = request->http_uri;
            }
            send_file(client_sock, path, 1);
            // backlength = send(client_sock, request->headers, strlen(request->headers), 0);
            close_socket(client_sock);
            continue;
        }
        else if (strcmp(request->http_method, "GET") == 0){
            char* path[50];
            if (strcmp(request->http_uri, "/") == 0) {
                strcat(path, "./static_site/index.html"); // 默认页面
            }
            else {
                strcat(path, "./static_site");
                strcat(path, request->http_uri);
                fprintf(stderr, "%s/r/n", path);
            }
            send_file(client_sock, path, 0);
            // backlength = send(client_sock, request->headers, strlen(request->headers), 0);
            close_socket(client_sock);
            continue;
        }
        else if (strcmp(request->http_method, "POST") == 0){
             // Re-encapsulate the received request and send it back
            write_log("POST 200 OK.");
            backlength = send_response(client_sock, ok200, buf, readret);
            close_socket(client_sock);
            continue;
        }
        else
        {
            // Unsupported method
            write_log(not501);
            send(client_sock, not501, strlen(not501), 0);
            close_socket(client_sock);
            continue;
        }
        while((readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1)
        {
            // if (backlength != readret)
            // {
            //     close_socket(client_sock);
            //     close_socket(sock);
            //     fprintf(stderr, "Error sending to client.\n");
            //     return EXIT_FAILURE;
            // }
            memset(buf, 0, BUF_SIZE);
        } 

        if (readret <= -1)
        {
            close_socket(client_sock);
            close_socket(sock);
            fprintf(stderr, "Error reading from client socket.\n");
            return EXIT_FAILURE;
        }

        if (close_socket(client_sock))
        {
            close_socket(sock);
            fprintf(stderr, "Error closing client socket.\n");
            return EXIT_FAILURE;
        }
    }

    close_socket(sock);

    return EXIT_SUCCESS;
}
