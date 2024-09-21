/*------------------------------------------------
#!#M# Version $Id: gcsupervisor.c,v 1.1.1.1 2013/10/26 11:36:49 lahlou Exp $*
------------------------------------------------*/

/*
#!#M#-------------------------------------------------------------------------
#!#M#			MAGIX Middle Office Server 7i (C)M2M 2000-2006
#!#M#			All rights reserved. 
#!#M#-------------------------------------------------------------------------
#!#M#			Program  	: Visa7.4.x2 
#!#M#			Description	: module interface avec le module switch
#!#M#			Module		: gcsupervisor.c
#!#M#			Description	: module de supervision du gestionnaire 
#!#M#			comminication.
#!#M#	Write 	Author		: H.OUAADA   		Date : 07.02.2000
#!#M#	Release Description	: 
#!#M#			Author		: 					Date :
#!#M#			Description	:
#!#M#			Author		: A.HAIMOURA            
#!#M#-------------------------------------------------------------------------
*/
#include "types.h"
#include "define.h"
#include <poll.h>
#include <stropts.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <time.h>
#include "Emm_conf.h"
#include "version.h"
/*********************VARIABLES GLOBALES**************************/
char ModuleEnv;		/*T:test,P:prod*/
char ServA[3];
char ServI[3];
char argCodbqe[6];	/*'00000':cas int visa*/
char ModuleBaseI;
char InterfaceBaseI[5];
char PathConf[80];
char PathTraces[80];
char PathTracesParent[80];
char PathTracesSuffix[80];
char PathTracesEnv[80];

char w_file[120];
char ServerCode[3];
int DEVISA;
int timer_global=NOTIMEOUT;
int next_indic_timeout = -1;/* indice de l'entree qui va expirer la premiere */
int cur_indic_timeout = -1;/* indice de l'entree qui a expire */
int _x25Connect  ;
char adress_switch_acq[5];
char adress_switch_iss[5];
char adress_reseau[5];
extern int netwcode;
extern unsigned char buffvisa_reqav[1200];
int trac;
int i_trac = 0;
int fois=0;
char text[4000];
int TimeOutPoll=-1;
int SeuilErrBaseCfg;
extern int ContourErrBase;
extern char buff_sw[1024];
char CodeREASONVal[5]; /* H.Marouane 160608 */
int test_crypt=0; /* TEST cryptage*/
int VAL_ASCII=1; /* TEST cryptage*/
/*********************FILES**************************/
FILE * ftrac;
FILE * fichtra ;
FILE * ftracdata;
FILE * ftracech;
FILE * fpc;
extern FILE *ftracdata;
extern FILE *ftraciv;
extern FILE *ftracav;
/*********************STRUCTURES**************************/
struct tvisaconf 	v_visaconf;
struct _crn gen_crn();
struct pollfd fds[MAXPT];
struct teven_out even_out;
struct teven_in even_in;
struct tab_csd TabCascade[MAXPT];
extern struct tmsg tab_msg[100];
extern struct tcom v_com;
char mes_test[2000];/*TEST cryptage*/



void release(int sig)
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : release
#!#F#              Type       : L 
#!#F#              Input parameters: <sig:signal>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: permet de liberer les pts d'ecoutes crees et 
#!#F#              fermer les pts de transports lors d'un arret force.
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: liberation pts d'ecoutes. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int i,ret1,ret2;
	
	Ftimingtraces("debut release \n",ftrac,0,40);

	insert_status("04",ServerCode,SIGNALRECVD,netwcode);
                sprintf(text,"Signal:%d_\n",
                    sig);
                Ftimingtraces(text,fichtra,0,0);
	for (i=0;i<MAXPT;i++){
		ret1=disconnect(fds[i].fd);
		ret2=fermer(fds[i].fd);
		sprintf(text,"Visa ShutDown--> Desc:%d,ret1:%d,ret2:%d\n",
		    fds[i].fd,ret1,ret2);
		Ftimingtraces(text,fichtra,0,0);
	}
	Ftimingtraces("fin release \n",ftrac,0,40);
	myexit("User ShutDown");
}

void arret_ok(int sig)
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : arret_ok
#!#F#              Type       : L 
#!#F#              Input parameters: <sig:signal>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: l'arret du programme lors de la reception 
#!#F#              du signal SIGUSR1,l'arret du systeme consiste a liberer
#!#F#              les points d'ecoutes crees et la fermeture des points 
#!#F#              de transports grace a la fonction release. 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: arret programme. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int i,ret;
	
	Ftimingtraces("debut arret_ok \n",ftrac,0,40);

	insert_status("04",ServerCode,DOWNRECVD,netwcode);
	fprintf(ftrac,"visa ---> arret_ok,process %d received signal %d==>arret\n",
	    getpid(),sig);
	sleep(1);
	ret = kill(getppid(),SIGUSR2);

	for (i=0;i<MAXPT;i++){
		disconnect(fds[i].fd);
		fermer(fds[i].fd);
	}

	fprintf(stderr,"visa ---> kill retour:%d\n",ret);
	fprintf(stderr,"visa ---> visa process:%d --> killed\n",getpid());
	fflush(stderr);
	Ftimingtraces("fin arret_ok \n",ftrac,0,40);
	exit(0);
}

/*-------------------------------------------------------------------------
	Fonction : EditWarning
	Entree	: signal recu
	Sortie :
	Role  : Editer un warning
-------------------------------------------------------------------------*/
void EditWarning(int sig)
{
	int i;

    Ftimingtraces("start EditWarning()",ftrac,1,40);

	/*if (trac >= 30) {*/
		sprintf(text," WARNING RECEIVED SIGNAL : %d",sig);
		Ftimingtraces(text,ftrac,0,30);
		/*fflush(dbgfile);*/
	/*}*/
	Ftimingtraces("end EditWarning()",ftrac,1,40);
	/* AT/IS 02-09-2004 t_error("CauseWarning");*/
}
int lire_event(int pt_evt[MAXPT])
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : lire_event
#!#F#              Type       : L 
#!#F#              Input parameters: <pt:point ecoute>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: permet de detecter un evenement dans les pts 
#!#F#              d'ecoute crees,la detection se fait grace a la primitive poll
#!#F#              qui recoit en parametre une table qui contient les 
#!#F#              descripteurs de tous les points crees et qui nous renvoi 
#!#F#              les descripteurs ou l'evenement s'est produit. 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: liberation pts d'ecoutes. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int desc,i,n,j;
	
	Ftimingtraces("debut lire_event \n",ftrac,0,40);

	n=-1;
	j=0;
	n = poll(fds,MAXPT,TimeOutPoll);

	sprintf(text,"nombre of event:%d\n",n);
	Ftimingtraces(text,fichtra,0,10);

	if (n < 0){
		Ftimingtraces("fin lire_event \n",ftrac,0,40);
		return(-1);
	}

	for (i = 0;i < MAXPT ; i++ ){
		sprintf(text,"fd:%d,revents:%d,events:%d\n",
		    fds[i].fd,fds[i].revents,fds[i].events);
		Ftimingtraces(text,fichtra,0,40);
	}

	for (i = 0;i < MAXPT ; i++ ) {
		if (fds[i].revents == 1) {
			sprintf(text,"event at desc:%d,indice:%d\n",fds[i].fd,i);
			Ftimingtraces(text,fichtra,0,10);
			fds[i].revents = -1;
			pt_evt[j++] = fds[i].fd;
			if (j == n)
				break;
		}
	}
	Ftimingtraces("fin lire_event \n",ftrac,0,40);
	return(n);
}

void f_alarme(int sig)
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : f_alarme
#!#F#              Type       : L 
#!#F#              Input parameters: <sig:signal>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: Handler du signal SIGALRM 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: Handler du signal SIGALRM. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	
	Ftimingtraces("debut f_alarme \n",ftrac,0,40);
	
	fois+=1;
	sprintf(text,"alarme received for %d times\n",fois);
	Ftimingtraces(text,fichtra,0,10);

	sighold(SIGALRM);
	trait_timeout();
	sigrelse(SIGALRM);
	signal(SIGALRM,f_alarme);
	Ftimingtraces("fin f_alarme \n",ftrac,0,40);
}

timer(int ind,int timeout)
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : timer
#!#F#              Type       : L 
#!#F#              Input parameters: <ind:indice dde envoyee,timeout en seconde>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: permet de lancer une alarme pour la demande 
#!#F#              ayant le plus petit timeout.
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: lancement alarme. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	time_t temps,tloc,temps_min,timer;

	temps=time(&tloc);
	if (tab_msg[ind].status != 0)
		tab_msg[ind].time_xpr = temps + timeout;
	temps_min = inf_time();
	if (temps_min == 0)
		return(0);
	timer = temps_min - temps;
	if (timer < 0) timer=1;
	alarm(timer);
	return(0);
}

