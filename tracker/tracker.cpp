
#include<iostream> //basic inp output
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
#include<bits/stdc++.h>
#include <netinet/in.h>

using namespace std;
typedef struct user_info user_info;
struct user_create
{
	//string user_id;
	string password;
	string ip;
	string port;
	int uflag=0;

};
typedef struct user_create user_create;

unordered_map<string,struct user_create*>user_id_map;

struct group_struct
{
	vector<pair<string,int>>group_files;
	vector<string>group_user;
	string owner;
	vector<string>pending_req;
};

typedef struct group_struct group_struct;

unordered_map<string,group_struct*>group_map;

struct file_struct
{
	string f_gid;
	int fsize;
	string my_sha;
	vector<string>file_owner;
	string file_in_gid;
};

typedef struct file_struct file_struct;

unordered_map<string,file_struct*>file_map;
struct DataforThread
{
	int fd;
	int port;
	char *file;
	int chunk;
	FILE* filefd;
	string cport;
	string cip;
};
typedef struct DataforThread threaddata;

void sending_vector(vector<string>&send_v,int client_fd)
{
	int ack=1;
	//char msg[1024]={'\0'};
	string msg;
	int r_size=send_v.size();
	msg=to_string(r_size);
	cout<<"sending size "<<msg<<endl;
	send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
	recv(client_fd,&ack,sizeof(ack),0);

	for(int i=0;i<send_v.size();i++)
	{
		string to_send=send_v[i];
		cout<<"sending "<<to_send<<endl;
		send(client_fd,(char *)to_send.c_str(),sizeof(to_send),0);
		recv(client_fd,&ack,sizeof(ack),0);
	}
	send(client_fd,&ack,sizeof(ack),0);
}


