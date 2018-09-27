#include "zookeeper.h"
#include <proto.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <set>
#include <string>

#include <omp.h>

#include "cass.h"

typedef struct String_vector zoo_string;

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#include <sys/select.h>
#else
#include "winport.h"
//#include <io.h> <-- can't include, conflicting definitions of close()
int read(int _FileHandle, void * _DstBuf, unsigned int _MaxCharCount);
int write(int _Filehandle, const void * _Buf, unsigned int _MaxCharCount);
#define ctime_r(tctime, buffer) ctime_s (buffer, 40, tctime)
#endif

#include <time.h>
#include <errno.h>
#include <assert.h>

#define _LL_CAST_ (long long)

std::set<std::string> status_done;

int WORKER_ID = 0;

int verbose = 0;
char* hostPort;

static zhandle_t *zh;
static clientid_t myid;
static const char *clientIdFile = 0;
struct timeval startTime;
static char cmd[1024];
static int batchMode=0;

static int to_send=0;
static int sent=0;
static int recvd=0;

static int shutdownThisThing=0;

static __attribute__ ((unused)) void 
printProfileInfo(struct timeval start, struct timeval end, int thres,
                 const char* msg)
{
  int delay=(end.tv_sec*1000+end.tv_usec/1000)-
    (start.tv_sec*1000+start.tv_usec/1000);
  if(delay>thres)
    fprintf(stderr,"%s: execution time=%dms\n",msg,delay);
}

static const char* state2String(int state){
  if (state == 0)
    return "CLOSED_STATE";
  if (state == ZOO_CONNECTING_STATE)
    return "CONNECTING_STATE";
  if (state == ZOO_ASSOCIATING_STATE)
    return "ASSOCIATING_STATE";
  if (state == ZOO_CONNECTED_STATE)
    return "CONNECTED_STATE";
  if (state == ZOO_EXPIRED_SESSION_STATE)
    return "EXPIRED_SESSION_STATE";
  if (state == ZOO_AUTH_FAILED_STATE)
    return "AUTH_FAILED_STATE";

  return "INVALID_STATE";
}

static const char* type2String(int state){
  if (state == ZOO_CREATED_EVENT)
    return "CREATED_EVENT";
  if (state == ZOO_DELETED_EVENT)
    return "DELETED_EVENT";
  if (state == ZOO_CHANGED_EVENT)
    return "CHANGED_EVENT";
  if (state == ZOO_CHILD_EVENT)
    return "CHILD_EVENT";
  if (state == ZOO_SESSION_EVENT)
    return "SESSION_EVENT";
  if (state == ZOO_NOTWATCHING_EVENT)
    return "NOTWATCHING_EVENT";

  return "UNKNOWN_EVENT_TYPE";
}

void watcher(zhandle_t *zzh, int type, int state, const char *path,
             void* context)
{
    /* Be careful using zh here rather than zzh - as this may be mt code
     * the client lib may call the watcher before zookeeper_init returns */

    fprintf(stderr, "Watcher %s state = %s", type2String(type), state2String(state));
    if (path && strlen(path) > 0) {
      fprintf(stderr, " for path %s", path);
    }
    fprintf(stderr, "\n");

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            const clientid_t *id = zoo_client_id(zzh);
            if (myid.client_id == 0 || myid.client_id != id->client_id) {
                myid = *id;
                fprintf(stderr, "Got a new session id: 0x%llx\n",
                        _LL_CAST_ myid.client_id);
                if (clientIdFile) {
                    FILE *fh = fopen(clientIdFile, "w");
                    if (!fh) {
                        perror(clientIdFile);
                    } else {
                        int rc = fwrite(&myid, sizeof(myid), 1, fh);
                        if (rc != sizeof(myid)) {
                            perror("writing client id");
                        }
                        fclose(fh);
                    }
                }
            }
        } else if (state == ZOO_AUTH_FAILED_STATE) {
            fprintf(stderr, "Authentication failure. Shutting down...\n");
            zookeeper_close(zzh);
            shutdownThisThing=1;
            zh=0;
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            fprintf(stderr, "Session expired. Shutting down...\n");
            zookeeper_close(zzh);
            shutdownThisThing=1;
            zh=0;
        }
    }
}

void test_set();
void get_completion(int rc, const char *value, int value_len,
        const struct Stat *stat, const void *data) {

	char v[value_len];
	memcpy(v, value, value_len);

	//printf("GOT VALUE : %s\n", v);
	free((void*)data);
}

