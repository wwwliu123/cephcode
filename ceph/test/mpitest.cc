

#include <sys/stat.h>
#include <iostream>
#include <string>
using namespace std;

#include "mds/MDCluster.h"
#include "mds/MDS.h"
#include "osd/OSD.h"
#include "fakeclient/FakeClient.h"

#include "mds/MDCache.h"
#include "mds/MDStore.h"

#include "msg/MPIMessenger.h"

#include "messages/MPing.h"


__uint64_t ino = 1;



#include "include/config.h"
#define NUMMDS g_conf.num_mds
#define NUMOSD g_conf.num_osd
#define NUMCLIENT g_conf.num_client

// this parses find output
int play();

int main(int argc, char **argv) {
  cout << "mpitest starting" << endl;

  int myrank = mpimessenger_init(argc, argv);
  int world = mpimessenger_world();

  MDCluster *mdc = new MDCluster(NUMMDS, NUMOSD);
  
  // create mds
  MDS *mds[NUMMDS];
  for (int i=0; i<NUMMDS; i++) {
	if (myrank != MPI_DEST_TO_RANK(MSG_ADDR_MDS(i),world)) continue;
	mds[i] = new MDS(mdc, i, new MPIMessenger(MSG_ADDR_MDS(i)));
	mds[i]->init();
  }
  
  // create osds
  OSD *osd[NUMOSD];
  for (int i=0; i<NUMOSD; i++) {
	if (myrank != MPI_DEST_TO_RANK(MSG_ADDR_OSD(i),world)) continue;
	osd[i] = new OSD(i, new MPIMessenger(MSG_ADDR_OSD(i)));
	osd[i]->init();
  }
  
  // create clients
  FakeClient *client[NUMCLIENT];
  for (int i=0; i<NUMCLIENT; i++) {
	if (myrank != MPI_DEST_TO_RANK(MSG_ADDR_CLIENT(i),world)) continue;
	client[i] = new FakeClient(mdc, i, new MPIMessenger(MSG_ADDR_CLIENT(i)), g_conf.client_requests);
	client[i]->init();
  }
  
  // seed initial requests
  for (int i=0; i<NUMCLIENT; i++) {
	if (myrank != MPI_DEST_TO_RANK(MSG_ADDR_CLIENT(i),world)) continue;
	client[i]->issue_request();
  }

  // start message loop
  mpimessenger_start();

  // wait for it to finish
  mpimessenger_wait();

  // 
  cout << "---- check ----" << endl;
  for (int i=0; i<NUMMDS; i++) {
	if (myrank != MPI_DEST_TO_RANK(MSG_ADDR_MDS(i),world)) continue;
	mds[i]->mdcache->shutdown_pass();
  }
  
  // cleanup
  cout << "cleanup" << endl;
  for (int i=0; i<NUMMDS; i++) {
	if (myrank != MPI_DEST_TO_RANK(MSG_ADDR_MDS(i),world)) continue;
	if (mds[i]->shutdown_final() == 0) {
	  //cout << "clean shutdown of mds " << i << endl;
	  delete mds[i];
	} else {
	  cout << "problems shutting down mds " << i << endl;
	}
  }
  for (int i=0; i<NUMOSD; i++) {
	if (myrank != MPI_DEST_TO_RANK(MSG_ADDR_OSD(i),world)) continue;
	if (osd[i]->shutdown() == 0) { 
	  //cout << "clean shutdown of osd " << i << endl;
	  delete osd[i];
	} else {
	  cout << "problems shutting down osd " << i << endl;
	}
  }
  for (int i=0; i<NUMCLIENT; i++) {
	if (myrank != MPI_DEST_TO_RANK(MSG_ADDR_CLIENT(i),world)) continue;
	if (client[i]->shutdown() == 0) { 
	  //cout << "clean shutdown of client " << i << endl;
	  delete client[i];
	} else {
	  cout << "problems shutting down client " << i << endl;
	}
  }
  delete mdc;
  
  cout << "done." << endl;
  return 0;
}

