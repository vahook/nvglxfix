#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <xcb/xcb.h>

#define NUM_THREADS 8
#define NUM_CLIENTS 4  // per thread
#define NUM_EVENTS  50 // per client

static int run = 0;
static void* spam_thread(void* arg)
{
  int tid = (int)(uintptr_t)arg;

  // Establish the connections
  xcb_connection_t* cs[NUM_CLIENTS];
  for (int cid = 0; cid < NUM_CLIENTS; ++cid) {
    cs[cid] = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(cs[cid])) {
      fprintf(stderr, "[%d] Error: couldn't connect to X.\n", tid);
      while (--cid >= 0)
        xcb_disconnect(cs[cid]);
      return NULL;
    }
  }

  // Do the spamming
  fprintf(stderr, "[%d] Info: started spamming X.\n", tid);
  xcb_get_input_focus_cookie_t ops[NUM_EVENTS];
  while (run) {
    for (int cid = 0; cid < NUM_CLIENTS; ++cid) {
      xcb_connection_t* c = cs[cid];
      for (int oid = 0; oid < NUM_EVENTS; ++oid)
        ops[oid] = xcb_get_input_focus(c);
      xcb_flush(c);
      for (int oid = 0; oid < NUM_EVENTS; ++oid)
        free(xcb_get_input_focus_reply(c, ops[oid], NULL));
    }
  }

  // Close the connections
  fprintf(stderr, "[%d] Info: shutting down X.\n", tid);
  for (int cid = 0; cid < NUM_CLIENTS; ++cid)
    xcb_disconnect(cs[cid]);
  return NULL;
}

int main()
{
  // Start spamming threads
  run = 1;
  pthread_t threads[NUM_THREADS];
  for (int tid = 0; tid < NUM_THREADS; ++tid)
    pthread_create(&threads[tid], NULL, &spam_thread, (void*)(uintptr_t)tid);

  // Wait for input
  fprintf(stderr, "Started X spam threads. Press any key to stop.\n");
  getchar();

  // Shut down the threads
  run = 0;
  for (int tid = 0; tid < NUM_THREADS; ++tid)
    pthread_join(threads[tid], NULL);

  return 0;
}