inf_time()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : inf_time
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <indice de la demande>
#!#F#              Description: recherche de la demande qui expirera la 
#!#F#              premiere et revoi son indice.
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: renvoi l'indice de la demande. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int i,pos=0;
	long temp,min = 0;
	
	Ftimingtraces("debut inf_time \n",ftrac,0,40);

	for(i = 0;i<100;i++) {
		temp = -1;
		if (tab_msg[i].status== 0)
			continue;
		if (tab_msg[i].status== 2)
			continue;
		temp = tab_msg[i].time_xpr;
		if (temp != -1)  {
			if ((temp < min) || (min == 0)) {
				pos = i;   		/* position du minimum */
				min = temp;
			}
		}
	}
	if (min==0){
		Ftimingtraces("fin inf_time \n",ftrac,0,40);
		return(min) ; /* aucune demande a armer*/
	}

	sprintf(text,"position of minimum:%d\n",pos );
	Ftimingtraces(text,fichtra,0,20);

	next_indic_timeout = pos;
	Ftimingtraces("fin inf_time \n",ftrac,0,40);
	return(min);
}

init_variables()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : init_variables
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: permet l'initialisation des structures :
#!#F#              1-tab_msg qui contient des infos sur les message en cours
#!#F#              du traitement(type msg,nombre de repetition,...)
#!#F#              2-fds qui contient les descripteurs de tous les points
#!#F#              de communication crees.
#!#F#              Caller Modules: gcsupervisor.c,acq_autoris.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: initialisation structures. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int i;

	for (i = 0;i<100;i++) {
		tab_msg[i].nr = 0;
		tab_msg[i].status=0;
		tab_msg[i].stan = -1;
		tab_msg[i].aid = -1;
		tab_msg[i].fid = -1;
	}
	for (i = 0;i<MAXPT;i++) {
		fds[i].fd = -1;
		fds[i].events = -1;
		fds[i].revents = -1;
	}
}
gc()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : gc
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: c'est le moteur de l'application,permet : 
#!#F#              1-la creation des pts d'ecoutes et l'ouverture des pts
#!#F#              de transports grace a la fonction gcconnect.
#!#F#              2-la detection des evnts et leurs traitement selon le type
#!#F#              3-la reception et l'envoi des messages.
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: moteur application. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	static struct msg info_rcpt;
	static struct msg info_emis;
	struct msg_tli_large msg_recu_large;
	int code,traffic,cnt_ret,i;
	int event = -1;
	int ret=-1;
	int origin = -1;
	int val ;
	int type_licence = 2 ;/* -1:rien, 0:acquirer, 1:issuer, 2:les deux */
	FILE * ft ;
	int tab_evt[MAXPT];
	int nb_evt;
	int k,m,ii;
	int nbre_tentative;
	int NbMsg;
	char numtrst[13];
	char CurrDate[9]; /*--Rida 190209  Gestion des Tarces Par date--*/
	
	Ftimingtraces("debut gc \n",ftrac,0,40);

	signal(SIGUSR1,arret_ok);
	signal(SIGSEGV,release);
	signal(SIGQUIT,arret_ok);
	signal(SIGINT,arret_ok);
	signal(SIGHUP,release);
	signal(SIGALRM,f_alarme);
	signal(SIGILL,release);

	signal(SIGPIPE   ,EditWarning);
	signal(SIGTRAP   ,EditWarning);
	signal(SIGIOT    ,EditWarning);
	signal(SIGKILL   ,EditWarning);
	signal(SIGABRT   ,EditWarning);
	signal(SIGEMT    ,EditWarning);
	signal(SIGFPE    ,EditWarning);
	signal(SIGBUS    ,EditWarning);
	signal(SIGSYS    ,EditWarning);
	signal(SIGTERM   ,EditWarning);
	signal(SIGUSR2   ,EditWarning);
	signal(SIGCLD    ,EditWarning);
	signal(SIGCHLD   ,EditWarning);
	signal(SIGPWR    ,EditWarning);
	signal(SIGWINCH  ,EditWarning);
	signal(SIGPOLL   ,EditWarning);
	signal(SIGSTOP   ,EditWarning);
	signal(SIGTSTP   ,EditWarning);
	signal(SIGCONT   ,EditWarning);
	signal(SIGTTIN   ,EditWarning);
	signal(SIGTTOU   ,EditWarning);
	signal(SIGVTALRM ,EditWarning);
	signal(SIGPROF   ,EditWarning);
	/**/

	Ftimingtraces("gcconnect calling\n",fichtra,0,40);

	cnt_ret = gcconnect(type_licence);

	sprintf(text,"gcconnect result:%d\n",cnt_ret);
	Ftimingtraces(text,fichtra,0,30);

	if (cnt_ret < 0){
		for (i = 0;i<MAXPT;i++){
			disconnect(fds[i].fd);
			fermer(fds[i].fd);
		}
		Ftimingtraces("fin gc \n",ftrac,0,40);
		exit(0); /* Rajaa 18-02-2020 myexit("Fatal Error-->See trac_visa.cnt\n");*/
	}

	TimeOutPoll = -1;

	while(1) {
		/*--Rida--Gestion des traces Par date-- 190209--*/
    open_trace();
    /*
    memset(CurrDate,'\0',9);
    Get_date_trace(CurrDate);
    if(strncmp(CurrDate,(strrchr(PathTraces,'/')+1),6)){
      fprintf(stderr,"CurrentDate:%s|PathDate:%s_\n",CurrDate,(strrchr(PathTraces,'/')+1));
      fflush(stderr);
      close_trace();
      open_trace();
    }
    */
    /*--Rida--Gestion des traces par date-- 190209--*/
		memset((void *)&info_rcpt,'\0',sizeof(struct msg));

		Ftimingtraces("Polling\n",fichtra,0,10);

		nb_evt = lire_event(tab_evt);
		if (nb_evt <= 0){
			if (v_com.desc == -1)
				Tentative_Connect();
			continue;
		}
		

		for (k=0;k<nb_evt;k++)  {
			synch_trace("debut traitement evenement");
       memset(buff_sw,0,1024);
			info_rcpt.desc = tab_evt[k];
			sighold(SIGALRM);
			event = look(info_rcpt.desc);
			sprintf(text,"RAHIB_1: DATA RECEIVED AT DESC:%d",info_rcpt.desc);
			Ftimingtraces(text,fichtra,0,0);			
			switch(event) {
			case DDE_CONNEX:

				sprintf(text,"DDE_CONNEX,desc:%d\n",info_rcpt.desc);
				Ftimingtraces(text,fichtra,0,10);

				even_in.motif=info_rcpt.desc;
				accepter_connexion(info_rcpt.desc);

				if((info_rcpt.desc==v_com.desc)&&(v_com.type[0]=='S'))
					insert_status("04",ServerCode,0,netwcode);
				continue;
			case DATA:
				NbMsg=0;
				sprintf(text,"DATA RECEIVED AT DESC:%d",info_rcpt.desc);
				Ftimingtraces(text,fichtra,0,0);
				afftime(1,fichtra,text);
				fflush(fichtra);
				
					/*memset((void *)&msg_recu_large.msg,'\0',sizeof(msg_recu_large.msg));*/
					memset((void *)&msg_recu_large,'\0',sizeof(struct msg_tli_large));
					
					msg_recu_large.desc = info_rcpt.desc;
					reception_large(&msg_recu_large);
					fprintf(fichtra,"\nMSG RECU :");
					fflush(fichtra);
					sprintf(text,"RAHIB_2: DATA RECEIVED AT DESC:%d",info_rcpt.desc);
					Ftimingtraces(text,fichtra,0,0);						
				for(ii=0;ii<msg_recu_large.lg;ii++){
				fprintf(fichtra,"%c",msg_recu_large.msg[ii]);
				fflush(fichtra);
				}
				if(info_rcpt.desc==v_com.desc){
					 
					NbMsg=DecomposeCascadeIso(msg_recu_large.msg,msg_recu_large.lg);
				}
				else {
					NbMsg = DecomposeCascade(msg_recu_large.msg);
					sprintf(text,"RAHIB_3: DATA RECEIVED AT DESC:%d",info_rcpt.desc);
					Ftimingtraces(text,fichtra,0,0);						
					
				}
				fprintf(fichtra,"\nNbMsg:%d\n",NbMsg);
					/*
					sprintf(text,"msg_recu_large.msg : -%s-\n", (char*) msg_recu_large.msg);
					Ftimingtraces(text,fichtra,0,0);
					*/
					sprintf(text,"msg_recu_large.lg : -%d-\n", msg_recu_large.lg);
					Ftimingtraces(text,fichtra,0,0);

					sprintf(text,"strlen(msg_recu_large.msg) : -%d-\n", strlen(msg_recu_large.msg));
					Ftimingtraces(text,fichtra,0,0);
					
					sprintf(text,"decomposecascade : %d\n",NbMsg);
					Ftimingtraces(text,fichtra,0,0);
					edit_trace_ih2(0,ftracech,msg_recu_large.msg,msg_recu_large.lg,msg_recu_large.desc);
					sprintf(text,"RAHIB_4: DATA RECEIVED AT DESC:%d",info_rcpt.desc);
					Ftimingtraces(text,fichtra,0,0);						
				/*traitement msgnull (4*null) ** debut */
				if(!memcmp(msg_recu_large.msg,"\0\0\0\0",4)) {
					sprintf(text,"nul_msg RECEIVED at desc:%d",info_rcpt.desc);
					Ftimingtraces(text,ftrac,0,0);
					if(info_rcpt.lg == 4) {
						Ftimingtraces("null_msg ignored\n",ftrac,0,0);
						continue;
					}
					if(info_rcpt.lg > 4) {
						memcpy(msg_recu_large.msg,msg_recu_large.msg+4,msg_recu_large.lg - 4);
						msg_recu_large.lg = msg_recu_large.lg - 4;
						Ftimingtraces("null_msg corrected\n",ftrac,0,0);
					}
				}
				break;
			case LIB:
					sprintf(text,"LIB RECEIVED FROM desc:%d\n",
					    info_rcpt.desc);
					Ftimingtraces(text,fichtra,0,10);

				liberer(info_rcpt.desc);
				ret = -1;
				if (v_com.type[0]=='S'){
					insert_status("04",ServerCode,ret,netwcode);
				}
				if((info_rcpt.desc==v_com.desc)&&(v_com.type[0]=='C')){
					v_com.desc = -1;
					Update_Rout("04",ServA,ServI,'0');
					insert_status("04",ServerCode,ret,netwcode);
					sleep(2*v_com.TimeOut);
					Tentative_Connect();
				}
				continue;

			default :
					sprintf(text,"UNKOWN EVENT,desc:%d\n",info_rcpt.desc);
					Ftimingtraces(text,fichtra,0,10);
				continue;
			}
			sprintf(text,"RAHIB_5: DATA RECEIVED AT DESC:%d",info_rcpt.desc);
			Ftimingtraces(text,fichtra,0,0);				
			if (v_com.desc == -1) {
				sprintf(text,"ret:%d,netcode:%d\n",ret, netwcode);
				Ftimingtraces(text,ftracdata,0,0);
				insert_status("04",ServerCode,-1,netwcode);
				Tentative_Connect();
			}

			for(m=0;m<NbMsg;m++){
				sprintf(text,"RAHIB_6: DATA RECEIVED AT DESC:%d",info_rcpt.desc);
				Ftimingtraces(text,fichtra,0,0);					
				info_rcpt.lg = TabCascade[m].lg;
				memset(info_rcpt.msg,'\0',sizeof(info_rcpt.msg));
				memcpy(info_rcpt.msg,TabCascade[m].msg,TabCascade[m].lg);
				sprintf(text,"RAHIB_7: DATA RECEIVED AT DESC:%d",info_rcpt.desc);
				Ftimingtraces(text,fichtra,0,0);					
				fprintf(fichtra,"\nMSG Numero %d lg=%d\n:",m,info_rcpt.lg);
				fflush(fichtra);
			 	for(ii=0;ii<info_rcpt.lg;ii++){
				fprintf(fichtra,"%c",info_rcpt.msg[ii]);
				fflush(fichtra);
				}
					if(test_crypt){
						crypter_msg(info_rcpt.msg,mes_test,info_rcpt.lg);
						if(!VAL_ASCII)
							for(i=0;i<(info_rcpt.lg+8);i++)
								fprintf(fichtra,"%02X ",mes_test[i]);
					}
					/*if(test_crypt)ELYAAGOUBI 02/12/2010 debut
				{
					memcpy(mes_test,info_rcpt.msg,info_rcpt.lg);
					mes_test[info_rcpt.lg+1]='\0';
					crypter(VAL_ASCII, mes_test,info_rcpt.lg);
					fprintf(fichtra,"m:-%d-,size:-%d-,msg:-\n",
				    m,info_rcpt.lg);
					for(i=0;i<info_rcpt.lg;i++){
							fprintf(fichtra,"%02X ",(unsigned char)mes_test[i]);
					}
					fprintf(fichtra,"-\n");
				}ELYAAGOUBI 02/12/2010 fin*/
				if(!test_crypt){
				sprintf(text,"m:-%d-,size:-%d-,msg:-%s-\n",
				    m,info_rcpt.lg,info_rcpt.msg);
				Ftimingtraces(text,fichtra,0,0);
				}
				sprintf(text,"ContourErrBase:%d,SeuilErrBaseCfg:%d\n",
				    ContourErrBase,SeuilErrBaseCfg);
				Ftimingtraces(text,fichtra,0,0);

					sprintf(text,"Desc0=<%d>\n", fds[0].fd);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"Desc1=<%d>\n", fds[1].fd);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"Desc2=<%d>\n", fds[2].fd);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"Desc3=<%d>\n", fds[3].fd);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"Desc4=<%d>\n", fds[4].fd);
					Ftimingtraces(text,fichtra,0,0);

					sprintf(text,"info_rcpt.desc =<%d>\n", info_rcpt.desc);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"SeuilErrBaseCfg =<%d>\n", SeuilErrBaseCfg);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"ContourErrBase=<%d>\n", ContourErrBase);
					Ftimingtraces(text,fichtra,0,0);

				if((fds[1].fd==info_rcpt.desc || fds[4].fd==info_rcpt.desc)
				    && ContourErrBase  >= SeuilErrBaseCfg && SeuilErrBaseCfg!=0)
				{
					Ftimingtraces("message from switch depassement erreur base\n",fichtra,0,0);

					if(fds[1].fd==info_rcpt.desc)
						even_out.dest=ACQ;
					else
						even_out.dest=GC;

					even_out.lg=20;
					get_champs(info_rcpt.msg,strlen(info_rcpt.msg),26,numtrst);
					sprintf(even_out.msg,"02000296026%03d%s",strlen(numtrst),numtrst);
					env_msg();
					continue;
				}

				if((fds[2].fd==info_rcpt.desc || fds[0].fd==info_rcpt.desc)
				    && ContourErrBase  >= SeuilErrBaseCfg && SeuilErrBaseCfg!=0)
				{
					Ftimingtraces("message from visa depassement erreur base -->pas action\n",fichtra,
					    0,0);
					continue;
				}
				trt_msg(&info_rcpt);
				env_msg();
			}
			sigrelse(SIGALRM);
			synch_trace("fin traitement evenement");
            i_trac = 0;
		}
	}
	Ftimingtraces("fin gc \n",ftrac,0,40);
}
gc_old()/*M.Abbad 31102014*/
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : gc
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: c'est le moteur de l'application,permet : 
#!#F#              1-la creation des pts d'ecoutes et l'ouverture des pts
#!#F#              de transports grace a la fonction gcconnect.
#!#F#              2-la detection des evnts et leurs traitement selon le type
#!#F#              3-la reception et l'envoi des messages.
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: moteur application. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	static struct msg info_rcpt;
	static struct msg info_emis;
	struct msg_tli_large msg_recu_large;
	int code,traffic,cnt_ret,i;
	int event = -1;
	int ret=-1;
	int origin = -1;
	int val ;
	int type_licence = 2 ;/* -1:rien, 0:acquirer, 1:issuer, 2:les deux */
	FILE * ft ;
	int tab_evt[MAXPT];
	int nb_evt;
	int k,m;
	int nbre_tentative;
	int NbMsg;
	char numtrst[13];
	char CurrDate[9]; /*--Rida 190209  Gestion des Tarces Par date--*/
	
	Ftimingtraces("debut gc \n",ftrac,0,40);

	signal(SIGUSR1,arret_ok);
	signal(SIGSEGV,release);
	signal(SIGQUIT,arret_ok);
	signal(SIGINT,arret_ok);
	signal(SIGHUP,release);
	signal(SIGALRM,f_alarme);
	signal(SIGILL,release);

	Ftimingtraces("gcconnect calling\n",fichtra,0,40);

	cnt_ret = gcconnect(type_licence);

	sprintf(text,"gcconnect result:%d\n",cnt_ret);
	Ftimingtraces(text,fichtra,0,30);

	if (cnt_ret < 0){
		for (i = 0;i<MAXPT;i++){
			disconnect(fds[i].fd);
			fermer(fds[i].fd);
		}
		Ftimingtraces("fin gc \n",ftrac,0,40);
		myexit("Fatal Error-->See trac_visa.cnt\n");
	}

	TimeOutPoll = -1;

	while(1) {
		/*--Rida--Gestion des traces Par date-- 190209--*/
    
    open_trace();
    /*
    memset(CurrDate,'\0',9);
    Get_date_trace(CurrDate);
    if(strncmp(CurrDate,(strrchr(PathTraces,'/')+1),6)){
      fprintf(stderr,"CurrentDate:%s|PathDate:%s_\n",CurrDate,(strrchr(PathTraces,'/')+1));
      fflush(stderr);
      close_trace();
      open_trace();
    }
    */
    /*--Rida--Gestion des traces par date-- 190209--*/
		memset((void *)&info_rcpt,'\0',sizeof(struct msg));

		Ftimingtraces("Polling\n",fichtra,0,10);

		nb_evt = lire_event(tab_evt);
		if (nb_evt <= 0){
			if (v_com.desc == -1)
				Tentative_Connect();
			continue;
		}

		for (k=0;k<nb_evt;k++)  {
			synch_trace("debut traitement evenement");
            		memset(buff_sw,0,1024);
			info_rcpt.desc = tab_evt[k];
			sighold(SIGALRM);
			event = look(info_rcpt.desc);
			switch(event) {
			case DDE_CONNEX:

				sprintf(text,"DDE_CONNEX,desc:%d\n",info_rcpt.desc);
				Ftimingtraces(text,fichtra,0,10);

				even_in.motif=info_rcpt.desc;
				accepter_connexion(info_rcpt.desc);

				if((info_rcpt.desc==v_com.desc)&&(v_com.type[0]=='S'))
					insert_status("04",ServerCode,0,netwcode);
				continue;
			case DATA:
				NbMsg=0;
				sprintf(text,"DATA RECEIVED AT DESC:%d",info_rcpt.desc);
				afftime(1,fichtra,text);
				fflush(fichtra);
				if(info_rcpt.desc==v_com.desc){
					reception(&info_rcpt);

					sprintf(text,"info_rcpt.msg : -%s-\n", (char*) info_rcpt.msg);
					Ftimingtraces(text,fichtra,0,0);

					sprintf(text,"info_rcpt.lg : -%d-\n", info_rcpt.lg);
					Ftimingtraces(text,fichtra,0,0);

					sprintf(text,"strlen(info_rcpt.msg) : -%d-\n", strlen(info_rcpt.msg));
					Ftimingtraces(text,fichtra,0,0);

					memset((void *)&TabCascade[0].msg,'\0',sizeof(TabCascade[0].msg));
					memcpy(TabCascade[0].msg,info_rcpt.msg,info_rcpt.lg);
					TabCascade[0].lg=info_rcpt.lg;

					sprintf(text,"msg from visa [%d]\n",TabCascade[0].lg);
					Ftimingtraces(text,fichtra,0,0);

					NbMsg = 1;
					edit_trace_ih2(0,ftracech,info_rcpt.msg,
					    info_rcpt.lg,info_rcpt.desc);
				}
				else {
					memset((void *)&msg_recu_large.msg,'\0',sizeof(msg_recu_large.msg));
					msg_recu_large.desc = info_rcpt.desc;
					reception_large(&msg_recu_large);
					NbMsg = DecomposeCascade(msg_recu_large.msg);
					sprintf(text,"decomposecascade : %d\n",NbMsg);
					Ftimingtraces(text,fichtra,0,0);
					edit_trace_ih2(0,ftracech,msg_recu_large.msg,
					    msg_recu_large.lg,msg_recu_large.desc);
				}
				/*traitement msgnull (4*null) ** debut */
				if(!memcmp(info_rcpt.msg,"\0\0\0\0",4)) {
					sprintf(text,"nul_msg RECEIVED at desc:%d",info_rcpt.desc);
					Ftimingtraces(text,ftrac,0,0);
					if(info_rcpt.lg == 4) {
						Ftimingtraces("null_msg ignored\n",ftrac,0,0);
						continue;
					}
					if(info_rcpt.lg > 4) {
						memcpy(info_rcpt.msg,info_rcpt.msg+4,info_rcpt.lg - 4);
						info_rcpt.lg = info_rcpt.lg - 4;
						Ftimingtraces("null_msg corrected\n",ftrac,0,0);
					}
				}
				break;
			case LIB:
					sprintf(text,"LIB RECEIVED FROM desc:%d\n",
					    info_rcpt.desc);
					Ftimingtraces(text,fichtra,0,10);

				liberer(info_rcpt.desc);
				ret = -1;
				if (v_com.type[0]=='S'){
					insert_status("04",ServerCode,ret,netwcode);
				}
				if((info_rcpt.desc==v_com.desc)&&(v_com.type[0]=='C')){
					v_com.desc = -1;
					Update_Rout("04",ServA,ServI,'0');
					insert_status("04",ServerCode,ret,netwcode);
					sleep(2*v_com.TimeOut);
					Tentative_Connect();
				}
				continue;

			default :
					sprintf(text,"UNKOWN EVENT,desc:%d\n",info_rcpt.desc);
					Ftimingtraces(text,fichtra,0,10);
				continue;
			}
			if (v_com.desc == -1) {
				sprintf(text,"ret:%d,netcode:%d\n",ret, netwcode);
				Ftimingtraces(text,ftracdata,0,0);
				insert_status("04",ServerCode,-1,netwcode);
				Tentative_Connect();
			}

			for(m=0;m<NbMsg;m++){
				info_rcpt.lg = TabCascade[m].lg;
				memset(info_rcpt.msg,'\0',sizeof(info_rcpt.msg));
				memcpy(info_rcpt.msg,TabCascade[m].msg,TabCascade[m].lg);
					if(test_crypt){
						crypter_msg(info_rcpt.msg,mes_test,info_rcpt.lg);
						if(!VAL_ASCII)
							for(i=0;i<(info_rcpt.lg+8);i++)
								fprintf(fichtra,"%02X ",mes_test[i]);
					}
					/*if(test_crypt)ELYAAGOUBI 02/12/2010 debut
				{
					memcpy(mes_test,info_rcpt.msg,info_rcpt.lg);
					mes_test[info_rcpt.lg+1]='\0';
					crypter(VAL_ASCII, mes_test,info_rcpt.lg);
					fprintf(fichtra,"m:-%d-,size:-%d-,msg:-\n",
				    m,info_rcpt.lg);
					for(i=0;i<info_rcpt.lg;i++){
							fprintf(fichtra,"%02X ",(unsigned char)mes_test[i]);
					}
					fprintf(fichtra,"-\n");
				}ELYAAGOUBI 02/12/2010 fin*/
				if(!test_crypt)
				sprintf(text,"m:-%d-,size:-%d-,msg:-%s-\n",
				    m,info_rcpt.lg,info_rcpt.msg);
				Ftimingtraces(text,fichtra,0,0);

				sprintf(text,"ContourErrBase:%d,SeuilErrBaseCfg:%d\n",
				    ContourErrBase,SeuilErrBaseCfg);
				Ftimingtraces(text,fichtra,0,0);

					sprintf(text,"Desc0=<%d>\n", fds[0].fd);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"Desc1=<%d>\n", fds[1].fd);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"Desc2=<%d>\n", fds[2].fd);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"Desc3=<%d>\n", fds[3].fd);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"Desc4=<%d>\n", fds[4].fd);
					Ftimingtraces(text,fichtra,0,0);

					sprintf(text,"info_rcpt.desc =<%d>\n", info_rcpt.desc);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"SeuilErrBaseCfg =<%d>\n", SeuilErrBaseCfg);
					Ftimingtraces(text,fichtra,0,0);
					sprintf(text,"ContourErrBase=<%d>\n", ContourErrBase);
					Ftimingtraces(text,fichtra,0,0);

				if((fds[1].fd==info_rcpt.desc || fds[4].fd==info_rcpt.desc)
				    && ContourErrBase  >= SeuilErrBaseCfg && SeuilErrBaseCfg!=0)
				{
					Ftimingtraces("message from switch depassement erreur base\n",fichtra,0,0);

					if(fds[1].fd==info_rcpt.desc)
						even_out.dest=ACQ;
					else
						even_out.dest=GC;

					even_out.lg=20;
					get_champs(info_rcpt.msg,strlen(info_rcpt.msg),26,numtrst);
					sprintf(even_out.msg,"02000296026%03d%s",strlen(numtrst),numtrst);
					env_msg();
					continue;
				}

				if((fds[2].fd==info_rcpt.desc || fds[0].fd==info_rcpt.desc)
				    && ContourErrBase  >= SeuilErrBaseCfg && SeuilErrBaseCfg!=0)
				{
					Ftimingtraces("message from visa depassement erreur base -->pas action\n",fichtra,
					    0,0);
					continue;
				}
				trt_msg(&info_rcpt);
				env_msg();
			}
			sigrelse(SIGALRM);
			synch_trace("fin traitement evenement");
            i_trac = 0;
		}
	}
	Ftimingtraces("fin gc \n",ftrac,0,40);
}