void *handlerequest_f(void *request_data)
{
	cout<<"entered \n";

	threaddata *req_data=(threaddata *)request_data;

	 int client_fd=req_data->fd;
		char command[1024];
		
	 	 vector<user_info*>user_table;

	 	while(1)
	 	{
	 		int ack=1;
	 		memset(command,'\0',1024);
	 	int bytes=recv(client_fd,&command,sizeof(command),0);
	 	string received=string(command,0,bytes);
	 	char* command1=strtok(command," ");
	 	if(command1==NULL)
	 	{
	 		pthread_exit(NULL);
	 	}
//----------------------------------------------------------------------------------------------
	 			if(strcmp(command1,"create_user")==0)
	 			{
	 				string username=strtok(NULL," ");
					string password1=strtok(NULL," ");
					//cout<<user_id_map[username]<<endl
					if(user_id_map.find(username)!=user_id_map.end())
					{
						char msg[1024]="USER_ALREADY";
						send(client_fd,msg,sizeof(msg),0);
						recv(client_fd,&ack,sizeof(ack),0);
						continue;
					}

				 	 user_create *user_create1=new user_create;
				 	 user_create1->password=password1;
				 	 user_id_map[username]=user_create1;
				 	 cout<<"received password as : "<<user_id_map[username]->password<<endl;

				 		char msg[1024]="success";
				 		send(client_fd,msg,sizeof(msg),0);
				 		recv(client_fd,&ack,sizeof(ack),0);
						continue;

							
	 			}
//----------------------------------------------------------------------------------------------
	 			if(strcmp(command1,"login")==0)
			 	{
			 		string username=strtok(NULL," ");
					string password1=strtok(NULL," ");
					string msg;
					if(user_id_map.find(username)!=user_id_map.end() && user_id_map[username]->password==password1)
					{
						string port_f=req_data->cport;
						string ip_f=req_data->cip;
						
						user_id_map[username]->port=port_f;
						user_id_map[username]->ip=ip_f;
						//cout<<"port_f is "<<user_id_map[username]->port<<endl;
						//cout<<"ip_f is "<<user_id_map[username]->ip<<endl;


						user_id_map[username]->uflag=1;

						 msg="correct";
						
						

					}

					else
					{
						msg="incorrect";
					}

					send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
					recv(client_fd,&ack,sizeof(ack),0);
					continue;
					//continue;
			 	}
//--------------------------------------------------------------------------------------------------------
				if(strcmp(command1,"logout")==0)
			 	{
			 		cout<<"enter logout\n";

			 		//string gid=strtok(NULL," ");
					string owner=strtok(NULL," ");
					string msg;
					user_id_map[owner]->uflag=0;
					
						msg="success";
					send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
					recv(client_fd,&ack,sizeof(ack),0);
					continue;
				}

//----------------------------------------------------------------------------------------------
			 	if(strcmp(command1,"create_group")==0)
			 	{
			 		cout<<"enter create_group\n";

			 		string gid=strtok(NULL," ");
					string owner=strtok(NULL," ");
					string msg;

					if(group_map.find(gid)!=group_map.end())
					{
						msg="used";
						//cout<<"checked\n";
					}

					//

					else
					{
						//cout<<"inside else\n";
						group_struct *group_struct1=new group_struct;
						group_struct1->owner=owner;
						group_struct1->group_user.push_back(owner);
						group_map[gid]=group_struct1;
						//cout<<"assigned\n";

						//check 
						cout<<"owner : "<<group_map[gid]->owner<<endl;
						for(auto it=group_map[gid]->group_user.begin();it!=group_map[gid]->group_user.end();it++)
						{
							cout<<"group_users are "<<*it<<endl;
							
						}

						msg="success";
						

					}

					send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
					recv(client_fd,&ack,sizeof(ack),0);
					continue;



					

				}
//----------------------------------------------------------------------------------------------
				
				if(strcmp(command1,"join_group")==0)
			 	{
			 		cout<<"enter join_group\n";

			 		string gid=strtok(NULL," ");
					string user=strtok(NULL," ");
					string msg;

					

					if(group_map.find(gid)==group_map.end())
					{
						msg="fail";
						//cout<<"checked\n";
					}

					
					

					else
					{
						

						int already=0;
						for(auto it=group_map[gid]->group_user.begin();it!=group_map[gid]->group_user.end();it++)
						{
							cout<<"group_users are "<<*it<<endl;
							if(*it==user)
								already=1;
							
						}


					//
						if(already==1)
						{
						msg="already";
						}

						else
						{

								group_map[gid]->pending_req.push_back(user);

								//check 
								cout<<"owner : "<<group_map[gid]->owner<<endl;
								for(auto it=group_map[gid]->group_user.begin();it!=group_map[gid]->group_user.end();it++)
								{
									cout<<"group_users are "<<*it<<endl;
									
								}

								msg="success";
						}
						

					}

					send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
					recv(client_fd,&ack,sizeof(ack),0);
					continue;
				}
//----------------------------------------------------------------------------------------------				}
				//--------to complete abhi file entry wala-------
				if(strcmp(command1,"leave_group")==0)
			 	{
			 		cout<<"enter leave_group\n";

			 		string gid=strtok(NULL," ");
					string user=strtok(NULL," ");
					string msg;

					if(group_map.find(gid)==group_map.end())
					{
						msg="fail";
						//cout<<"checked\n";
					}

					else if(user==group_map[gid]->owner)
					{
							msg="owner";
					}
					

					else
					{
						int already=0;
						auto it=group_map[gid]->group_user.begin();
						for(it=group_map[gid]->group_user.begin();it!=group_map[gid]->group_user.end();it++)
						{
							//cout<<"group_users are "<<*it<<endl;
							if(*it==user)
							{
								already=1;
								break;
							}
							
						}


					//
						if(already==0)
						{
						msg="not";
						}

						else
						{

								group_map[gid]->group_user.erase(it);

								//check 
								cout<<"owner : "<<group_map[gid]->owner<<endl;
								for(auto it=group_map[gid]->group_user.begin();it!=group_map[gid]->group_user.end();it++)
								{
									cout<<"group_users are "<<*it<<endl;
									
								}

								msg="success";
						}
						

					}

					send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
					recv(client_fd,&ack,sizeof(ack),0);
					continue;
				}
//----------------------------------------------------------------------------------------------	
				if(strcmp(command1,"list_requests")==0)
			 	{
			 		cout<<"enter list_requests\n";

			 		string gid=strtok(NULL," ");
					string user=strtok(NULL," ");
					string msg;

					if(group_map.find(gid)==group_map.end())
					{
						msg="fail";
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);

						//cout<<"checked\n";
					}

					else if(group_map[gid]->pending_req.size()==0)
					{
						msg="not";
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);


					}
					

					else
					{
						sending_vector(group_map[gid]->pending_req,client_fd);
					
					}

					recv(client_fd,&ack,sizeof(ack),0);
					continue;
				}
