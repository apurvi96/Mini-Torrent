
#include<iostream> //basic inp outpuy
#include<stdio.h>
#include<string.h>
#include<sys/socket.h> //for socket creation
#include<arpa/inet.h> // in_addr structure
#include<sys/types.h> // defines various basic derived types
#include<errno.h>
#include<unistd.h> //read write
#include<cstring>
#include<string>
#include<pthread.h>
#include<openssl/sha.h>
#include<bits/stdc++.h>
using namespace std;
pthread_mutex_t lockem;
#define CHUNK_SIZE 512
unordered_map<string,vector<int>>peer_file;

//In thread we can pass only one parameter to thread function therefore use structure to pass multiple values
struct DataforThread
{
	int fd;
	int tport;
	int pport;
	char *file;
	int chunk;
	FILE* filefd;
	vector<int>chunk_d;
	int *chunkp;
};
typedef struct DataforThread threaddata;

//sha caculation
string sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH])
{
   stringstream ss;
   for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}
string sha256(const string str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

string sha256_file(FILE *file,int file_size)
{
    if(!file) return NULL;
    string finalHash="";
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 512;
    unsigned char *buffer =(unsigned char*) malloc(bufSize+1);
    int bytesRead = 0;
    if(!buffer) return NULL;
	int i=0;
	while((bytesRead = fread(buffer, sizeof(char), bufSize, file)))
	{
		SHA256_Update(&sha256, buffer, bytesRead);
		SHA256_Final(hash, &sha256);
	        string outputBuffer = sha256_hash_string(hash);
		string finalAnswer = outputBuffer.substr(0, 20);
		finalHash += finalAnswer;
	        memset ( buffer , '\0', 512);
	}

    fclose(file);
    free(buffer);
    return finalHash;
    }


void get_chunks(vector<vector<int>>&chunks_to_take,int ci,string c_port,string c_ip,string fname)
{
		cout<<"enter get_chunks"<<endl;
		cout<<"ci is "<<ci<<endl;
		//socket create
		int s_fd=socket(AF_INET,SOCK_STREAM,0);

		if(s_fd<0)
		{
			perror("ERROR IN SOCKET CREATION");
			pthread_exit(NULL);

		}

		//setopt 
		int opt=3;
		int setopt=setsockopt(s_fd,SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT ,&opt,sizeof(opt));

		if(setopt<0)
		{
			perror(" SOCKOPT FAILED");
			pthread_exit(NULL);

		}
		int cportn=stoi(c_port);

		cout<<"cpotn in funcn is "<<cportn<<endl;
		const char* c_ipn=(const char *)c_ip.c_str();
		cout<<c_ipn<<endl;
		//server structure
		struct sockaddr_in ip_server;
		ip_server.sin_family=AF_INET;
		ip_server.sin_addr.s_addr =inet_addr(c_ipn);

		ip_server.sin_port=htons(cportn);

		int con=connect(s_fd,(struct sockaddr*)&ip_server,sizeof(ip_server));
		if(con<0)
		{
			perror("CANNOT CONNECT");
				pthread_exit(NULL);

			//break;
			//exit(1);
		}
		int ack=1;
		string msg="request_chunk";
		send(s_fd,(char *)msg.c_str(),sizeof(msg),0);
		recv(s_fd,&ack,sizeof(ack),0);
		cout<<"fname in func "<<fname<<endl;
		send(s_fd,(char *)fname.c_str(),sizeof(fname),0);
		
		int chunk_v_s;
		recv(s_fd,&chunk_v_s,sizeof(chunk_v_s),0);
		cout<<"received "<<chunk_v_s<<endl;
		send(s_fd,&ack,sizeof(ack),0);
		for(int i=0;i<chunk_v_s;i++)
		{
			int chunk_f;
			recv(s_fd,&chunk_f,sizeof(chunk_f),0);
			cout<<"received chunk_f "<<chunk_f<<endl;
			chunks_to_take[ci].push_back(chunk_f);
			//chunks_to_take[ci][i]=chunk_f;
			//cout<<ci<<" in vector "<<chunks_to_take[ci][i]<<endl;
			send(s_fd,&ack,sizeof(ack),0);
		}
		recv(s_fd,&ack,sizeof(ack),0);


}

void piece_selection(vector<vector<int>>&final_chunk,vector<vector<int>>&chunks_to_take,int no_chunks,int clients_n)
{
	cout<<"in peice "<<endl;
	cout<<"clients "<<clients_n<<endl;
	cout<<"no chunks "<<no_chunks<<endl;
	vector<int>visited(no_chunks+1,0);
	int visit_c=0;
	for(int i=0;i<clients_n;i=(i+1)%clients_n)
	{
		//cout<<"inside for "<<i<<endl;
		//cout<<"visited chunk "<<visited[1]<<endl;
		cout<<"value before if"<<chunks_to_take[i][0]<<endl;
		cout<<"visited "<<visited[chunks_to_take[i][0]]<<endl;
		if(chunks_to_take[i].size()!=0 && visited[chunks_to_take[i][0]]==0)
		{
			int value=chunks_to_take[i][0];
			cout<<"value "<<chunks_to_take[i][0]<<endl;
			cout<<"entered"<<endl;
			final_chunk[i].push_back(chunks_to_take[i][0]);
			chunks_to_take[i].erase(chunks_to_take[i].begin());
			visit_c++;
			visited[value]=1;
			//cout<<"in for "<<visited[]<<endl;
		}

		else if(chunks_to_take[i].size()!=0 && visited[chunks_to_take[i][0]]==1)
		{
			cout<<"enter already visted "<<endl;
			chunks_to_take[i].erase(chunks_to_take[i].begin());
			i=i-1;
		}
		if(visit_c==no_chunks)
		{
			break;
		}


	}
}

void *client_download_f(void *downarg)
{

	cout<<"inside download "<<endl;
	threaddata *client_d=(threaddata *)downarg;
	int cportn=client_d->pport;
	FILE* fop=client_d->filefd;
	//vector<int>&chunk_v=client_d->chunk_d;
//	cout<<chunk_v[0]<<endl;
//	int chunk_s=chunk_v[0];
	string filep=client_d->file;
	cout<<"filep "<<filep<<endl;
	cout<<"filefd is "<<fop<<"\n";
	cout<<"cportn "<<cportn<<endl;
	// for(int i=0;i<client_d->chunk_d.size();i++)
	// {
	// cout<<"chunk "<<client_d->chunk_d[i]<<endl;
	// }
	for(int i=0;i<client_d->chunk_d.size();i++)
	{

		//socket create
			int s_fd=socket(AF_INET,SOCK_STREAM,0);

			if(s_fd<0)
			{
				perror("ERROR IN SOCKET CREATION");
				pthread_exit(NULL);

			}

		//setopt 
			int opt=3;
			int setopt=setsockopt(s_fd,SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT ,&opt,sizeof(opt));

			if(setopt<0)
			{
				perror(" SOCKOPT FAILED");
				pthread_exit(NULL);

			}

			//server structure
				struct sockaddr_in ip_server;
				ip_server.sin_family=AF_INET;
				ip_server.sin_addr.s_addr =inet_addr("127.0.0.1");
			//	cout<<"cport is\n"<<cportn<<"\n";
				ip_server.sin_port=htons(cportn);	

			//connecting to server
			char inp;
			//cout<<"enter c to connect";
			//cin>>inp;
			int con;

			//if(inp=='c')
			//{
				con=connect(s_fd,(struct sockaddr*)&ip_server,sizeof(ip_server));
					if(con<0)
					{
						perror("CANNOT CONNECT");
							pthread_exit(NULL);

						//break;
						//exit(1);
					}
					cout<<"connected tp server "<<endl;
			int ack=1;
			string msg="request_data";
			send(s_fd,(char *)msg.c_str(),sizeof(msg),0);
			recv(s_fd,&ack,sizeof(ack),0);
			send(s_fd,(char *)filep.c_str(),sizeof(filep),0);
			recv(s_fd,&ack,sizeof(ack),0);

			
			
					cout<<"chunk "<<client_d->chunk_d[i]<<endl;
					int chunk=client_d->chunk_d[i];
					send(s_fd,&chunk,sizeof(chunk),0);
					recv(s_fd,&ack,sizeof(ack),0);

					int fsize=512;
			int pos=(chunk-1)*fsize;
			    pthread_mutex_lock(&lockem); 
			rewind(fop);

			fseek(fop,pos,SEEK_SET);
			cout<<"writing at "<<pos<<"\n";
			cout<<"chunk is "<<chunk<<"\n";

		char buffer[512]={0};

			int byteread;

			while(fsize>0 && (byteread=recv(s_fd,buffer,512,0))>0)
			{
				cout<<"bufffer coming as "<<buffer<<"\n";
				//cout<<"data witen "<<;
				fwrite(buffer,sizeof(char),byteread,fop);
				memset(buffer,'\0',512);
				fsize=fsize-byteread;
			}
				    pthread_mutex_unlock(&lockem); 
			//send(s_fd,&ack,sizeof(ack),0);
					cout<<"out "<<endl;
					close(s_fd);


	}
					
			
		pthread_exit(NULL);

	

}





void *client(void *clientarg)
{

	//cout<<"client in peert\n";
	threaddata *cport=(threaddata *)clientarg;
	int cportn=cport->tport;
	int pportn=cport->pport;

		//socket create
				int s_fd=socket(AF_INET,SOCK_STREAM,0);
					if(s_fd<0)
					{
						perror("ERROR IN SOCKET CREATION");
						pthread_exit(NULL);
					}


		//setopt 
				int opt=3;
				int setopt=setsockopt(s_fd,SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT ,&opt,sizeof(opt));
				if(setopt<0)
				{
					perror(" SOCKOPT FAILED");
					pthread_exit(NULL);		
				}

		//server structure

			struct sockaddr_in ip_server;
			ip_server.sin_family=AF_INET;
			ip_server.sin_addr.s_addr =inet_addr("127.0.0.1");
			//cout<<"cport is\n"<<cportn<<"\n";
			ip_server.sin_port=htons(cportn);

			//client structure
				struct sockaddr_in ip_client;
				memset(&ip_client,'0',sizeof(ip_client));

				ip_client.sin_family=AF_INET;
				ip_client.sin_addr.s_addr =inet_addr("127.0.0.1");
				//cout<<"port of peer in client "<<pportn<<endl;
				ip_client.sin_port=htons(pportn);

	int bd=bind(s_fd,(struct sockaddr *)&ip_client,sizeof(ip_client));


	//connecting to server
			//char inp[2048];
			string input;
			int flag=0;
				int con=connect(s_fd,(struct sockaddr*)&ip_server,sizeof(ip_server));
					if(con<0)
					{
						perror("CANNOT CONNECT");
						pthread_exit(NULL);
					}
					cin.ignore();
					string username="";

			while(1)
			{

					char* command;
				int rack,sack=1;
				char msg[1024]={'\0'};


					cout<<"\nenter command to connect\n";
					//char ar[100];
					string input;
					getline(cin,input);
					string input1=input;
					//char inp1[2048];
					////strcpy(inp1,inp);
					//cout<<input<<" ";
					command=strtok((char *)input.c_str()," ");
					//cout<<"\ninput after strtok "<<input<<"\n";
					//cout<<"command is "<<command<<endl;
					

//----------------------------------------------------------------------------------------------------------					
					
					if(strcmp(command,"create_user")==0)
					{
						char *user=strtok(NULL," ");
						char *password=strtok(NULL," ");
						cout<<"enter create_user"<<endl;

						if(user==NULL || password==NULL)
						{
							cout<<"INVALID COMMAND\n";
							continue;
						}
						//cout<<"give username:"<<endl;
						//getline(cin,username);
						//cin.ignore();


						//int a=10;
						//cout<<"sending command "<<input1<<"\n";
						send(s_fd,(char *)input1.c_str(),sizeof(input1),0);
						//int ack;
						recv(s_fd,msg,sizeof(msg),0);
						if(strcmp(msg,"USER_ALREADY")==0)
						{
							cout<<"USER ALREADY PRESENT\n";
							send(s_fd,&sack,sizeof(sack),0);
							continue;
						}

						else if(strcmp(msg,"success")==0)
						{
							cout<<"USER CREATED SUCCESFULLY\n";

			 				//char msg[1024]="success";
							send(s_fd,&sack,sizeof(sack),0);
							continue;
						}

						
							
					}
//-------------------------------------------------------------------------
					if(strcmp(command,"login")==0)

					{
						cout<<"inside login\n";

						char *user=strtok(NULL," ");
						char *password=strtok(NULL," ");
						if(user==NULL || password==NULL)
						{
							cout<<"INVALID COMMAND\n";
							continue;
						}
						if(flag==1)
						{
							cout<<"\nALREADY LOGIN AT THIS TERMINAL !!!\n";
							continue;
						}
						
						
						//username=strtok(NULL," ");
						send(s_fd,(char *)input1.c_str(),sizeof(input1),0);
						memset(msg,'\0',sizeof(msg));
						recv(s_fd,msg,sizeof(msg),0);

						if(strcmp(msg,"correct")==0)
						{
							cout<<"LOGIN SUCCESSFUL\n";
							flag=1;
							username=user;
							//cout<<"username is "<<username<<endl;

						}

						else if(strcmp(msg,"incorrect")==0)
						{
							cout<<"INCORRECT DATA OR USER DOESNT EXIST!!! \n";
						}

						send(s_fd,&sack,sizeof(sack),0);
						continue;



					}
//-------------------------------------------------------------------------------------------------					
				if(strcmp(command,"logout")==0)

					{
						cout<<"inside logout\n";


						
						//username=strtok(NULL," ");
						if(flag==0)
						{
							cout<<"\nPLEASE LOGIN!!!\n";
							continue;
						}

						//send data
						string tosend="";
						tosend=input1+" "+username;
						send(s_fd,(char *)tosend.c_str(),sizeof(tosend),0);
						//recieve msg
						memset(msg,'\0',sizeof(msg));
						recv(s_fd,msg,sizeof(msg),0);

						if(strcmp(msg,"success")==0)
						{
							cout<<"LOGOUT SUCCESSFULLY\n";
							flag=0;
							
							
							//cout<<"username is "<<username<<endl;

						}

						else if(strcmp(msg,"used")==0)
						{
							cout<<"ERROR LOGGING OUT\n";
						}

						send(s_fd,&sack,sizeof(sack),0);
						continue;
					}

//---------------------------------------------------------------------------------
					if(strcmp(command,"create_group")==0)

					{
						cout<<"inside create_group\n";


						//checks if command parameter are correct
						char* token;
						if((token=strtok(NULL," "))==NULL)
						{
							cout<<"INVALID COMMAND\n";
							continue;
						}
						//username=strtok(NULL," ");
						if(flag==0)
						{
							cout<<"\nPLEASE LOGIN!!!\n";
							continue;
						}

						//send data
						string tosend="";
						tosend=input1+" "+username;
						send(s_fd,(char *)tosend.c_str(),sizeof(tosend),0);
						
						//recieve msg
						memset(msg,'\0',sizeof(msg));
						recv(s_fd,msg,sizeof(msg),0);

						if(strcmp(msg,"success")==0)
						{
							cout<<"GROUP CREATED SUCCESSFULLY\n";
							
							
							//cout<<"username is "<<username<<endl;

						}

						else if(strcmp(msg,"used")==0)
						{
							cout<<"ALREADY CREATED\n";
						}

						send(s_fd,&sack,sizeof(sack),0);
						continue;
					}
//-------------------------------------------------------------------------------------

					if(strcmp(command,"join_group")==0)

					{
						cout<<"inside join_group\n";


						//checks if command parameter are correct
						char* token;
						if((token=strtok(NULL," "))==NULL)
						{
							cout<<"INVALID COMMAND\n";
							continue;
						}
						//username=strtok(NULL," ");
						if(flag==0)
						{
							cout<<"\nPLEASE LOGIN!!!\n";
							continue;
						}

						//send data
						string tosend="";
						tosend=input1+" "+username;
						send(s_fd,(char *)tosend.c_str(),sizeof(tosend),0);
						
						//recieve msg
						memset(msg,'\0',sizeof(msg));
						recv(s_fd,msg,sizeof(msg),0);

						if(strcmp(msg,"success")==0)
						{
							cout<<"GROUP  REQUEST SENT \n";
							
							
							//cout<<"username is "<<username<<endl;

						}

						else if(strcmp(msg,"fail")==0)
						{
							cout<<"NO SUCH GROUP !!!\n";
						}

						else if(strcmp(msg,"already")==0)
						{
							cout<<"ALREADY IN GROUP !!!\n";
						}


						send(s_fd,&sack,sizeof(sack),0);
						continue;
					}

//--------------------------------------------------------------------------------------------------
					//-----------remaining-----------------
					if(strcmp(command,"leave_group")==0)

					{
						cout<<"inside leave_group\n";


						//checks if command parameter are correct
						char* token;
						if((token=strtok(NULL," "))==NULL)
						{
							cout<<"INVALID COMMAND\n";
							continue;
						}
						//username=strtok(NULL," ");
						if(flag==0)
						{
							cout<<"\nPLEASE LOGIN!!!\n";
							continue;
						}

						//send data
						string tosend="";
						tosend=input1+" "+username;
						send(s_fd,(char *)tosend.c_str(),sizeof(tosend),0);
						
						//recieve msg
						memset(msg,'\0',sizeof(msg));
						recv(s_fd,msg,sizeof(msg),0);

						if(strcmp(msg,"success")==0)
						{
							cout<<"LEFT GROUP\n";
							
							
							//cout<<"username is "<<username<<endl;

						}

						else if(strcmp(msg,"fail")==0)
						{
							cout<<"NO SUCH GROUP !!!\n";
						}

						else if(strcmp(msg,"not")==0)
						{
							cout<<"YOU'RE NOT IN GROUP !!!\n";
						}

						else if(strcmp(msg,"owner")==0)
						{
							cout<<"YOU'RE OWNER CANNOT LEAVE !!!\n";
						}


						send(s_fd,&sack,sizeof(sack),0);
						continue;
					}
//--------------------------------------------------------------------------------------------------------
	
					if(strcmp(command,"list_requests")==0)

					{
						cout<<"inside list_requests\n";


						//checks if command parameter are correct
						char* token;
						if((token=strtok(NULL," "))==NULL)
						{
							cout<<"INVALID COMMAND\n";
							continue;
						}
						//username=strtok(NULL," ");
						if(flag==0)
						{
							cout<<"\nPLEASE LOGIN!!!\n";
							continue;
						}

						//send data
						string tosend="";
						tosend=input1+" "+username;
						send(s_fd,(char *)tosend.c_str(),sizeof(tosend),0);
						
						//recieve msg
						memset(msg,'\0',sizeof(msg));
						recv(s_fd,msg,sizeof(msg),0);

						

						if(strcmp(msg,"fail")==0)
						{
							cout<<"NO SUCH GROUP !!!\n";
						}

						else if(strcmp(msg,"not")==0)
						{
							cout<<"NO PENDING REQUEST!!!\n";
						}

						else
						{
							string msg1=msg;
							//cout<<"received msg: "<<msg1<<endl;
							int msg2=stoi(msg1);
							//cout<<"int converted: "<<msg2<<endl;
							send(s_fd,&sack,sizeof(sack),0);

							cout<<"PENDING REQUESTS ARE :"<<endl;
							for(int i=0;i<msg2;i++)
							{
								char user_p[1024]={'\0'};
								recv(s_fd,user_p,sizeof(user_p),0);
								cout<<user_p<<endl;
								send(s_fd,&sack,sizeof(sack),0);


							}
							recv(s_fd,&sack,sizeof(sack),0);

						}

						

						//recv(s_fd,&sack,sizeof(sack),0);
						send(s_fd,&sack,sizeof(sack),0);
						continue;
					}
//----------------------------------------------------------------------------------------------

					if(strcmp(command,"accept_request")==0)

					{
						cout<<"inside accept_request\n";


						//checks if command parameter are correct
						char* token;
						if((token=strtok(NULL," "))==NULL)
						{
							cout<<"INVALID COMMAND\n";
							continue;
						}
						//username=strtok(NULL," ");
						if(flag==0)
						{
							cout<<"\nPLEASE LOGIN!!!\n";
							continue;
						}

						//send data
						string tosend="";
						tosend=input1+" "+username;
						send(s_fd,(char *)tosend.c_str(),sizeof(tosend),0);
						
						//recieve msg
						memset(msg,'\0',sizeof(msg));
						recv(s_fd,msg,sizeof(msg),0);

						

						if(strcmp(msg,"fail")==0)
						{
							cout<<"NO SUCH GROUP !!!\n";
						}

						else if(strcmp(msg,"not_allowed")==0)
						{
							cout<<"PERMISSION DENIED!!!\n";
						}

						else if(strcmp(msg,"not_there")==0)
						{
							cout<<"USER NOT IN LIST!!!\n";
						}


						else if(strcmp(msg,"done")==0)
						{
							cout<<"REQUEST ACCEPTED!!!\n";
						}

						

						//recv(s_fd,&sack,sizeof(sack),0);
						send(s_fd,&sack,sizeof(sack),0);
						continue;
					}
//----------------------------------------------------------------------------------------------
					if(strcmp(command,"list_groups")==0)

					{
						cout<<"inside list_groups\n";


						
						//username=strtok(NULL," ");
						if(flag==0)
						{
							cout<<"\nPLEASE LOGIN!!!\n";
							continue;
						}

						//send data
						//string tosend="";
						//tosend=input1+" "+username;
						send(s_fd,(char *)input1.c_str(),sizeof(input1),0);
						
						//recieve msg
						memset(msg,'\0',sizeof(msg));
						recv(s_fd,msg,sizeof(msg),0);

						

						if(strcmp(msg,"empty")==0)
						{
							cout<<"NO GROUP !!!\n";
						}

						else
						{
							string msg1=msg;
							//cout<<"received msg: "<<msg1<<endl;
							int msg2=stoi(msg1);
							//cout<<"int converted: "<<msg2<<endl;
							send(s_fd,&sack,sizeof(sack),0);

							cout<<"Groups ARE :"<<endl;
							for(int i=0;i<msg2;i++)
							{
								char user_p[1024]={'\0'};
								recv(s_fd,user_p,sizeof(user_p),0);
								cout<<user_p<<endl;
								send(s_fd,&sack,sizeof(sack),0);


							}
							recv(s_fd,&sack,sizeof(sack),0);

						}

						

						//recv(s_fd,&sack,sizeof(sack),0);
						send(s_fd,&sack,sizeof(sack),0);
						continue;
						// else
						// {
						// 	//cout<<"received "<<msg<<endl;
						// 	char *token=strtok(msg," ");
						// 	while(token!=NULL)
						// 	{
						// 		cout<<token<<endl;
						// 		token=strtok(NULL," ");
						// 	}
						// }


						

			
					}
//----------------------------------------------------------------------------------------------
					if(strcmp(command,"upload_file")==0)
					{
						cout<<"inside upload\n";

						//checks if command parameter are correct
						char *filepath=strtok(NULL," ");

						if(filepath==NULL)
						{
							cout<<"INVALID COMMAND\n";
							continue;
						}

						
						if(flag==0)
						{
							cout<<"\nPLEASE LOGIN!!!\n";
							continue;
						}
						//cout<<"file is "<<filepath<<endl;

						//calculate file size and send
						FILE *fop=fopen(filepath,"rb"); 
							if(fop<0)
							{
								perror("unable to open file");
								pthread_exit(NULL);
							}

							int ack;
							//calculate file size
							//cout<<"open file \n";
							rewind(fop);
							fseek(fop,0,SEEK_END); //data after seek_end+0
							int fsize=ftell(fop); //gives file pointer location
							rewind(fop); //sets pointer to beginning of file
							
							//cout<<fsize<<"\n";
							int no_chunks=ceil(((float)fsize/CHUNK_SIZE));
							cout<<"no_chunks"<<no_chunks<<endl;

							for(int i=0;i<no_chunks;i++)
							peer_file[filepath].push_back(i+1);

							for(int i=0;i<no_chunks;i++)
							{
								cout<<peer_file[filepath][i]<<endl;
							}


							//cout<<"after file size "<<endl;
							

							//sending commandd and other datas
							string sendata=input1+" "+username+" "+to_string(fsize);
							
							send(s_fd,(char *)sendata.c_str(),sendata.length(),0);
							memset(msg,'\0',sizeof(msg));
							recv(s_fd,msg,sizeof(msg),0);
							cout<<"msg "<<msg<<endl;

							if(strcmp(msg,"fail")==0)
							{
								cout<<"NO SUCH GROUP!!"<<endl;
								send(s_fd,&ack,sizeof(ack),0);
								continue;	
							}

							else if(strcmp(msg,"not_member")==0)
							{
								cout<<"NOT IN GROUP!!"<<endl;
								send(s_fd,&ack,sizeof(ack),0);
								continue;	
							}


							else if(strcmp(msg,"old")==0)
							{
								//cout<<"enter old"<<endl;
								cout<<"UPLOADED!!"<<endl;
								send(s_fd,&ack,sizeof(ack),0);
								continue;

							}

							else if(strcmp(msg,"new")==0)
							{
								//cout<<"enter new "<<endl;
								//caluclating sha
									string sha=sha256_file(fop,fsize);
									//sending sha
									int shai=0;

									while(shai!=sha.size())
									{
										string send_sha=sha.substr(shai,20);
										send(s_fd,(char *)send_sha.c_str(),send_sha.length(),0);
										//cout<<"sent sha "<<send_sha<<endl;
										//int ack;
										recv(s_fd,&ack,sizeof(ack),0);


										shai+=20;
									}
									char* msg;

									msg=(char *)"end";
									send(s_fd,msg,sizeof(msg),0);
									fclose(fop);
									
									cout<<"UPLOADED FILE!!!";
									recv(s_fd,&ack,sizeof(ack),0);

									continue;
							}


					}					
				
//--------------------------------------------------------------------------------------
					if(strcmp(command,"download_file")==0)
					{
						vector<string>c_ports_r;
						vector<string>c_ip_r;
						cout<<"inside download\n";
						int ack=1;
						//checks if command parameter are correct
						char *gid=strtok(NULL," ");
						char *file1=strtok(NULL," ");
						char *filepath=strtok(NULL," ");

						if(filepath==NULL)
						{
							cout<<"INVALID COMMAND\n";
							continue;
						}

						
						if(flag==0)
						{
							cout<<"\nPLEASE LOGIN!!!\n";
							continue;
						}
						
						//sending commandd and other datas
						string sendata=input1+" "+username;
						
						send(s_fd,(char *)sendata.c_str(),sendata.length(),0);
						memset(msg,'\0',sizeof(msg));
						recv(s_fd,msg,sizeof(msg),0);
						cout<<"msg "<<msg<<endl;
						
						if(strcmp(msg,"fail")==0)
						{
							cout<<"NO SUCH GROUP!!"<<endl;
							send(s_fd,&ack,sizeof(ack),0);
							continue;	
						}

						else if(strcmp(msg,"not_member")==0)
						{
							cout<<"NOT IN GROUP!!"<<endl;
							send(s_fd,&ack,sizeof(ack),0);
							continue;	
						}

						else
						{
							send(s_fd,&ack,sizeof(ack),0);
							int fsize;
							recv(s_fd,&fsize,sizeof(fsize),0);
							cout<<"file size is "<<fsize<<endl;
							int no_chunks=ceil(((float)fsize/CHUNK_SIZE));
					 		//cout<<"no_chunks"<<no_chunks<<endl;
							send(s_fd,&ack,sizeof(ack),0);


							//receive sha
							string sha_coming="";
							char buffer_sha[25]={'\0'};
							int n;

							while(n=recv(s_fd,buffer_sha,sizeof(buffer_sha),0)>0)
							{
								
								if(strcmp(buffer_sha,"end")==0)
								{
									break;
								}

								string sha_20=buffer_sha;
								sha_coming+=buffer_sha;
								send(s_fd,&rack,sizeof(rack),0);
							}
							//cout<<"recieved sha"<<sha_coming<<endl;

							//write null file
							int null_buff_size;
							if(fsize<=2048)
							{
								null_buff_size=fsize;
							}
							else
							{
								null_buff_size=2048;
							}
							char buffer_null[null_buff_size];
							memset(buffer_null,'\0',null_buff_size);



							char buffer[2048];
							//open file and copy it 
							// char filep[]="./files/testc.txt";
							  //cout<<"file in client is "<<filepath<<"\n";
							FILE *fop=fopen(filepath,"wb+"); //rb used for non text files
							if(fop<0)
							{
								perror("unable to open file");
								pthread_exit(NULL);

								
							}

							int n_f=fsize;
							while(n_f>0)
							{
								//cout<<n<<"  n is\n";
								int minus;
								if(n_f<2048)
								{
									null_buff_size=n;
									minus=fwrite(buffer_null,sizeof(char),null_buff_size,fop);
									//cout<<"wrote "<<minus<<"\n";
									memset(buffer_null,'\0',null_buff_size);
								}
								else
								{
								minus=fwrite(buffer_null,sizeof(char),null_buff_size,fop);
								//cout<<"wrote "<<minus<<"\n";
								memset(buffer_null,'\0',null_buff_size);
									}
								// if(n<=2048)
								// {
								// 	fwrite(buffer_null,sizeof(char),null_buff_size,fop);
								// }
								n_f=n_f-minus;

							}
							//fclose(fop);
							//NULL FILE END
							cout<<"size of sha "<<sha_coming.size()<<endl;
							send(s_fd,&rack,sizeof(rack),0);


							//receive client port and ip
							int owner_n;
							recv(s_fd,&owner_n,sizeof(owner_n),0);
							//cout<<"recieved owner_n is"<<owner_n<<endl;
							send(s_fd,&rack,sizeof(rack),0);

							for(int i=0;i<owner_n;i++)
							{
								char receive_s[2048]={'\0'};
	 							recv(s_fd,&receive_s,sizeof(receive_s),0);

	 							if(strcmp(receive_s,"none")==0)
	 							{
	 								cout<<"nothing"<<endl;
	 							}

	 							else 
	 							{

	 								//cout<<i<<" receive_s is "<<receive_s<<endl;
	 								string port_s=strtok(receive_s," ");
	 								string ip_s=strtok(NULL," ");
	 								c_ports_r.push_back(port_s);
	 								c_ip_r.push_back(ip_s);
	 							}
								send(s_fd,&rack,sizeof(rack),0);
							}

							recv(s_fd,&rack,sizeof(rack),0);


							//call for chunk values
							int clients_n=c_ports_r.size();
							vector<vector<int>>chunks_to_take(clients_n);

							for(int i=0;i<c_ports_r.size();i++)
							{
									get_chunks(chunks_to_take,i,c_ports_r[i],c_ip_r[i],file1);

							}

							//print chunk vector
							cout<<"received vector"<<endl;
							for(int i=0;i<clients_n;i++)
							{
								cout<<i<<"---->"<<endl;
								for(int j=0;j<chunks_to_take[i].size();j++)
								{
									cout<<chunks_to_take[i][j]<<endl;
								}
							}

							//hatana h code se only for testing 
							//chunks_to_take[0][0]=2;
							//chunks_to_take[1][0]=1;

							vector<vector<int>>actual_chunks(clients_n);
							piece_selection(actual_chunks,chunks_to_take,no_chunks,clients_n);
							cout<<"after peice "<<endl;
							for(int i=0;i<clients_n;i++)
							{
								cout<<i<<"---->"<<endl;
								for(int j=0;j<actual_chunks[i].size();j++)
								{
									cout<<actual_chunks[i][j]<<endl;
								}
							}

							//hatana h code se only for testing 
						//	chunks_to_take[1][0]=2;
						//	 chunks_to_take[1][1]=1;

							int i=0;
						
							pthread_t client_download[clients_n];

							pthread_mutex_init(&lockem,NULL);
							while(i<clients_n)
							{
								//int chunk_a[100],j;
								//for(j=0;j<)
								threaddata *data_download=new threaddata;
								cout<<"running CLIENT "<<c_ports_r[i]<<"\n";
								int client_p;
								data_download->chunk_d.clear();
								for(int j=0;j<actual_chunks[i].size();j++)
								{
									data_download->chunk_d.push_back(actual_chunks[i][j]);
								}
								client_p=stoi(c_ports_r[i]);
								//threaddata data_download;
								data_download->filefd=fop;
								data_download->file=file1;
								data_download->pport=client_p;
								//data_download->chunk=chunk_to_take[i];
								data_download->chunk_d=actual_chunks[i];
								//cout<<data_download->chunk_d[0]<<endl;
								cout<<"thread data sent "<<endl;
								pthread_create(&client_download[i],NULL,client_download_f,(void *)data_download);
								pthread_join(client_download[i],NULL);

								i++;
							}
							i=0;
							while(i<clients_n){
								pthread_join(client_download[i],NULL);
								i++;
							}
							    pthread_mutex_destroy(&lockem); 
								fclose(fop);

							// for(auto it:c_ports_r)
							// {
							// 	cout<<"a "<<it<<endl;
							// }

							// for(auto it:c_ip_r)
							// {
							// 	cout<<"ip "<<it<<endl;
							// }
							
							send(s_fd,&rack,sizeof(rack),0);
							continue;


						}

					}

						
		
		}	
			close(s_fd);
		
	
}


void *handlerequest_f(void *request_data)
{
	cout<<"entered handle REQUEST\n";
	threaddata *req_data=(threaddata *)request_data;

	 int client_fd=req_data->fd;
	 int ack=1;
	//char buffer[512]={0};

	char msg[1024]={'\0'};
	memset(msg,'\0',1024);
	recv(client_fd,msg,sizeof(msg),0);
	cout<<"msg in handle "<<msg<<endl;

	if(strcmp(msg,"request_chunk")==0)
	{
		cout<<"enter r_c"<<endl;
		send(client_fd,&ack,sizeof(ack),0);
		char fname[1024]={'\0'};
		memset(fname,'\0',1024);
		recv(client_fd,&fname,sizeof(fname),0);
		cout<<"here"<<endl;
		 cout<<"handle request "<<fname<<endl;

		int chunk_v_s=peer_file[fname].size();
		cout<<"sending "<<chunk_v_s<<endl;
		send(client_fd,&chunk_v_s,sizeof(chunk_v_s),0);
		recv(client_fd,&ack,sizeof(ack),0);
		for(int i=0;i<peer_file[fname].size();i++)
		{
			int chunk_f=peer_file[fname][i];
			send(client_fd,&chunk_f,sizeof(chunk_f),0);
			cout<<"sent chunk_f "<<chunk_f<<endl;
			recv(client_fd,&ack,sizeof(ack),0);
		}
		send(client_fd,&ack,sizeof(ack),0);



	}

	else if(strcmp(msg,"request_data")==0)
	{
			send(client_fd,&ack,sizeof(ack),0);
				char filepath[2048]={'\0'};
				//memset(filepath,'\0',2048);
				recv(client_fd,filepath,2048,0);
				
				send(client_fd,&ack,sizeof(ack),0);
				cout<<"received file "<<filepath<<endl;
								
			 // filepath=req_data->file;
			//  cout<<"handle request "<<filepath<<"\n";
			FILE *fop=fopen(filepath,"rb"); //rb used for non text files
			if(fop<0)
			{
				perror("unable to open file");
				pthread_exit(NULL);

				//exit(1);
			}
			
			cout<<"open file \n";
			int bytesent;

			
				int chunk;
				recv(client_fd,&chunk,sizeof(chunk),0);
				cout<<"CHUNK IS "<<chunk<<"\n";
				send(client_fd,&ack,sizeof(ack),0);
				int fsize=512;
				int pos=(chunk-1)*fsize;

				fseek(fop,pos,SEEK_SET);
				cout<<"reading from byte "<<pos<<"\n";
				char buffer[512]={0};
				while(fsize>0 && (bytesent=fread(buffer,sizeof(char),512,fop))>0  )
				{
					cout<<"bufffer sent as "<<buffer<<"\n";
					send(client_fd,buffer,bytesent,0);
					memset(buffer,'\0',512);
					fsize=fsize-bytesent;
				}

				 fclose(fop);

				// recv(client_fd,&ack,sizeof(ack),0);

						//close(client_fd);
				pthread_exit(NULL);



 			 //fclose(fop);
			//receive chunk

			
	}	
	 close(client_fd);
	pthread_exit(NULL);
}

void *server(void *clientarg)
{

	//cout<<"server in peert\n";

	threaddata *sport=(threaddata *)clientarg;
	int sportn=sport->pport;
	int s_fd;

	s_fd=socket(AF_INET,SOCK_STREAM,0);

		if(s_fd<0)
		{
			perror("ERROR IN SOCKET CREATION");
			pthread_exit(NULL);
			//exit(EXIT_FAILURE);
		}

	//cout<<"Server socket created\n";

	int opt=3;
	int setopt=setsockopt(s_fd,SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT ,&opt,sizeof(opt));

	if(setopt<0)
	{
		perror(" SOCKOPT FAILED");
		pthread_exit(NULL);
		//exit(EXIT_FAILURE);
	}

	struct sockaddr_in ip_server,ip_client;
	memset(&ip_server,'0',sizeof(ip_server));

	ip_server.sin_family=AF_INET;
	ip_server.sin_addr.s_addr =inet_addr("127.0.0.1");
	ip_server.sin_port=htons(sportn);

	int bd=bind(s_fd,(struct sockaddr *)&ip_server,sizeof(ip_server));

	if(bd<0)
	{
		perror("BIND FAILED");
		pthread_exit(NULL);
	}



	if(listen(s_fd,10)<0)
	{
		perror("LISTEN FAILED");
		pthread_exit(NULL);
	}

		//perror("LISTEN FAILED");
		//exit(1);
	

	
		cout<<"wait accept\n";
	int ip_client_length,client_fd;
					pthread_t handlerequest[10];
					int i=0;
	//client_fd=accept(s_fd,(struct sockaddr *)&ip_client,(socklen_t*)&ip_client_length);
	while((client_fd=accept(s_fd,(struct sockaddr *)&ip_client,(socklen_t*)&ip_client_length))>0)
	{

			cout<<"enter while\n";

	//cout<<"inside while1";
				//threaddata request_data;
				threaddata *request_data=(threaddata *)malloc(sizeof(threaddata));

				request_data->fd=client_fd;
				//request_data->file=sport->file;


				pthread_create(&handlerequest[i],NULL,handlerequest_f,(void *)request_data);
				pthread_detach(handlerequest[i]);
				i++;
				


	}
//cout<<"at end";
	

	
	close(s_fd);
	

	
}

int main()
{

	char filepath[1000],filepath1[1000];
	threaddata *sport=(threaddata *)malloc(sizeof(threaddata));
	threaddata *cport=(threaddata *)malloc(sizeof(threaddata));
	cout<<"ENTER tracker PORT \n";
	cin>>cport->tport;

	cout<<"ENTER peer PORT \n";
	 cin>>sport->pport;
	 cport->pport=sport->pport;
	pthread_t threadserver,threadclient;
	pthread_create(&threadserver,NULL,server,(void *)sport);
	pthread_create(&threadclient,NULL,client,(void *)cport);
	pthread_join(threadserver,NULL);
	pthread_join(threadclient,NULL);

}