int DecomposeCascade(char *msg_recu_large)
{
	/*
	#!#F#----------------------------------------------------------------------
	#!#F#           Fonction : DecomposeCascade
	#!#F#           Entree	 : Buffer contenant des messages en cascade
	#!#F#           Sortie   : Aucune 
	#!#F#           Role     : Decompose un buffer comportant plusieurs message
	#!#F#           		   elle arange les different message dans un tableau
	#!#F#           Auteur	 : HAKIMI
	#!#F#           Date	 : 01/09/03
	#!#F#           Caller	 : swsupervisor.c 
	#!#F#----------------------------------------------------------------------
	*/
	int NbMsg=-1;
	int lg,LgMsgi,nummsg=0;
	char ValChamps[100];
	
	char code[4],ch_code[4],ch_code1[7]; 
	int i,j;
	int posf,posd,posstr;
  int ch_sv_n;
  char ch_sv[4];

	lg = strlen(msg_recu_large);
	fprintf(ftrac,"longueur message (%d)\n", lg);
	fflush(ftrac);
	
	Ftimingtraces("start DecomposeCascade()",ftrac,1,40);
	/* AE 27/03/2014 for(i=0;i<MAXPT;i++) {*/
	for(i=0;i<30;i++) {
		memset((void *)&TabCascade[i],'\0', sizeof(struct tab_csd));
	}
	lg = strlen(msg_recu_large);
	fprintf(ftrac,"longueur message (%d)\n", lg);
	fflush(ftrac);


  /*On rechere le debut de message correct*/
  posstr=0;
  
  while(lg>0){
  	strcpy(msg_recu_large,strstr(msg_recu_large+posstr,"000001"));
   	lg = strlen(msg_recu_large);
   	fprintf(ftrac,"longueur message (%d) posstr (%d)\n", lg,posstr);
		fflush(ftrac);
   	 j=0;
   	 posd=0;
  	 posf=0;
   for (j=1;j<6;j++){ /*pour s'assurer qu'on est sur le bon début de msg*/
   	memcpy(ch_sv,msg_recu_large+posd,3);
   	fprintf(ftrac,"avant get_champs_casc 1 ch_sv (%s) \n",ch_sv);
   	for (i=0;(i<3)&& isdigit(ch_sv[i]);i++){}
   		fprintf(ftrac,"avant get_champs_casc 2 i (%d) \n",i);
			fflush(ftrac);
			if(i<2)
				break;
			ch_sv_n = atoi(ch_sv);
			fprintf(ftrac,"avant get_champs_casc 3 ch_sv_n (%d) j (%d) \n", ch_sv_n,j);
			fflush(ftrac);
			posf=get_champs_casc(msg_recu_large,lg,ch_sv_n,ValChamps);
			fprintf(ftrac,"apre get_champs_casc posf (%d) \n", posf);
			fflush(ftrac);
			if(posf==-1)
				break;
			posd+=3+3+strlen(ValChamps);
			if(posd>=lg)
			j=5;
		}
		if(j>=5)
			break;
		else{
			posstr+=7;
			continue;
		}
  }
  Ftimingtraces("",ftrac,0,40);
  if(!test_crypt)
	fprintf(ftrac,"Apres cherche debut:%s\n\n",msg_recu_large);
	fflush(ftrac);
	
	 if(lg<14){
	 			Ftimingtraces("",ftrac,0,40);
				fprintf(ftrac,"Nbr Msg detecte = 0\n");
				fflush(ftrac);
      	
      	return(0);
      }
  
   posd=0;
   posf=0;
	while(1){
   	posf=get_champs_casc(msg_recu_large+posd+7,lg-7-posd,0,ValChamps);
   	if(posf==0||posf==-1)
   		break;
   	memcpy(TabCascade[nummsg].msg,msg_recu_large + posd,posf);
   	TabCascade[nummsg].lg=posf;
   	posd+=posf;
   	nummsg++;
   	continue;
  }
 
  if(lg-posd>=27 && posf!=-1){
  	memcpy(TabCascade[nummsg].msg,msg_recu_large + posd,lg-posd);
  	TabCascade[nummsg].lg=lg-posd;
   	nummsg+=1;
  }
	for(i=0;i<nummsg;i++){
		if(!test_crypt)
			fprintf(ftrac,"TabCascade.msg[%d][lg:%d]:--->|%s|\n",i, TabCascade[i].lg,TabCascade[i].msg);
		else
			fprintf(ftrac,"TabCascade.msg[%d][lg:%d]\n",i, TabCascade[i].lg);
	}
		fflush(ftrac);
	
	NbMsg=nummsg;


	fprintf(ftrac,"end DecomposeCascade(return nbmsg:%d)\n",NbMsg);
	fflush(ftrac);
	return(NbMsg);
}

