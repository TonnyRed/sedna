/*
 * File:  listener.cpp
 * Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#include "common/sedna.h"

#include "gov/listener.h"
#include "common/errdbg/d_printf.h"
#include "gov/gov_globals.h"
#include "common/sp.h"
#include "common/u/usem.h"
#include "common/u/uutils.h"
#include "gov/hb_funcs.h"

using namespace std;

static USOCKET sockfd;

static clClient clClients[U_SSET_SIZE];
static U_SSET allset, rset; 	
static int maxfd, maxi = -1, nres;

static int clRegisterClient(USOCKET sock, clProcess_fun fun)
{
	for (int i = 0; i < U_SSET_SIZE; i++)
		if (clClients[i].sock == U_INVALID_SOCKET)
		{
			clClients[i].sock = sock;
			clClients[i].clProcess = fun;

			U_SSET_SET(sock, &allset);

			if (maxfd < sock) maxfd = sock;
			if (maxi < i) maxi = i;
       			
			return i;
		}

	return -1; // too many clients
}

static void clDiscardClient(int i)
{
	U_ASSERT(i >= 0 && i <= maxi);

	U_SSET_CLR(clClients[i].sock, &allset);
	clClients[i].sock = U_INVALID_SOCKET;
	clClients[i].clProcess = NULL;	
}

int client_listener(gov_config_struct* cfg, bool background_off_from_background_on)
{   
   msg_struct msg;

   USOCKET socknew, hbsock = U_INVALID_SOCKET;

   sockfd = usocket(AF_INET, SOCK_STREAM, 0, __sys_call_error);
   if (sockfd == U_INVALID_SOCKET) throw SYSTEM_EXCEPTION ("Can't init socket");

   if (uNotInheritDescriptor(UHANDLE(sockfd), __sys_call_error) != 0) throw USER_EXCEPTION(SE4080);

   if (ubind_tcp(sockfd, cfg->gov_vars.lstnr_port_number, __sys_call_error) == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION("Can't bind socket");

   if (ulisten(sockfd, 100, __sys_call_error) == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION("Can't set socket to a listening mode");

   elog(EL_LOG, ("GOVERNOR is ready"));

   if (!background_off_from_background_on)
   {	   
     d_printf1("OK\n");
     fprintf(res_os, "GOVERNOR has been started successfully\n");
     fflush(res_os);
   }

   ///////// NOTIFY THAT SERVER IS READY //////////////////////////////////
   USemaphore started_sem;
   if (0 == USemaphoreOpen(&started_sem, CHARISMA_GOVERNOR_IS_READY, __sys_call_error))
   {
       USemaphoreUp(started_sem, __sys_call_error);
       USemaphoreClose(started_sem, __sys_call_error);
   }
   ///////// NOTIFY THAT SERVER IS READY //////////////////////////////////

   U_SSET_ZERO(&allset);
   U_SSET_SET(sockfd, &allset);
   maxfd = sockfd;

   for (int i = 0; i < U_SSET_SIZE; i++)
   {
   		clClients[i].sock = U_INVALID_SOCKET;
   		clClients[i].clProcess = NULL;
   }   			

   int stop_serv = 0;
   int stop_db = 0;
   int res = 0, res2 = 0;
   int socket_optval = 1, socket_optsize = sizeof(int);
   for(;;)
   {
       rset = allset;

       res = uselect_read_arr(&rset, maxfd, NULL, __sys_call_error);
	   if (res == U_SOCKET_ERROR) 
	   		throw USER_EXCEPTION2(SE3007,usocket_error_translator());
       
       nres = res;

       if (U_SSET_ISSET(sockfd, &rset)) //accept a call from a client
       {
	       socknew = uaccept(sockfd, __sys_call_error);
	       if (socknew == U_INVALID_SOCKET)
	       {
   	    	  d_printf1("Can't accept client's connection\n");
    	      continue;
	       }
       
    	   if (usetsockopt(socknew, IPPROTO_TCP, TCP_NODELAY, (char*)&socket_optval, socket_optsize, __sys_call_error) == U_SOCKET_ERROR)
       	   {
          		d_printf1("Can't accept client's connection: couldn't set socket option\n");
          		ushutdown_close_socket(socknew, __sys_call_error);
          		continue;
       	   }

	       gov_table->erase_all_closed_pids();
    	   gov_table->put_all_free_sids_in_ids_table();
 	       stop_serv = gov_table->check_stop_gov();
    	   if (stop_serv == 0)
        	  stop_db = gov_table->check_stop_databases();

	       //!!!! process msg from client !!!!
    	   res = sp_recv_msg(socknew, &msg);

      	   if (stop_serv == 0)
           		if (res == -1)//socket error 
          		{
             		d_printf2("Connection with client lost: %s\n", usocket_error_translator());
             		continue;
          		}
          		else if (res == -2)
          		{
             		d_printf1("Too large msg recieved\n");
             		continue;
           		}

       	   if (stop_serv == 1 )
       	   {
          		if (msg.instruction != STOP)
          		{
             		sp_error_message_handler(socknew, 100, SE4608, "Transaction is rolled back because server is stopping");
             		if (res2 == U_SOCKET_ERROR) d_printf1("Can't send msg to client that server is stopped\n");
          		}

          		//close session
          		ushutdown_close_socket(socknew, __sys_call_error);
          		break;
       	   }

       	   if (stop_db == 1)
       	   {
          		ushutdown_close_socket(socknew, __sys_call_error);
          		continue;
       	   }
		   
	       switch (msg.instruction)
    	   {
        	       ////////////////////////////////////
          		case CREATE_NEW_SESSION:
          		{
              		CreateNewSessionProcess(socknew, background_off_from_background_on);
              		break;
          		}

               	  ///////////////////////////////////

		        case REGISTER_NEW_SESSION:
          		{

              		sess_registering(socknew, msg.body);
              		break;
          		}
 
               		////////////////////////////////////
  
          		case REGISTER_DB:
          		{
              		sm_registering(socknew, msg.body);
              		break;
          		}

               		////////////////////////////////////

          		case RUNTIME_CONFIG:
          		{
             		send_runtime_config(socknew);
             		break;
          		}

               		////////////////////////////////////

          		case  IS_RUN_SM:
          		{
             		check_sm_run(socknew, msg.body);
             		break;
          		}


               		////////////////////////////////////

               	case HOTBACKUP_START:
               	{
        			if (clRegisterClient(socknew, hbProcessMessage) == -1)
			       	{
          				ushutdown_close_socket(socknew, __sys_call_error);
          				continue;
       	   			}

        			if (uNotInheritDescriptor(UHANDLE(socknew), __sys_call_error) != 0) throw USER_EXCEPTION(SE4080);
          			
          			hbNewClient(socknew);

          			break;
          		}

               		////////////////////////////////////
          		
          		default:
          		{
             		d_printf1("unknown message from client\n");
             		ushutdown_close_socket(socknew, __sys_call_error);
             		break;
          		}
           }

           if (--nres <= 0) continue;
       }

       for (int i = 0; i <= maxi; i++)
       {
       		if (clClients[i].sock == U_INVALID_SOCKET) continue;

       		if (U_SSET_ISSET(clClients[i].sock, &rset))
       		{
       			if (clClients[i].clProcess(clClients[i].sock) != 0)
					clDiscardClient(i);

       			if (--nres <= 0)
       				break;
       		}
       }
       		
   }//end of for

   if (uclose_socket(sockfd, __sys_call_error) == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION("Can't close listening socket");

   return 0;
}

void CreateNewSessionProcess(USOCKET socknew, bool background_off_from_background_on)
{

try{
    UFlag window_mode;
    UPID pid;
    UPHANDLE proc_h;


    //check number of sessions
    if (gov_table->get_total_session_procs_num() > 2*MAX_SESSIONS_NUMBER)
       throw USER_EXCEPTION(SE3046);

#ifdef _WIN32
    USOCKET DuplicateSock = U_INVALID_SOCKET;
    //DuplicateHandle is used only for WinSockets 
    //(as create process doesn't inherit socket handles correctly)
    // Duplicate the socket socknew to create an inheritable copy.
    if (!DuplicateHandle(GetCurrentProcess(),
                         (HANDLE)socknew,
                         GetCurrentProcess(),
                         (HANDLE*)&DuplicateSock,
                         0,
                         TRUE, // Inheritable
                         DUPLICATE_SAME_ACCESS))
    {
       d_printf2("dup error %d\n", GetLastError());
       throw SYSTEM_EXCEPTION("Can't duplicate socket handle");
    }

    // Sets SOCKET HANDLE to an evironment variable
    uSetEnvironmentVariable(CONNECTION_SOCKET_HANDLE,int2string((int)DuplicateSock).c_str(), __sys_call_error);
#else           // no need to duplicate SOCKET HANDLE in Unix
    uSetEnvironmentVariable(CONNECTION_SOCKET_HANDLE,int2string((int)socknew).c_str(), __sys_call_error);
#endif

    uSetEnvironmentVariable(SEDNA_SERVER_MODE, "1", __sys_call_error);

    char buf2[1024];
    uSetEnvironmentVariable(SEDNA_OS_PRIMITIVES_ID_MIN_BOUND, u_itoa(gov_table->get_config_struct()->gov_vars.os_primitives_id_min_bound, buf2, 10), __sys_call_error);    

    // create security attributes for the new process
    USECURITY_ATTRIBUTES *sa;	
    if(0 != uCreateSA(&sa, 
                      U_SEDNA_DEFAULT_ACCESS_PERMISSIONS_MASK, 
                      0,                  // new process will not inherit handle returned by CreateProcess
                      __sys_call_error)) 
    throw USER_EXCEPTION(SE3060);

    // Spawn the child process.
    // Socket HANDLE are passed throught an environment variable
                 	 
    char buf[U_MAX_PATH + 10];
    string con_path_str = uGetImageProcPath(buf, __sys_call_error) + string("/") + SESSION_EXE;
    strcpy(buf, con_path_str.c_str());
 
    if (background_off_from_background_on)
       window_mode = U_DETACHED_PROCESS; //process has no window for output
    else
       window_mode = 0;           //process is created without flags
                
    if (0 != uCreateProcess(buf,
                            true, // inherit handles
                            NULL,
                            window_mode,
                            &proc_h,
                            NULL,
                            &pid,
                            NULL,
                            sa, 
                            __sys_call_error
                           ))
    {
#ifdef _WIN32                  
       d_printf2("create process failed %d\n", GetLastError());
#else           
       d_printf1("create process failed\n");
#endif
       throw USER_EXCEPTION2(SE4413, "Try to reconnect later");
    }

   // release security attributes
   if(uReleaseSA(sa, __sys_call_error)!=0) throw USER_EXCEPTION(SE3063);

   uclose_socket(socknew, __sys_call_error);
#ifdef _WIN32
   uclose_socket(DuplicateSock, __sys_call_error);
#endif

   gov_table->add_pid(pid, proc_h);


   } catch (SednaUserException &e) {
       fprintf(stderr, "%s\n", e.what());
       sp_error_message_handler(socknew, 100, e.get_code(), e.what());
   } catch (SednaException &e) {
       sp_error_message_handler(socknew, 100, 0, "System error");
       sedna_soft_fault(e, EL_GOV);
   } catch (ANY_SE_EXCEPTION) {
       sp_error_message_handler(socknew, 100, 0, "System error");
       sedna_soft_fault(EL_GOV);
   }


   return;
}


int sess_registering(USOCKET s, char* msg_buf)
{
	msg_struct reg_msg;
    UPID sess_pid;
	char db_name[SE_MAX_DB_NAME_LENGTH + 1];

   	__int32 length;
    int res, res2;
    UPHANDLE proc_h;
    bool is_child_process;
   	net_int2int(&length, msg_buf+1);
   	if( length > SE_MAX_DB_NAME_LENGTH ) throw USER_EXCEPTION(SE3015);
   	
    memcpy(db_name, msg_buf+5, length);
    db_name[length] = '\0';
  	d_printf2("Listener: register trn with db_name: %s\n",db_name);
  	char ptr[4];
  	memcpy(ptr, msg_buf+5+strlen(db_name), 4);
            	
//    sess_pid = ntohl(*(__int32*)ptr);
    sess_pid = *(__int32*)ptr;

  	d_printf2("Listener: register trn with pid: %d\n",sess_pid);

    session_id s_id;
	string db_name_str = string(db_name);

    if ((is_child_process = gov_table->find_pid(sess_pid, proc_h)) == true)
    {//session run by Governor       
       res = gov_table->insert_session(sess_pid, &proc_h, db_name_str, true, s_id);                                
    }
    else
    {//session run from command line
       res = gov_table->insert_session(sess_pid, NULL, db_name_str, false, s_id);                                
    }

    //d_printf2("Governor returned sid=%d\n", s_id);

    if ( res == 0 )
    {
    	reg_msg.instruction = 161;
		reg_msg.length = 4;
	    __int32 tmp = s_id;
	    memcpy(reg_msg.body,(char*)&(tmp),4);
	    if(sp_send_msg(s,&reg_msg)!=0){
               throw SYSTEM_EXCEPTION(string(usocket_error_translator()).c_str());
	    }
        gov_table->remove_pid(sess_pid);
    }
    else if (res == -1)
    {
        reg_msg.instruction = 171;
		reg_msg.length = 0;
		res2 = sp_send_msg(s, &reg_msg);
        if (res2 == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION(string(usocket_error_translator()).c_str());
    }
    else if (res == -2)
    {
        d_printf1("currently there are maximum number of session in the system\n");
		reg_msg.instruction = 172;
		reg_msg.length = 0;
        res2 = sp_send_msg(s, &reg_msg);
        if (res2 == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION(string(usocket_error_translator()).c_str());
    }
    else if (res == -3)
    {
	    d_printf1("internal server error\n");
   	    reg_msg.instruction = 173;
		reg_msg.length = 0;
		res2 = sp_send_msg(s, &reg_msg);
        if (res2 == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION(string(usocket_error_translator()).c_str());
    }
    else
    {
        throw SYSTEM_EXCEPTION("Governor failed while registering a new session");
	}

    if (res != 0)
    {
       gov_table->wait_remove_pid(sess_pid, is_child_process);
    }
    
    ushutdown_close_socket(s, __sys_call_error);
	return 0;
	
}

int sm_registering(USOCKET s, char* msg_buf)
{
	msg_struct reg_msg;
    UPID sm_pid;
	char db_name[SE_MAX_DB_NAME_LENGTH + 1];
   	__int32 length;
   	int res, res2;
   	
   	net_int2int(&length, msg_buf+1);
   	d_printf2("sm_registering: %d\n", length);
   	if( length > SE_MAX_DB_NAME_LENGTH ) throw USER_EXCEPTION(SE3015);

    memcpy(db_name, msg_buf+5, length);
    db_name[length] = '\0';
            	
  	d_printf2("Listener: register SM with db_name: %s\n",db_name);
  	char ptr[4];
  	memcpy(ptr, msg_buf+5+strlen(db_name), 4);
    sm_pid = ntohl(*(__int32*)ptr);
                
  	d_printf2("Listener: register SM with pid: %d\n",sm_pid);

///////////////Trying to Register in SHARED MEMORY//////////////////////
	string db_name_str = string(db_name);
    res = gov_table->insert_database(sm_pid, db_name_str);

    if (res == 0)
    {
        reg_msg.instruction = 181;
		reg_msg.length = 0;
		res2 = sp_send_msg(s, &reg_msg);
        if (res2 == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION(string(usocket_error_translator()).c_str());
    }
    else if (res == -2)
    {
        reg_msg.instruction = 182;
		reg_msg.length = 0;
		res2 = sp_send_msg(s, &reg_msg);
        if (res2 == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION(string(usocket_error_translator()).c_str());
    }
    else if (res == -3)
    {
        reg_msg.instruction = 182;
		reg_msg.length = 0;
		res2 = sp_send_msg(s, &reg_msg);
        if (res2 == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION(string(usocket_error_translator()).c_str());
    }
    else if (res == -4)
    {
        reg_msg.instruction = 182;
		reg_msg.length = 0;
		res2 = sp_send_msg(s, &reg_msg);
        if (res2 == U_SOCKET_ERROR) throw SYSTEM_EXCEPTION(string(usocket_error_translator()).c_str());
    }

    ushutdown_close_socket(s, __sys_call_error);

	return 0;
}

/// Sends message to the se_rc utility. Message can be one of three types:
/// 1. | SE_RC_INVALID  |   (message length 1) - means that RC is invalid at the moment;
/// 2. | SE_RC_OVERFLOW |   (message length 1) - means that RC is too large to be sent;
/// 3. | SE_RC_VALID    |  sizeof(int)         | sizeof(int)        | zero ended string | ... |  sizeof(int)        | 
///    | validness byte |  number of databases | number of sessions | db name           | ... |  number of sessions |
///    message length > 1, RC contains list of all databases names and count of sessions for each database
void send_runtime_config(USOCKET s)
{
    rc_vector rc;     /// runtime configuration vector of (database name, sessions count) pairs
    msg_struct msg;   /// message we are going to sent to the se_rc
    int len = 1;      /// current offset within the msg.body(), 
                      /// fist byte is always indicates validness and oveflow
    int res;

    msg.instruction = 0; /// not important here

    if( 1 == gov_table->get_rc(rc) )
    {
        msg.body[0] = SE_RC_INVALID;     /// gov table is not consistent now
    }
    else
    {
        msg.body[0] = SE_RC_VALID;

          /// write number of databases
        int2net_int(rc.size(), msg.body + len);
        len += sizeof(int);

        rc_const_iterator rit = rc.begin();
        rc_const_iterator rit_end = rc.end();

        for(; rit != rit_end; rit++)
        {
            int db_name_len = rit->first.size();
              
              /// sizeof(int) + (db_name_len + 1) means number of sessions and zero ended database name
            if( len + sizeof(int) + (db_name_len + 1) > SE_SOCKET_MSG_BUF_SIZE )
            {
                elog(EL_ERROR, ("Can't send runtime configuration since it is too large"));
                msg.body[0] = SE_RC_OVERFLOW;
                len = 1;
                break;
            }
              
              /// write number of sessions connected to this database
            int2net_int(rit->second, msg.body + len);
            len += sizeof(int);
              
              /// write name of the database and zero at the end
            memcpy(msg.body + len, rit->first.c_str(), db_name_len);
            msg.body[len + db_name_len] = '\0';
            len += db_name_len + 1;
        }
        
        msg.length = len;
    }

    res = sp_send_msg(s, &msg);

    if (res == U_SOCKET_ERROR)
       elog(EL_ERROR, ("Can't send runtime configuration (connection error)"));

    ushutdown_close_socket(s, __sys_call_error);
}

void check_sm_run(USOCKET s, char* msg_buf)
{
	msg_struct msg;
    bool run;
    int res;

    d_printf2("database name to check=%s\n", msg_buf);
    run = gov_table->is_database_run(msg_buf);

    if (run) (msg.body)[0] = 'y';
    else (msg.body)[0] = 'n';

    msg.length = 1;

    res = sp_send_msg(s, &msg);

    if (res == U_SOCKET_ERROR)
       d_printf1("Can't send reply to ddb utility\n");

    ushutdown_close_socket(s, __sys_call_error);
}