//----------------------------------------------------------------------------------------------	

				if(strcmp(command1,"accept_request")==0)
			 	{
			 		cout<<"enter accept_request\n";

			 		string gid=strtok(NULL," ");
					string user=strtok(NULL," ");
					string owner1=strtok(NULL," ");
					string msg;

					if(group_map.find(gid)==group_map.end())
					{
						msg="fail";
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);

						//cout<<"checked\n";
					}

					else if(group_map[gid]->owner!=owner1)
					{
						msg="not_allowed";
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);


					}
					

					else
					{
						auto it=find(group_map[gid]->pending_req.begin(),group_map[gid]->pending_req.end(),user);
						if(it==group_map[gid]->pending_req.end())
						{
							msg="not_there";
							send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
						}

						else
						{
							group_map[gid]->pending_req.erase(it);
							group_map[gid]->group_user.push_back(user);
							msg="done";
							send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
						}

					}

					recv(client_fd,&ack,sizeof(ack),0);
					continue;
				}
//----------------------------------------------------------------------------------------------				 	
				if(strcmp(command1,"list_groups")==0)
			 	{
			 		cout<<"enter list_groups\n";

			 		
					string msg1="";
					if(group_map.size()==0)
					{
						msg1="empty";
						send(client_fd,(char *)msg1.c_str(),sizeof(msg1),0);
					}

					else
					{
						//msg1="done";
							string msg="";
							int r_size=group_map.size();
							msg=to_string(r_size);
							cout<<"sending size "<<msg<<endl;
							send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
							recv(client_fd,&ack,sizeof(ack),0);

							for(auto i : group_map)
							{
								string to_send=i.first;
								cout<<"sending "<<to_send<<endl;
								send(client_fd,(char *)to_send.c_str(),sizeof(to_send),0);
								recv(client_fd,&ack,sizeof(ack),0);
							}
							send(client_fd,&ack,sizeof(ack),0);
					}

					//send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
					recv(client_fd,&ack,sizeof(ack),0);
					continue;
				}