void test_get()
{
	int rc;
	char* line = "/lulu";
        rc = zoo_aget(zh, line, 1, get_completion, strdup(line));
}

void set_completion(int rc, const struct Stat *stat, const void *data)
{
	//printf("SET value callback succesfull.\n");
    	free((void*)data);
}

void test_set()
{
	char* line = "/lulu";
	char* ptr = "ioanna";
        int rc = zoo_aset(zh, line, ptr, strlen(ptr), -1, set_completion,
                    strdup(line));
}

void create_completion(int rc,  const char * val, const void * data)
{
	//printf("Create callback called!\n");
}

void create_znode(char* name)
{
        char* ptr = "stefan";
	zoo_acreate(zh, name, ptr, strlen(ptr), &ZOO_OPEN_ACL_UNSAFE, 0, create_completion, strdup(name));
}

void tasks_watcher (zhandle_t *zh,
                    int type,
                    int state,
                    const char *path,
                    void *watcherCtx) {
    printf("Tasks watcher triggered %s %d", path, state);
/*    if( type == ZOO_CHILD_EVENT) {
        assert( !strcmp(path, "/tasks") );
        get_tasks();
    } else {
        LOG_INFO(("Watched event: %s", type2string(type)));
    }
    LOG_DEBUG(("Tasks watcher done"));
*/
}

/*
void delete_task_completion(int rc, const void *data) {
	if (rc == ZOK)
	{
		free((char *) data);
	}
}

void delete_assignment(char* name)
{
    printf("ASSIGNMENT IS DELETED");
    char * tmp_path = strdup(name);
    zoo_adelete(zh,
                tmp_path,
                -1,
                delete_task_completion,
		(const void*) tmp_path);
}

void process_task(char* task_name)
{
	// crop the file name : is after the last dash

	// start a new thread?
	// re-key file (get metadata, data from cassandra, use hardcoded old, new gk)
	printf("Re-encryption of File Name : %s\n", task_name);
	usleep(3 * 1000 * 1000);

	// create status znode, so the admin knows that the task is done
	
	// delete assignment znode, so that master does not re-assign the task
	delete_assignment(name);
}

void tasks_completion (int rc,
                       const struct String_vector *strings,
			const void *data) {
	if (rc == ZOK)
	{
		int i;
 		for( i = 0; i < strings->count; i++) {
        		printf("RETREIVED task %s", (char *) strings->data[i]);
			process_task();
		}
	}
}

void watch_znode(char* name)
{
	zoo_awget_children(zh,
                       name,
                       tasks_watcher,
                       NULL,
                       tasks_completion,
			NULL);
}
*/

int total_tasks = 0;
void create_tasks()
{
	// TODO : query cassandra metadata and get all the file names
	Cassandra::Init();
	std::vector<std::string> f = Cassandra::get_all_files();
	//printf("Found %d files in DB.\n", f.size());

	srand(time(NULL));
	total_tasks = f.size();
	for(int i=0; i<total_tasks; i++)
	{
		char task_name[32];

		std::size_t found = f[i].find(".dat");
		
		std::string s = f[i].substr(0, found + 4);
		//printf("Found file : %s\n", s.c_str());

		snprintf(task_name, 32, "/tasks/task-%s\0", s.c_str());
		//printf("Creating task : %s\n", task_name);
//		create_znode(task_name);
	}

	Cassandra::Bye();
}

void create_batches(int batch_count)
{
        // TODO : query cassandra metadata and get all the file names
        Cassandra::Init();
        std::vector<std::string> f = Cassandra::get_all_files();

        // clear batches in DB
        Cassandra::ClearPartitions();

        // split the files in configured batches
        printf("TOTAL FILES : %d\n", f.size());
        int batch_size = f.size() / batch_count;
        printf("BATCH SIZE : %d\n", batch_size);
	total_tasks = batch_count;
	
        for (int i=0; i<batch_count; i++)
        {
                std::vector<std::string> batch;
                for (int j=0; j<batch_size; j++)
                {
                        int index = (i * batch_size) + j;
                        if (index < f.size())
                                batch.push_back(f[index]);
                }

                printf("Creating batch ... %d, size %d\n", i, batch.size());

                // save batch to DB
                Cassandra::SavePartition(i, batch);

                // push batch to cassandra
                char task_name[32];
                snprintf(task_name, 32, "/tasks/task-%d\0", i);
                printf("Creating task : %s\n", task_name);
                create_znode(task_name);
        }

        Cassandra::Bye();
        printf("BATCHES CREATED\n");
}