int DecomposeCascade_old(char *msg_recu_large)
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : DecomposeCascade
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: decompose un buffer contenant plusieurs msgs 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: decomposition buffer. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int NbMsg=-1;
	int lg,LgMsgi,nummsg=0;
	char ValChamps[50];
	char msg[2048];
	int debut=0,fin=0;
	char code[4],ch_code[4];
	
	Ftimingtraces("debut DecomposeCascade \n",ftrac,0,40);

	lg = strlen(msg_recu_large);
	sprintf(code,"%03d",0);
	code[3]='\0';

	while(fin < lg){
		memcpy(ch_code,msg_recu_large +fin,3);
		ch_code[3]='\0';
		if (strncmp(code,ch_code,3)!=0){
			fin+=3;
			memcpy(ch_code,msg_recu_large +fin,3);
			ch_code[3]='\0';
			ch_code[3] = '\0';
			fin+=3+atoi(ch_code);
		}
		else{
			NbMsg++;
			if(NbMsg){
				TabCascade[nummsg].lg = fin-debut;
				memcpy(TabCascade[nummsg].msg,msg_recu_large +debut,fin-debut);
				nummsg++;
				debut=fin;
			}
			fin+=3;
			memcpy(ch_code,msg_recu_large +fin,3);
			ch_code[3]='\0';
			ch_code[3] = '\0';
			fin+=3+atoi(ch_code);
		}
	}
	TabCascade[nummsg].lg = fin-debut;
	memcpy(TabCascade[nummsg].msg,msg_recu_large +debut,fin-debut);
	NbMsg++;

	Ftimingtraces("fin DecomposeCascade \n",ftrac,0,40);
	return(NbMsg);
}