//-------------------------------------------------------------------------------------------------------------------------
			 	if(strcmp(command1,"upload_file")==0)
			 	{
			 	

			 		string msg;
			 		cout<<"enter upload_file\n";
			 		string filename1=strtok(NULL," ");
			 		string gid1=strtok(NULL," ");
			 		string user=strtok(NULL," ");
					string fsize1=strtok(NULL," ");
					
					cout<<"filename 1 "<<filename1<<endl;
					cout<<"gid1 " <<gid1<<endl;
					cout<<"user "<<user<<endl;
					cout<<"fsize1 "<<fsize1<<endl; 

					string key=filename1+gid1;
					//if user allowed to upload
										
					if(group_map.find(gid1)==group_map.end())
					{
						msg="fail";
						//auto it=find(group_map[gid]->pending_req.begin(),group_map[gid]->pending_req.end(),user);
						
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
						recv(client_fd,&ack,sizeof(ack),0);
						//cout<<"checked\n";
					}

					else if((find(group_map[gid1]->group_user.begin(),group_map[gid1]->group_user.end(),user))==group_map[gid1]->group_user.end())
					{
						msg="not_member";
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
						recv(client_fd,&ack,sizeof(ack),0);
						//cout<<"checked\n";
					}

					//if file already there in list

					else if(file_map.find(key)!=file_map.end())
					{
						cout<<"enter old trac"<<endl;
						msg="old";
						file_map[key]->file_owner.push_back(user);

						//check sharable 
						auto check_p=make_pair(filename1,0);
						auto it=find(group_map[gid1]->group_files.begin(),group_map[gid1]->group_files.end(),check_p);

						if(it!=group_map[gid1]->group_files.end())
						{
							it->second=1;
							cout<<"file is "<<it->first<<endl;
						}

						else
							cout<<"already sharable "<<endl;
						

							
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
						recv(client_fd,&ack,sizeof(ack),0);

						//cout<<"checked\n";
					}


					else
					{
					//sending ack
						cout<<"enter new "<<endl;
						int rack=1;
						msg="new";
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);


							//receive sha
							string sha_coming="";
							char buffer_sha[25]={'\0'};
							int n;

							while(n=recv(client_fd,buffer_sha,sizeof(buffer_sha),0)>0)
							{
								//cout<<"enter while"<<endl;
								if(strcmp(buffer_sha,"end")==0)
								{
									//cout<<"sha ended "<<endl;
									//send(client_fd,&rack,sizeof(rack),0);

									break;
								}

								string sha_20=buffer_sha;
								sha_coming+=buffer_sha;
							//	user_data->sha=sha_coming;
								//cout<<"sha received "<<sha_20<<endl;

								send(client_fd,&rack,sizeof(rack),0);



							}

							//insert in group_files
							group_map[gid1]->group_files.push_back(make_pair(filename1,1));

							string ans=group_map[gid1]->group_files[0].first;
							cout<<"inserted "<<ans<<endl;
							
							//when file is not present in the struct

							file_struct *file_struct1=new file_struct;
							int fsize2=stoi(fsize1);
						 	file_struct1->fsize=fsize2;
						 	cout<<"gid is "<<gid1<<endl;
						 	file_struct1->f_gid=gid1;
						 	file_struct1->file_owner.push_back(user);
						 	file_struct1->file_in_gid=filename1;
						 	file_struct1->my_sha=sha_coming;
						 	//file_struct1->file_sha=sha_coming;
						 	cout<<"checking values "<<endl;

						 	
						 	file_map[key]=file_struct1;
						 	cout<<"fsize :"<<file_map[key]->fsize<<endl;
						 	cout<<"gid :"<<file_map[key]->f_gid<<endl;
						 	cout<<"file_owner: "<<file_map[key]->file_owner.size()<<endl;
						 	cout<<"file is : "<<file_map[key]->file_in_gid<<endl;
						 	cout<<"sha is : "<<file_map[key]->my_sha<<endl;
						 	send(client_fd,&rack,sizeof(rack),0);
					}
					
					continue;
					//cout<<"size of sha "<<sha.size()<<endl;
				}
