#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

static void die(const char *s) { perror(s); exit(1); }

//got this function from stack overflow
//https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
int checkDir(char *filepath){
    struct stat buf;
    if (stat(filepath, &buf) != 0)
        return 0;
    return S_ISDIR(buf.st_mode); 
}

int main (int argc, char ** argv){

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        die("signal() failed");

    if (argc != 5){
        printf("usage: <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>\n");
        exit(1);
    }

    
    const char *form = "<h1>mdb-lookup</h1>\n"
        "<p>\n"
        "<form method=GET action=/mdb-lookup>\n"
        "lookup: <input type=text name=key>\n"
        "<input type=submit>\n"
        "</form>\n"
        "<p>\n";

    unsigned short mdbPort = atoi(argv[4]);
    unsigned short port = atoi(argv[1]);
    
    //CREATE MDB SOCKET
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");
    struct sockaddr_in mdbClntAddr;
    memset(&mdbClntAddr, 0, sizeof(mdbClntAddr));
    mdbClntAddr.sin_family = AF_INET;
    mdbClntAddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    mdbClntAddr.sin_port = htons(mdbPort);

    if (connect(sock, (struct sockaddr *) &mdbClntAddr, sizeof(mdbClntAddr)) < 0)
        die("connect failed");

    char *IPaddressB = inet_ntoa(mdbClntAddr.sin_addr);
    
    //****************************************  2a  ***********************8
    int even;
    char *token_separators;
    char *method;
    char *requestURI;
    char *httpVersion;
    char *IPaddressA;
    char *index = "index.html";
    char *slash = "/";
    FILE *requestFile;
    FILE *mdbSearch;
    int j;

    int servsock;
    if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");
   
    //CREATE HTML SOCKET
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    servaddr.sin_port = htons(port);

    // Bind to the local address

    if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        die("bind failed");

    // Start listening for incoming connections

    if (listen(servsock, 5 /* queue size for connection requests */ ) < 0)
        die("listen failed");

    int clntsock;
    socklen_t clntlen;
    struct sockaddr_in clntaddr;

    char buf_in[4096] = "";
    char buf_out[4096] = "";

    while (1) { //initiate client connection loop
        char path_file[1024];
        strcpy(path_file, argv[2]);
    
        // Accept an incoming connection
        clntlen = sizeof(clntaddr); // initialize the in-out parameter

        if ((clntsock = accept(servsock,
                        (struct sockaddr *) &clntaddr, &clntlen)) < 0){
            printf("accept failed. Waiting for new connection.\n");
            goto CloseConnection; //restart loop
        }

        IPaddressA = inet_ntoa(clntaddr.sin_addr);
        FILE *input = fdopen(clntsock, "r");

        fgets(buf_in, sizeof(buf_in), input);

        //IF REQUEST LINE IS NOT A GET REQUEST OR IS NOT HTTP/1.0 OR HTTP/1.1
        if (!strstr(buf_in, "GET") || (!strstr(buf_in, "HTTP/1.0") && !strstr(buf_in, "HTTP/1.1"))){
            
            j = snprintf(buf_out, sizeof(buf_out), "HTTP 1.0 501 Not Implemented\r\n\r\n" "<html><body><h1>501 Not Implemented</h1></body></html>");
            send(clntsock, buf_out, j, 0);

        } else { //received GET request
           
            token_separators = "\t  \r\n";
            method = strtok(buf_in,token_separators);
            requestURI = strtok(NULL, token_separators);
            httpVersion = strtok(NULL, token_separators); //error comes from http being parsed before / is being appended      
            
            //need to check here if URI is for HTML
            strcat(path_file, requestURI); //get full filepath
            if (strstr(requestURI, "cs3157") && !strstr(requestURI, "mdb-lookup")){
                if (requestURI[(int)strlen(requestURI)-1] != '/' && checkDir(path_file) ){ //if it's a directory, add /index.html
                    strcat(path_file, slash);
                    strcat(path_file, index);
                    strcat(requestURI, slash);
                    strcat(requestURI, index);
                } else if (requestURI[strlen(requestURI)-1] == '/'){ //if URI ends in /, append index.html
                    strcat(path_file, index);
                }
            }
            
            //if URI doesn't start with / or contains /../ or /..
            if(requestURI[0] != '/' || strstr(requestURI,"..")  ) { //error check URI
                j = snprintf(buf_out, sizeof(buf_out), "HTTP/1.0 400 Bad Request\r\n\r\n");
                send(clntsock, buf_out, j, 0);
                
                j = snprintf(buf_out, sizeof(buf_out), "<html><body><h1>400 Bad Request</h1></body></html>");
                send(clntsock, buf_out, 1, 0);
                printf("%s \"%s %s %s\" 400 Bad Request\n", IPaddressB, method, requestURI, httpVersion);  
                fclose(input);
                close(clntsock);
                goto CloseConnection;
            }

            if (strstr(requestURI, "mdb-lookup")) { //do 2b code

                if (strcmp(requestURI, "/mdb-lookup") == 0) { //if requestURI is exactly /mdb-lookup
                    j = snprintf(buf_out, sizeof(buf_out), "HTTP/1.0 200 OK \r\n\r\n");
                    send(clntsock, buf_out, j, 0);

                    printf("%s \"%s %s %s\" 200 OK\n", IPaddressB, method, requestURI, httpVersion);
                    send(clntsock, form, strlen(form), 0); //SEND FORM
                }
                
                //IF A SEARCH TERM IS ENTERED
                else if (strstr(requestURI, "/mdb-lookup?key=") && strlen(requestURI) > 16){ //STRONGER TEST, COMPARE FIRST 16 CHARS TO THE STRING
                    mdbSearch = fdopen(sock, "r"); //OPEN FILE FOR READING
                    
                    j = snprintf(buf_out, sizeof(buf_out), "HTTP/1.0 200 OK \r\n\r\n");
                    send(clntsock, buf_out, j, 0); 
                    
                    send(clntsock, form, strlen(form), 0); //SEND FORM
                    
                    printf("%s \"%s %s %s\" 200 OK\n", IPaddressB, method, requestURI, httpVersion);
                    
                    strtok(requestURI, "=");
                    char *searchTerm = strtok(NULL,"=");
                    j = snprintf(buf_out, sizeof(buf_out), "<p><table border>\r\n");
                    send(clntsock, buf_out, j, 0);
                    
                    j = snprintf(buf_out, sizeof(buf_out), "%s", searchTerm);
                    //send(sock, buf_out, j, 0); //send search term to mdb-lookup-server
                    send(sock, searchTerm, strlen(searchTerm), 0);
                    if(send(sock, "\n", 1, 0) != 1){
                        printf("send failed. closing connection\n");
                        close(clntsock);
                        fclose(input);
                        fclose(mdbSearch);
                        goto CloseConnection;
                    }

                   
                    even = 2; 
                    bzero(buf_in,sizeof(buf_in));
                    while (fgets(buf_in, sizeof(buf_in), mdbSearch)){ //want to read database entries line by line
                        if (strlen(buf_in) == 1) break; //breaks on empty line

                        if (even++ % 2 == 0)
                            j = snprintf(buf_out, sizeof(buf_out), "<tr><td>"); //white
                        else
                            j = snprintf(buf_out, sizeof(buf_out), "<tr><td bgcolor=yellow>"); //yellow

                        send(clntsock,buf_out, j, 0); //send table formatting

                        j = snprintf(buf_out, sizeof(buf_out),"%s", buf_in );
                        send(clntsock, buf_out, j, 0); //THIS IS SENDING OUT THE TRAILING NEWLINE AS WELL
                        
                        send(clntsock, "\r\n", 2, 0);
                    }
                    j = snprintf(buf_out, sizeof(buf_out), "</table>\r\n");
                    send(clntsock, buf_out, j, 0);
                    fclose(input);
                    close(clntsock);
                    fclose(mdbSearch);
                    goto CloseConnection;

                    //IF NO SEARCH TERM IS ENTERED
                } else if (strstr(requestURI,"/mdb-lookup?key=") && strlen(requestURI) == 16){
                    mdbSearch = fdopen(sock, "r");

                    j = snprintf(buf_out, sizeof(buf_out), "HTTP/1.0 200 OK \r\n\r\n");
                    send(clntsock, buf_out, j, 0);

                    printf("%s \"%s %s %s\" 200 OK\n", IPaddressB, method, requestURI, httpVersion);
                    
                    send(clntsock, form, strlen(form), 0);//send form
                    
                    j = snprintf(buf_out, sizeof(buf_out), "<p><table border>\r\n");
                    send(clntsock, buf_out, j, 0);
                    send(sock, "\n", 1, 0);

                    even = 2;
                    while (fgets(buf_in, sizeof(buf_in), mdbSearch)){
                        if (strlen(buf_in) == 1) break; 

                        if (even++ % 2 == 0)
                            j = snprintf(buf_out, sizeof(buf_out), "<tr><td>");
                        else
                            j = snprintf(buf_out, sizeof(buf_out), "<tr><td bgcolor=yellow>");
                        
                        send(clntsock,buf_out, j, 0); //send table formatting

                        j = snprintf(buf_out, sizeof(buf_out),"%s", buf_in );
                        send(clntsock, buf_out, j, 0); //send mdb-lookup-server matches
                        
                        send(clntsock, "\r\n", 2, 0);
                    }
                    
                    j = snprintf(buf_out, sizeof(buf_out), "</table>\r\n");
                    send(clntsock, buf_out, j, 0);
                    fclose(input);
                    close(clntsock);
                    fclose(mdbSearch);
                    goto CloseConnection;
                } else {
                    j = snprintf(buf_out, sizeof(buf_out), "HTTP/1.0 400 Bad Request\r\n\r\n");
                    send(clntsock, buf_out, j, 0);
                
                    j = snprintf(buf_out, sizeof(buf_out), "<html><body><h1>400 Bad Request</h1></body></html>");
                    send(clntsock, buf_out, 1, 0);
            
                    printf("%s \"%s %s %s\" 400 Bad Request\n", IPaddressB, method, requestURI, httpVersion);  
                    fclose(input);
                    close(clntsock);
                    goto CloseConnection;
                }
            
                
            }else {//do 2a code

            if ((requestFile = fopen(path_file,"rb")) == NULL){
                j = snprintf(buf_out, sizeof(buf_out), "HTTP/1.0 404 Not Found\r\n\r\n");
                send(clntsock, buf_out, j, 0);
           
                j = snprintf(buf_out, sizeof(buf_out), "<html><body><h1>404 Not Found</h1></body></html>");
                send(clntsock, buf_out, j, 0);

                printf("%s \"%s %s %s\" 404 Not Found\n",IPaddressA,method,requestURI, httpVersion);
                
                //fclose(requestFile);
                fclose(input);
                close(clntsock);
                goto CloseConnection;
            }
            
                        
            //Program comes here if GET request is all good i.e 200
            printf("%s \"%s %s %s\" 200 OK\n",IPaddressA, method, requestURI, httpVersion);

            j = snprintf(buf_out, sizeof(buf_out), "HTTP/1.0 200 OK \r\n\r\n");
            send(clntsock, buf_out, j, 0);
            
            while(fread(buf_out, sizeof(char), sizeof(buf_out), requestFile)){
                send(clntsock, buf_out, sizeof(buf_out), 0);
            }       
            
            fclose(requestFile);
            fclose(input);
            close(clntsock);
        }
        }
        
        CloseConnection:
        even++;
    }
    close(sock);
    return 0;
}