int HexToInt(char ChHex)
{
	/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : HexToInt
#!#F#              Type       : L 
#!#F#              Input parameters: <ChHex:chaine en hexa>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: convertir un hexa decimal en decimal 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: conversion hexa au decimal. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/

Ftimingtraces(" Start  HexToInt()",ftrac,1,40);

	switch(ChHex) {
	case '0':
		return 0;
		break;
	case '1':
		return 1;
		break;
	case '2':
		return 2;
		break;
	case '3':
		return 3;
		break;
	case '4':
		return 4;
		break;
	case '5':
		return 5;
		break;
	case '6':
		return 6;
		break;
	case '7':
		return 7;
		break;
	case '8':
		return 8;
		break;
	case '9':
		return 9;
		break;
	case 'A':
		return 10;
		break;
	case 'B':
		return 11;
		break;
	case 'C':
		return 12;
		break;
	case 'D':
		return 13;
		break;
	case 'E':
		return 14;
		break;
	case 'F':
		return 15;
		break;
	}
	Ftimingtraces(" End  HexToInt()",ftrac,1,40);
}

int DecomposeCascadeIso(char *msg_recu_large,int lg)  {/*M.Abbad 31102014*/
	/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : DecomposeCascadeIso
#!#F#              Type       : L 
#!#F#              Input parameters: <msg_recu_large:msg recu,sa longueur>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description:decompose un buffer contenant des msg concatenes 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: decomposition buffer. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int NbMsg=0;
	int nummsg=0;
	int boucle=0,fin=0;
	char Lgmsg1[3],Lgmsg2[3];
	int  R1,R2,R3,R4;
	
	Ftimingtraces(" Start DecomposeCascadeIso()",ftrac,1,40);

	sprintf(text,"DecomposeCascadeIso,lg msg recu:%d:\n",lg);
  Ftimingtraces(text,ftrac,1,0);
	fin = 0;

	while((fin < lg)&&(nummsg<41)){
		/*--------Recuperation de la longueur-----------------*/
		memset(Lgmsg1,'\0',3);
		memset(Lgmsg2,'\0',3);

		sprintf(Lgmsg1,"%02X",msg_recu_large[fin]);
		sprintf(Lgmsg2,"%02X",msg_recu_large[fin+1]);

		R1 = HexToInt(Lgmsg1[0]);
		R2 = HexToInt(Lgmsg1[1]);
		R3 = HexToInt(Lgmsg2[0]);
		R4 = HexToInt(Lgmsg2[1]);

		R1 = 4096 * R1;
		R2 = 256  * R2;
		R3 = 16   * R3;
		R4 = R4;
		boucle = R1 + R2 + R3 + R4;
		TabCascade[nummsg].lg = boucle+4;
		/*AE*******/
		fprintf(ftrac,"indice:%d lg msg:%d\n",nummsg,TabCascade[nummsg].lg);
		fflush(ftrac);
		/********/
		memcpy(TabCascade[nummsg].msg,msg_recu_large+fin,boucle+4);
		fin = fin+boucle+4;
		nummsg++;
		NbMsg++;
	}
	Ftimingtraces(" End DecomposeCascadeIso()",ftrac,1,40);
	return(NbMsg);
}

int trait_timeout()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : trait_timeout
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: traitement des messages en timeout 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: traitement timeout. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	struct msg info_rcpt;
	time_t temps,tloc,temps_min,timer;
	int i,pos=0;
	long temp,min = 0;
	
	Ftimingtraces("debut trait_timeout \n",ftrac,0,40);

	temps=time(&tloc);
	for(i = 0;i<100;i++) {
		if (tab_msg[i].status == 0)
			continue;
		if (tab_msg[i].status == 2)
			continue;
		temp = tab_msg[i].time_xpr - temps;
		if (temp <= 0) {
			cur_indic_timeout=i;
			timer_global=TIMEOUT; /* timer global pour gc et acq */
			trt_msg(&info_rcpt);
			env_msg();
		}
	}
	Ftimingtraces("fin trait_timeout \n",ftrac,0,40);
}