//------------------------------------------------------------------------------------------------------------------
			 	if(strcmp(command1,"download_file")==0)
			 	{
			 	

			 		string msg;
			 		cout<<"enter download_file\n";
			 		string gid1=strtok(NULL," ");
			 		string filename1=strtok(NULL," ");
			 		string filename2=strtok(NULL," ");
			 		string user=strtok(NULL," ");
					
					
					cout<<"filename 1 "<<filename1<<endl;
					cout<<"gid1 " <<gid1<<endl;
					cout<<"user "<<user<<endl;
					cout<<"f2 "<<filename2<<endl; 

					string key=filename1+gid1;
					//if user allowed to upload
										
					if(group_map.find(gid1)==group_map.end())
					{
						msg="fail";
						//auto it=find(group_map[gid]->pending_req.begin(),group_map[gid]->pending_req.end(),user);
						
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
						recv(client_fd,&ack,sizeof(ack),0);
						continue;
						//cout<<"checked\n";
					}

					else if((find(group_map[gid1]->group_user.begin(),group_map[gid1]->group_user.end(),user))==group_map[gid1]->group_user.end())
					{
						msg="not_member";
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
						recv(client_fd,&ack,sizeof(ack),0);
						continue;
						//cout<<"checked\n";
					}

					//if file already there in list


					else
					{
						//sending msg
						cout<<"enter sending "<<endl;
						int rack=1;
						msg="new";
						send(client_fd,(char *)msg.c_str(),sizeof(msg),0);
						recv(client_fd,&ack,sizeof(ack),0);

						//send fsize

						int to_send_size=file_map[key]->fsize;
						cout<<"sent size "<<to_send_size<<endl;
						send(client_fd,&to_send_size,sizeof(to_send_size),0);
						recv(client_fd,&ack,sizeof(ack),0);
						
						//send sha
						string sha=file_map[key]->my_sha;
						cout<<"sent sha "<<sha<<endl;

						int shai=0;

						while(shai!=sha.size())
						{
							string send_sha=sha.substr(shai,20);
							send(client_fd,(char *)send_sha.c_str(),send_sha.length(),0);
							
							recv(client_fd,&ack,sizeof(ack),0);


							shai+=20;
						}
						char* msg;

						msg=(char *)"end";
						send(client_fd,msg,sizeof(msg),0);

						recv(client_fd,&ack,sizeof(ack),0);

						//send client port and ip
						int owner_n=file_map[key]->file_owner.size();
						cout<<owner_n<<endl;
						send(client_fd,&owner_n,sizeof(owner_n),0);
						recv(client_fd,&ack,sizeof(ack),0);

						for(int i=0;i<owner_n;i++)
						{
							string check=file_map[key]->file_owner[i];
							cout<<"check owner "<<check<<endl;
							if(user_id_map[check]->uflag==1)
							{
								string send_s="";
								send_s+=user_id_map[check]->port+" "+user_id_map[check]->ip;
								cout<<"send s "<<send_s<<endl;
								send(client_fd,(char *)send_s.c_str(),sizeof(send_s),0);

							}

							else if(user_id_map[check]->uflag==0)
							{
								string send_s="none";
								cout<<"send s "<<send_s<<endl;
								send(client_fd,(char *)send_s.c_str(),sizeof(send_s),0);


							}

							recv(client_fd,&rack,sizeof(rack),0);
						}



						send(client_fd,&ack,sizeof(ack),0);

						//wait for client to cinnect others
						recv(client_fd,&rack,sizeof(rack),0);

						continue;
							
					}
					
				}
//---------------------------------------------------------------------------------------------------------------------------
		}
		pthread_exit(NULL);
	
}

void *server(void *clientarg)
{

	//cout<<"server in peert\n";

	threaddata *sport=(threaddata *)clientarg;
	int sportn=sport->port;
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
	ip_server.sin_addr.s_addr =INADDR_ANY;
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
			int ip_client_length=sizeof(ip_server);
			int client_fd;
					pthread_t handlerequest[10];
					int i=0;
	//client_fd=accept(s_fd,(struct sockaddr *)&ip_client,(socklen_t*)&ip_client_length);
	while((client_fd=accept(s_fd,(struct sockaddr *)&ip_server,(socklen_t*)&ip_client_length))>0)
	{

			cout<<"enter while\n";

				threaddata *request_data=(threaddata *)malloc(sizeof(threaddata));

				request_data->fd=client_fd;
				request_data->file=sport->file;
				string cport1=to_string(ntohs(ip_server.sin_port));
				 request_data->cport=cport1;
				char *ip_c;
				ip_c=new char[INET_ADDRSTRLEN];
				inet_ntop(AF_INET,&(ip_server.sin_addr),ip_c,INET_ADDRSTRLEN);
				string ip_c1=ip_c;
				request_data->cip=ip_c1;
				pthread_create(&handlerequest[i],NULL,handlerequest_f,(void *)request_data);
				pthread_detach(handlerequest[i]);
				i++;
				
	}
	close(s_fd);
	
}
int main()
{

	char filepath[1000];
	threaddata sport;
	
	cout<<"ENTER server PORT \n";
	 cin>>sport.port;
	pthread_t threadserver;
	pthread_create(&threadserver,NULL,server,(void *)&sport);
	
	pthread_join(threadserver,NULL);
	

}