void status_watcher (zhandle_t *zh,
                    int type,
                    int state,
                    const char *path,
                    void *watcherCtx) {
}

void status_completion (int rc,
                       const struct String_vector *strings,
                        const void *data) {
        if (rc == ZOK)
        {
                int i;
                for( i = 0; i < strings->count; i++) {
			std::string s((char*) strings->data[i]);
			status_done.insert(s);
                }
        }
}


int watch_tasks_status()
{
	char* status_znode = "/status";
        zoo_awget_children(zh,
                       status_znode,
                       status_watcher,
                       NULL,
                       status_completion,
                        NULL);
	return 0;
}

int main(int argc, char **argv) {


#ifndef THREADED
    fd_set rfds, wfds, efds;
    int processed=0;
#endif
    char buffer[4096];
    char p[2048];
    int bufoff = 0;
    FILE *fh;

    if (argc < 2) {
        fprintf(stderr,
                "USAGE %s zookeeper_host_list [clientid_file|cmd:(ls|ls2|create|od|...)]\n", 
                argv[0]);
        fprintf(stderr,
                "Version: ZooKeeper cli (c client) version %d.%d.%d\n", 
                ZOO_MAJOR_VERSION,
                ZOO_MINOR_VERSION,
                ZOO_PATCH_VERSION);
        return 2;
    }
    if (argc > 2) {
      if(strncmp("cmd:",argv[2],4)==0){
        size_t cmdlen = strlen(argv[2]);
        if (cmdlen > sizeof(cmd)) {
          fprintf(stderr,
                  "Command length %zu exceeds max length of %zu\n",
                  cmdlen,
                  sizeof(cmd));
          return 2;
        }
        strncpy(cmd, argv[2]+4, sizeof(cmd));
        batchMode=1;
        fprintf(stderr,"Batch mode: %s\n",cmd);
      }else{
        clientIdFile = argv[2];
        fh = fopen(clientIdFile, "r");
        if (fh) {
            if (fread(&myid, sizeof(myid), 1, fh) != sizeof(myid)) {
                memset(&myid, 0, sizeof(myid));
            }
            fclose(fh);
        }
      }
    }

    strcpy(p, "dummy");
    verbose = 0;
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
    zoo_deterministic_conn_order(1); // enable deterministic order
    hostPort = argv[1];
    zh = zookeeper_init(hostPort, watcher, 30000, &myid, 0, 0);
    if (!zh) {
        return errno;
    }

	double startTime = omp_get_wtime();

    int CLIENT_WAKE_UP = 1;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    while (!shutdownThisThing) {
        int events;
        int fd;
        int interest;
        struct timeval tv;
        int rc;
        zookeeper_interest(zh, &fd, &interest, &tv);

        if (fd != -1) {
            if (interest&ZOOKEEPER_READ) {
                FD_SET(fd, &rfds);
            } else {
                FD_CLR(fd, &rfds);
            }
            if (interest&ZOOKEEPER_WRITE) {
                FD_SET(fd, &wfds);
            } else {
                FD_CLR(fd, &wfds);
            }
        } else {
            fd = 0;
        }
        FD_SET(0, &rfds);
        rc = select(fd+1, &rfds, &wfds, &efds, &tv);
        events = 0;
        if (rc > 0) {
            if (FD_ISSET(fd, &rfds)) {
                events |= ZOOKEEPER_READ;
            }
            if (FD_ISSET(fd, &wfds)) {
                events |= ZOOKEEPER_WRITE;
            }
        }

	// once executed
	if (CLIENT_WAKE_UP)
	{
		CLIENT_WAKE_UP = 0;
		//create_tasks();
		create_batches(100);
	}

	// sleep 100 miliseconds
	usleep(100 * 1000);

	// wath the status of tasks reported by workers
	printf("Checking the status. Done: %d out of %d...\n", status_done.size(), total_tasks);
	watch_tasks_status();
	if (status_done.size() == total_tasks)
	{
//		printf("TASKS WERE FINISHED !\n");
		break;
	}

        zookeeper_process(zh, events);
    }

	double stopTime = omp_get_wtime();
	double secsElapsed = stopTime - startTime;
//	printf("ADMIN time : %f\n", secsElapsed);
	printf("%f", secsElapsed);


    if (to_send!=0)
        fprintf(stderr,"Recvd %d responses for %d requests sent\n",recvd,sent);
    zookeeper_close(zh);
    return 0;
}