int trt_msg(struct msg * msg_rcpt)
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : trt_msg
#!#F#              Type       : L 
#!#F#              Input parameters: <msg_recu:message recu>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: pre-traitement des messages recus: 
#!#F#              1-determination du module demandeur.
#!#F#              2-chargement des structures even_in et even_out.
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: traitement messages. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int mod ,i;
	int indice , indic_dde;
	int origine;
	
	Ftimingtraces("debut trt_msg \n",ftrac,0,40);

	synch_trace("trt_msg()");

	Ftimingtraces("trt_msg,message in processing\n",fichtra,0,10);

	if (timer_global == TIMEOUT) {
		even_in.motif = 1; 			/* message timeout */

		indice = cur_indic_timeout;
		even_out.adr=indice;
		if (tab_msg[indice].nr==4){
			refus_autorisation();
			delete_file(tab_msg[indice].id_demande);

			sprintf(text,"sending nak timeout,status:%d\n",
				    tab_msg[indice].status);
			Ftimingtraces(text,fichtra,0,20);

			if (tab_msg[indice].status==2) even_out.dest=3;
			if (tab_msg[indice].status==1) even_out.dest=1;
			even_out.action=5;
			timer_global = NOTIMEOUT;
			Ftimingtraces("fin trt_msg \n",ftrac,0,40);
			return(1);
		}
		else 
			even_out.action=0;
		even_out.lg = tab_msg[indice].lg_iso;
		memcpy(buffvisa_reqav,tab_msg[indice].buff,1200);
		indic_dde=tab_msg[indice].id_demande;
		position_des_champs(indic_dde);
		insert_requete_avs_base();
		memcpy(even_out.msg,'\0',512);
		memcpy(even_out.msg,tab_msg[indice].msg_iso,even_out.lg);
		even_out.dest=tab_msg[indice].tmod;
		timer_global = NOTIMEOUT;

		sprintf(text,"trt_msg,indice:%d\n",indice);
		Ftimingtraces(text,fichtra,0,40);
		Ftimingtraces("fin trt_msg \n",ftrac,0,40);
		return(1);
	}
	else {
		even_in.indice = -1  ;
		if (msg_rcpt->desc == fds[2].fd) {
			if (v_com.typ_com == X25){
				even_in.lg = msg_rcpt->lg;
				memcpy(even_in.msg,msg_rcpt->msg,even_in.lg);
			}
			else {
				even_in.lg = msg_rcpt->lg - 4;
				memcpy(even_in.msg,msg_rcpt->msg+4,even_in.lg);
			}
		}

		else {
			even_in.lg = msg_rcpt->lg ;
			memcpy(even_in.msg,msg_rcpt->msg,even_in.lg);
		}
		even_in.msg[even_in.lg]='\0';
		origine = msg_rcpt->desc;
		even_in.origin = msg_rcpt->desc;
	}

	DEVISA=0;
	mod = get_mod_type(even_in);
	switch(mod) {
	case ISS:
		Ftimingtraces("message to issuer\n",fichtra,0,10);
		issuer();
		break;
	case ACQ:
		/* H.Marouane 160608 */
		memset((void *)CodeREASONVal,'\0',5);
		strcpy(CodeREASONVal,"0000");
		/* H.Marouane 160608 */
		acquirer();
		break;
	case GC:
		Ftimingtraces("network message in processing\n",fichtra,0,10);
		gest_reseau();
		break;
	default:
		Ftimingtraces("origine inconnue\n",fichtra,0,10);
		Ftimingtraces("fin trt_msg \n",ftrac,0,40);
		return(-1);
	}
	Ftimingtraces("fin trt_msg \n",ftrac,0,40);
}

env_msg()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : env_msg
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: envoi des messages 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: envoi messages. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int lg,b;
	int ret , i;
	struct msg msg_emis;
	
	Ftimingtraces("debut env_msg \n",ftrac,0,40);

	synch_trace("env_msg()");
	sprintf(text,"env_msg,lg message a envoyer:%d\n",even_out.lg);
	Ftimingtraces(text,fichtra,0,10);

	lg = msg_emis.lg = even_out.lg ;
	if (lg > 0)
		memcpy(msg_emis.msg,even_out.msg,lg);
	if(test_crypt)
		{
			crypter_msg(msg_emis.msg,mes_test,lg);
		if(!VAL_ASCII)
			for(i=0;i<(lg+8);i++)
				fprintf(fichtra,"%02X ",mes_test[i]);
		}
	/*if(test_crypt)ELYAAGOUBI 02/12/2010 debut
	{
	memcpy(mes_test,msg_emis.msg,lg);
	mes_test[lg+1]='\0';
	crypter(VAL_ASCII, mes_test,lg);
	fprintf(fichtra,"message a envoyer: ");
		for(i=0;i<lg;i++){
				fprintf(fichtra,"%02X ",(unsigned char)mes_test[i]);
		}
	fprintf(fichtra,"\n");
	}ELYAAGOUBI 02/12/2010 fin*/
	if(!test_crypt){
	sprintf(text,"message a envoyer:-%s-\n",msg_emis.msg);
	Ftimingtraces(text,fichtra,0,10);
    }
	sprintf(text,"message action:%d\n",even_out.action);
	Ftimingtraces(text,fichtra,0,10);

	if(!test_crypt){
		for (i=0;i<=lg;i++) {
			fprintf(fichtra,"%02X. ",even_out.msg[i]);
			if(!mod(i,20))
				fprintf(ftraciv,"\n");
		}
		fprintf(fichtra,"\n\n");
		fflush(fichtra);
	}
	msg_emis.desc= get_desc(even_out.dest);
	switch(even_out.action) {
	case 0:/*emission avec timeout*/
		sprintf(text,"SEND TO DESC %d",msg_emis.desc);
		afftime(1,fichtra,text);
		fflush(fichtra);
		edit_trace_ih2(1,ftracech,msg_emis.msg,msg_emis.lg,msg_emis.desc);
		emission(msg_emis);
		tab_msg[even_out.adr].tmod=even_out.dest;

		sprintf(text,"nbr of repeat:%d\n",tab_msg[even_out.adr].nr);
		Ftimingtraces(text,fichtra,0,10);

		tab_msg[even_out.adr].nr+=1;
		timer(even_out.adr,even_out.timeout);
		break;
	case 1: /* emission sans timeout */
		sprintf(text,"SEND TO DESC %d",msg_emis.desc);
		afftime(1,fichtra,text);
		fflush(fichtra);
		edit_trace_ih2(1,ftracech,msg_emis.msg,msg_emis.lg,msg_emis.desc);
		ret = emission(msg_emis);
		break;
	case 2: /* pas d'action: ne rien emettre */
		Ftimingtraces("pas emission\n",fichtra,0,10);
		break;
	case 3: /* rep recue: annulation timeout */
		edit_trace_ih2(1,ftracech,msg_emis.msg,msg_emis.lg,msg_emis.desc);
		emission(msg_emis);
		break;
	case 4: /* msg erronne : a rejeter */
		Ftimingtraces("msg erronne:a rejeter\n",fichtra,0,10);
		break;
	case 5:	/* sign off nbr de repetitions epuise */
		edit_trace_ih2(1,ftracech,msg_emis.msg,msg_emis.lg,msg_emis.desc);
		emission(msg_emis);
		memset((void *)&tab_msg[even_out.adr],'\0',sizeof(struct tmsg));
		tab_msg[even_out.adr].status=0;
		timer(even_out.adr,0);
		break;
	}
	Ftimingtraces("fin env_msg \n",ftrac,0,40);
}

int chercher_place_vide()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : chercher_place_vide
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <indice premier emplacement vide>
#!#F#              Description: cherche une place vide ds le tableau contenant 
#!#F#              les autorisatins et renvoi l'indice trouve. 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: renvoi indice emplacement vide. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int i;
	
	Ftimingtraces("debut chercher_place_vide \n",ftrac,0,40);
	
	for(i=0;i<100;i++)
		if((tab_msg[i].status==0)|| (tab_msg[i].status==2)){
			memset((void *)&tab_msg[i],'\0',sizeof(struct tmsg));
			break;
		}
	Ftimingtraces("fin chercher_place_vide \n",ftrac,0,40);
	return(i);
}

liberer(int desc)
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : liberer
#!#F#              Type       : L 
#!#F#              Input parameters: <desc:descripteur a liberer>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: liberation des connexions suite a une dde de 
#!#F#              deconnexion,libere l'espace alloue au pt de transport et
#!#F#              libere les entrees reservees dans la table fds.
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: liberation connexions et espaces. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int i;
	
	Ftimingtraces("debut liberer \n",ftrac,0,40);

	sprintf(text,"libertaion du -->desc:%d\n",desc);
	Ftimingtraces(text,fichtra,0,10);

	fermer(desc);
	for(i = 0;i<MAXPT;i++) {
		if( fds[i].fd == desc)
			break;
	}
	fds[i].fd = -1;
	fds[i].revents = -1;
	Ftimingtraces("fin liberer \n",ftrac,0,40);
}

get_desc(int dest)
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : get_desc
#!#F#              Type       : L 
#!#F#              Input parameters: <dest:destination>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <descripteur>
#!#F#              Description: renvoi le descripteur correspondant a la 
#!#F#              destination recue.
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: renvoi descripteur. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int desc;
	
	Ftimingtraces("debut get_desc \n",ftrac,0,40);
	
	switch(dest) {
	case VISA:
		desc = fds[2].fd;
		sprintf(text,"routage vers visa:%d\n",fds[2].fd);
		Ftimingtraces(text,fichtra,0,10);
		break;
	case GC:
		desc = fds[4].fd;
		sprintf(text,"routage reseau,desc:%d\n",desc);
		Ftimingtraces(text,fichtra,0,10);
		break;
	case ISS:
		desc = fds[ISS].fd;
		sprintf(text,"routage vers issueur:%d\n",fds[ISS].fd);
		Ftimingtraces(text,fichtra,0,10);
		break;
	case ACQ:
		desc = fds[ACQ].fd;
		sprintf(text,"routage vers acquereur:%d\n",fds[ISS].fd);
		Ftimingtraces(text,fichtra,0,10);
		break;
	}
	Ftimingtraces("fin get_desc \n",ftrac,0,40);
	return(desc);
}

open_trace()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : open_trace
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: ouverture des fichiers traces,3 types: 
#!#F#              1-pr le superviseur de communication.
#!#F#              2-pr l'issueur.
#!#F#              3-pr l'acquereur.
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: ouverture fichiers traces. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	FILE * fp;
	char  SousTraces[120];
	char dates[9];/*--Rida--Gestion des traces Par date-- 190209--*/
  memset(dates,'\0',9); 
	Get_date_trace(dates);
			/*memcpy(PathTraces+strlen(PathTraces)-6,dates,6);*/
			/*--Rida--Gestion des traces Par date-- 190209--*/
			
	memset(SousTraces,'\0',120);
	sprintf(SousTraces,"%s/%s",PathTracesParent,dates);
	fprintf(stderr,"visa ---> soustraces:%s\n",SousTraces);
	fflush(stderr);	
	mkdir(SousTraces,00777);

	memset(SousTraces,'\0',120);
	sprintf(SousTraces,"%s/%s/%s",PathTracesParent,dates,PathTracesEnv);
	fprintf(stderr,"visa ---> soustraces:%s\n",SousTraces);
	fflush(stderr);
	mkdir(SousTraces,00777);

	memset(SousTraces,'\0',120);
	sprintf(SousTraces,"%s/%s/%s/%s",PathTracesParent,dates,PathTracesEnv,PathTracesSuffix);
	fprintf(stderr,"visa ---> soustraces:%s\n",SousTraces);
	fflush(stderr);
	mkdir(SousTraces,00777);
						
	memset(PathTraces,'\0',80);
	sprintf(PathTraces,"%s/%s/%s/%s",PathTracesParent,dates,PathTracesEnv,PathTracesSuffix);
	fprintf(stderr,"Visa ---> PathTraces:%s\n",PathTraces);
	fflush(stderr);
	
	sprintf(w_file,"%s/trcvs.cfg",PathConf);
	if ((fp = fopen(w_file,"r")) == NULL)
		myexit("Error opening trcvs.cfg");

	fscanf(fp,"%d %d",&trac,&SeuilErrBaseCfg);
	fclose(fp);

	memset(SousTraces,'\0',120);

	mkdir(PathTraces,00777);

	sprintf(SousTraces,"%s/%s",PathTraces,"Vsi");
	fprintf(stderr,"visa ---> soustraces:%s\n",SousTraces);
	fflush(stderr);
	mkdir(SousTraces,00777);
	memset(SousTraces,'\0',120);
	sprintf(SousTraces,"%s/%s",PathTraces,"Trame");
	mkdir(SousTraces,00777);

	memset(SousTraces,'\0',120);
	sprintf(SousTraces,"%s/%s",PathTraces,"Anomal");
	mkdir(SousTraces,00777);

	memset(SousTraces,'\0',120);
	sprintf(SousTraces,"%s/%s",PathTraces,"mail");
	mkdir(SousTraces,00777);

	memset(SousTraces,'\0',120);
	sprintf(SousTraces,"%s/%s",PathTraces,"Mail");
	mkdir(SousTraces,00777);

	sprintf(w_file,"%s/trac.visa",PathTraces);
	if ((ftrac = fopen(w_file,"a"))== NULL)	/* trace de supervision de com */
		printf("Error opening tracvisa");

	sprintf(w_file,"%s/trace.evt",PathTraces);
	if ((fichtra = fopen(w_file,"a"))== NULL) /* trace de supervision de com */
		printf("Error opening trace.evt");

	sprintf(w_file,"%s/tracdata.visa",PathTraces);
	if ((ftracdata = fopen(w_file,"a"))== NULL)  /*trace de la base de donnee*/ 
	printf("Error opening tracdata.visa");

	sprintf(w_file,"%s/traces.ech",PathTraces);
	if ((ftracech = fopen(w_file,"a"))== NULL)  /*trace echange*/ 
	printf("Error opening tracdata.visa");

	ouvre_traceiss();
	ouvre_traceacq();
	return(0);
}


synch_trace(char* chaine)
{

	fprintf(ftrac,    "\n\t\t %d ... %s ...\n",i_trac,chaine);
	fprintf(fichtra,  "\n\t\t %d ... %s ...\n",i_trac,chaine);
	fprintf(ftracdata,"\n\t\t %d ... %s ...\n",i_trac,chaine);
	/*fprintf(ftracech, "\n\t\t %d ... %s ...\n",i_trac,chaine);*/
	fprintf(ftraciv,  "\n\t\t %d ... %s ...\n",i_trac,chaine);
	fprintf(ftracav,  "\n\t\t %d ... %s ...\n",i_trac,chaine);
	i_trac++;

	fflush(ftrac);
	fflush(fichtra);
	fflush(ftracdata);
	/*fflush(ftracech);*/
	fflush(ftraciv);
	fflush(ftracav);

}

close_trace()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : close_trace
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: fermeture fichiers traces 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: fermeture fichiers traces. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	fclose(ftrac);
	fclose(fichtra);
	fclose(ftracdata);
	fclose(ftracech);
	fin_traceiss();
	fin_traceacq();
}

read_adress ()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : read_adress
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: lecture fichier server.cfg pr recuperer les 
#!#F#              adresses du service switch. 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: recuperation serveurs. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	FILE *fp ;
	int nbreserv,i ;
	char line[255];
	char code [3] , adr[5];
	int  Actif=-1;
	int  Val2;
	char Val1[2],Val3[10],Val4[5];
	char w_file[80];
	char S_acq[3];
	char S_iss[3];
	
	Ftimingtraces("debut read_adress \n",ftrac,0,40);

	strcpy(adress_switch_acq,"0000");
	strcpy(adress_switch_iss,"0000");

	switch(ModuleBaseI){
	case 'U': /* Module Ucs */
		strcpy(S_acq,ServA);
		strcpy(S_iss,ServI);
		strcpy(InterfaceBaseI,"UCS");

		if (strcmp(ServA,"00")){
			strcpy(ServerCode,ServA);
		}
		else{
			strcpy(ServerCode,ServI);
		}
		break;
	case 'V': /* Module Visa */
	default : /* Module Visa */
		strcpy(S_acq,ServA);
		strcpy(S_iss,ServI);
		strcpy(InterfaceBaseI,"VISA");

		if (strcmp(ServA,"00")){
			strcpy(ServerCode,ServA);
		}
		else{
			strcpy(ServerCode,ServI);
		}
		sprintf(text,"adresse serveur:%s\n",ServerCode);
		Ftimingtraces(text,fichtra,0,0);
		break;
	}

	sprintf(w_file,"%s/../../Switch/server.cfg",PathConf);
	if ((fp = fopen(w_file,"r")) == NULL)
		myexit("Error opening server.cfg");

	fgets(line,255,fp); /* 1ere ligne commentaire */
	fgets(line,255,fp); /* 2eme ligne commentaite */
	fgets(line,255,fp); /* 3eme ligne nombre de serveur */
	sscanf(line, "%*s%d",&nbreserv);
	for (i = 0;i < nbreserv;i++) {
		fgets(line,255,fp);
		sscanf(line,"%s%d%1s%d%s%s",code,&Actif,Val1,&Val2,Val3,adr);

		if (!strcmp(code,S_acq)) {
			if (!strcmp(adr,"0000")||(Actif==0)) {
				Ftimingtraces("adresse serveur non affectee\n",fichtra,0,10);
				sprintf(text,"adresse serveur actif:%d\n",Actif);
				Ftimingtraces(text,fichtra,0,10);
				continue;
			}
			strcpy(adress_switch_acq,adr);
			adress_switch_acq[4]='\0';
		}

		if (!strcmp(code,S_iss)) {
			if (!strcmp(adr,"0000")||(Actif==0)) {
				Ftimingtraces("adresse serveur non affectee\n",fichtra,0,10);
				sprintf(text,"adresse serveur actif:%d\n",Actif);
				Ftimingtraces(text,fichtra,0,10);
				continue;
			}
			strcpy(adress_switch_iss,adr);
			adress_switch_iss[4]='\0';
		}
	}
	fclose(fp);

	strncpy(adress_reseau,v_visaconf.vsc_sessionport,4);
	adress_reseau[5]='\0';
	Ftimingtraces("fin read_adress \n",ftrac,0,40);
}

main(argc,argv)
int argc;
char *argv[];
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : main
#!#F#              Type       : L 
#!#F#              Input parameters: <argc:nbr argument,argv:valeur argument>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: programme lancement application 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: programme lancement application. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int ret;
	int rep1=-1;
	int i;

	char argLue[20];
	char Cfgfile[80];
	char datetrc[9];/*--Rida 200209 gestion des traces--*/

	memset(argLue,'\0',19);
	memset(ServA,'\0',3);
	memset(ServI,'\0',3);
	memset(argCodbqe,'\0',6);
	if (!strcmp(argv[1],"-v")) {
		fprintf(stderr,"%s %s \n","Version :",VERSION_STRING);
		fflush(stderr);
		exit(0);
	}
	strcpy(argLue,argv[2]);
	ModuleBaseI=argLue[0];
	ModuleEnv=argLue[1];		/* Type d'environement */

	strncpy(ServA,argLue+2,2);  /* code Serveur Acq */
	ServA[2]=0;
	strncpy(ServI,argLue+4,2);  /* code Serveur ISS */
	ServI[2]=0;
	strncpy(argCodbqe,argLue+6,5); /*00000 cas VISA */
	argCodbqe[5] = 0;
	memset(datetrc,'\0',sizeof(datetrc));
	Get_date_trace(datetrc);/*--Rida 200209 gestion des traces--*/
	fprintf(stderr,"visa ---> ModuleBaseI:%c,ModuleEnv:%c\n",ModuleBaseI,ModuleEnv);
	fprintf(stderr,"visa ---> ServA:%s,ServI:%s\n",ServA,ServI);
	fflush(stderr);

	sprintf(PathTracesParent,"%s/MXTraces",argv[1]);
	
	if(ModuleBaseI=='V'){
		if(ModuleEnv=='P'){
			sprintf(PathConf,"%s/MXMOConf/Visa/Prod%s%s",argv[1],ServA,ServI);
			/*sprintf(PathTraces,"%s/MXMOTraces/Visa/Prod%s%s/%s",argv[1],ServA,ServI,datetrc); */ /*--Rida 200209 gestion des traces--*/
			sprintf(PathTraces,"%s/MXTraces/%s/Visa/Prod%s%s",argv[1],datetrc,ServA,ServI);
			sprintf(PathTracesEnv,"Visa");
			sprintf(PathTracesSuffix,"Prod%s%s",ServA,ServI);
		}
		else {
			sprintf(PathConf,"%s/MXMOConf/Visa/Test%s%s",argv[1],ServA,ServI);
			sprintf(PathTraces,"%s/MXTraces/%s/Visa/Test%s%s",argv[1],datetrc,ServA,ServI);
			sprintf(PathTracesEnv,"Visa");
			sprintf(PathTracesSuffix,"Test%s%s",ServA,ServI);
		}
	}
	else{
		sprintf(PathConf,"%s/MXMOConf/Ucs/Prod%s%s",argv[1],ServA,ServI);
		sprintf(PathTraces,"%s/MXTraces/%s/Ucs/Prod%s%s",argv[1],datetrc,ServA,ServI);/*--Rida 200209 gestion des traces--*/
		sprintf(PathTracesEnv,"Ucs");
		sprintf(PathTracesSuffix,"Prod%s%s",ServA,ServI);
	}
	memset(Cfgfile,'\0',80);
	sprintf(Cfgfile,"%s/MXUsers/mxswitch.dbu",argv[1]);

	if(db_connect_crypt("mxswitch","mxswitch",Cfgfile) != 0) {
        fprintf(stderr,"Cannot Connect to Database Switch\n");	
        fflush(stderr);
		myexit("\tvisa.x2:--> Connect Database Failed\n");
	}

	rep1= RecuperConfig(ModuleBaseI,ModuleEnv,ServA,ServI);
	init_variables();
	open_trace();
	/*open_crypt_cfg();*//* TEST cryptage*/
		memset(Cfgfile,'\0',80);
	sprintf(Cfgfile,"%s/MXMOConf/Switch/crypt.cfg",argv[1]);
		
		if ((fpc = fopen(Cfgfile,"r")) != NULL)
			fscanf(fpc,"%d %d",&test_crypt,&VAL_ASCII);
		fclose(fpc);
	
	init_variablesv();
	read_adress();
	insert_status("04",ServerCode,STARTPROC,NOTYET);
	gc();
	close_trace();
}

Tentative_Connect()
{
/*
#!#F#--------------------------------------------------------------------------
#!#F#              Function   : Tentative_Connect
#!#F#              Type       : L 
#!#F#              Input parameters: <Aucun>
#!#F#              Output parameters: <Aucun> 
#!#F#              Returned parameters: <Rien>
#!#F#              Description: lecture fichier server.cfg pr recuperer les 
#!#F#              adresses du service switch. 
#!#F#              Caller Modules: gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA				Date: 23.11.2006
#!#M# Release      Description: lecture fichier. 
#!#M#              Author     : A.HAIMOURA				Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :							Date:
#!#F#--------------------------------------------------------------------------
*/
	int nbre_tentative;
	int ret ;
	char CmdSignon[200];
	
	Ftimingtraces("debut Tentative_Connect \n",ftrac,0,40);

	if(v_com.type[0]=='C'){
		nbre_tentative = 1;
		if (v_com.typ_com == X25)
			while(nbre_tentative > 0) {
				ret=x25connect(v_com.addr,&(v_com.desc),v_com.Card,v_com.Line) ;
				
				sprintf(text,"tentative de conn X25 echoue,ret:%d\n",ret);
				Ftimingtraces(text,fichtra,0,10);

				if (ret == 0)
					break;
				nbre_tentative--;
				sleep(v_com.TimeOut);
			}
		if (v_com.typ_com == TCP)
			while(nbre_tentative > 0) {
				ret=Connect_host(v_com.addr,&(v_com.desc), v_com.HostName);

				sprintf(text,"tentative de conn tcp echoue,ret:%d\n",ret);
				Ftimingtraces(text,fichtra,0,10);

				if (ret == 0)
					break;
				nbre_tentative--;
				sleep(v_com.TimeOut);
			}
		if (ret < 0) {
			v_com.desc = -1;
			TimeOutPoll = v_com.TimeOut * 1000;
		}
		else{
			TimeOutPoll = -1;
			Update_Rout("04",ServA,ServI,'1');
		}
		fds[2].fd = v_com.desc;
		fds[2].revents = 0;
		insert_status("04",ServerCode,ret,netwcode);
		if(v_com.desc > 0) {
			memset(CmdSignon,0,sizeof(CmdSignon));
			sprintf(CmdSignon,
			    "libpos.x %s localhost ../MXMOConf/Switch/Visa_Signon 2>Visa.log &",			adress_reseau);
			system(CmdSignon);
		}
	}
	Ftimingtraces("fin Tentative_Connect \n",ftrac,0,40);
}



int Get_date_trace(char *trcdate)
{
/*
#!#F#--------------------------------------------------------------------
#!#F#              Function   : Get_date_trace
#!#F#              Type       : L
#!#F#              Input parameters: <chaine a remplir>
#!#F#              Output parameters: <Aucun>
#!#F#              Returned parameters: <Rien>
#!#F#              Caller Modules:gcsupervisor.c
#!#F# Write        Author     : A.HAIMOURA        Date: 23.11.2006
#!#M# Release      Description: renvoi la date systeme
#!#M#              Author     : A.HAIMOURA        Date :23.11.2006
#!#F#              Description:
#!#F#              Author     :             Date:
#!#F#--------------------------------------------------------------------
*/
  struct timeb tp;
  struct tm date;
  char datef[7];
 
  ftime(&tp);
  memcpy (&date, localtime(&tp.time), sizeof(struct tm));
  date.tm_year %=100;
  sprintf(trcdate,"%02d%02d%02d",date.tm_mday,date.tm_mon+1,date.tm_year);
  return(0);
}



int get_champs_casc(msg_input,longueur,pos,val_champs)
char * msg_input;
int pos,longueur;
char * val_champs; 
{ 
	int lg,lg_ch,i;
	char ch_lg[4],code[4],ch_code[4];
	
	sprintf(code,"%03d",pos);
        code[3]='\0';
	lg = 0;
	while(lg<longueur-6) 
		{	memcpy(ch_code,msg_input+lg,3);
			if (strncmp(code,ch_code,3)==0)
			 {	
			 	memcpy(ch_lg,msg_input+lg+3,3);
				ch_lg[3] = '\0';
				
				if(strlen(ch_lg)<3)
					return(-1);
				for (i=0;(i<3)&& isdigit(ch_lg[i]);i++){}
					if(i<2)
						return(-1);
				
				lg_ch = atoi(ch_lg);
				if(lg_ch>100)
					return(-1);
				if(lg_ch> longueur-(lg+6))
					return(-1);

				memcpy(val_champs,msg_input+lg+6,lg_ch);
				val_champs[lg_ch] = '\0';
					lg +=7;
		    	return(lg);
				
			 }
			else{	
			  memcpy(ch_lg,msg_input+lg+3,3);
				ch_lg[3] = '\0';
				
				if(strlen(ch_lg)<3)
					return(-1);
				for (i=0;(i<3)&& isdigit(ch_lg[i]);i++){}
					if(i<2)
						return(-1);

				lg_ch=atoi(ch_lg);
				if(lg_ch>100)
					return(-1);
				if(lg_ch> longueur-(lg+6))
					return(-1);
						lg=lg+6+lg_ch;	 
		   } 
          	
		}
	return(0);
}

/**********************************FIN*****************************************/
