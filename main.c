#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h> /* PATH_MAX */
#include <unistd.h>       /* getpid() */
#include <xcb/xcb.h>
#include <xcb/sync.h>

#ifndef NVGLXFIX_PRINT_DEBUG_LOGS
#define NVGLXFIX_PRINT_DEBUG_LOGS 1
#endif

#if NVGLXFIX_PRINT_DEBUG_LOGS
#define NVGLXFIX_LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define NVGLXFIX_LOG(...)
#endif

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// Not too pretty, but libGLX_nvidia dynamically resolves the xcb functions
// that it uses for presenting, which means we have to intercept dlsym.
static void* (*real_dlsym)(void* __restrict, const char* __restrict) = NULL;
__attribute__((constructor)) void nvglxfix_init()
{
  // Resolve the real dlsym
  real_dlsym = dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.2.5");
  NVGLXFIX_LOG("Loaded into process (pid: %d, dlsym: %p)\n", getpid(), real_dlsym);
}

// TODO: can multithreading be an issue here? / should this be thread / xcb_connection_t local?
static xcb_get_input_focus_cookie_t sync_cookie = {};

static xcb_void_cookie_t (*real_xcb_sync_reset_fence)(xcb_connection_t*, xcb_sync_fence_t) = NULL;
static xcb_void_cookie_t xcb_sync_reset_fence_hooked(xcb_connection_t* c, xcb_sync_fence_t fence)
{
  // Issue the reset. If nvidia didn't change the driver around too much, we should be right at the
  // start of the presentation logic.
  xcb_void_cookie_t result = real_xcb_sync_reset_fence(c, fence);

  // Sanity check
  if (unlikely(sync_cookie.sequence))
    NVGLXFIX_LOG("Found unawaited sync_cookie: %d\n", sync_cookie.sequence);

  // Force the server to send us back something that we can wait for. (By the way, XSync() also does
  // exactly this.) This is neccesarry, because none of the xcb commands in the presentation logic
  // generate an immediate reply.
  sync_cookie = xcb_get_input_focus(c);

  return result;
}

static int (*real_xcb_flush)(xcb_connection_t*) = NULL;
static int xcb_flush_hooked(xcb_connection_t* c)
{
  // Call the real flush.
  int result = real_xcb_flush(c);

  // Now let's wait for xcb_sync_reset_fence() to execute. If we have a valid "sync cookie", we
  // are basically guaranteed to be inside the presentation logic, as both the
  // xcb_sync_reset_fence() and xcb_flush() calls happen in the same function.
  if (likely((result > 0) && sync_cookie.sequence)) {
    free(xcb_get_input_focus_reply(c, sync_cookie, NULL));
    sync_cookie.sequence = 0;
  }

  return result;
}

static uintptr_t nvlibglx_start = -1, nvlibglx_end = 0;
static int is_inside_nvlibglx(void* addr)
{
  // Look through our mapped memory regions, and look for libGLX_nvidia.
  // Yeah, this is neither thread safe nor pretty, but it should get the job done.
  if (nvlibglx_start == -1) {
    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps)
      return 0;

    char buffer[128 + PATH_MAX];
    while (fgets(buffer, sizeof(buffer), maps)) {
      char perms[5];
      uintptr_t addr_start, addr_end;
      unsigned long long file_offset, inode;
      unsigned dev_major, dev_minor;
      int path_pos;
      if (sscanf(
            buffer, "%lx-%lx %4s %llx %x:%x %llu%n", &addr_start, &addr_end, perms, &file_offset,
            &dev_major, &dev_minor, &inode, &path_pos
          ) < 7)
        continue;

      // Look for the nvidia GLX library and cache the region for later use.
      if (strstr(&buffer[path_pos], "libGLX_nvidia.so")) {
        nvlibglx_start = addr_start < nvlibglx_start ? addr_start : nvlibglx_start;
        nvlibglx_end = addr_end > nvlibglx_end ? addr_end : nvlibglx_end;
      }
    }
    fclose(maps);
    NVGLXFIX_LOG(
      "Found libGLX_nvidia.so in pid (%d): %p-%p\n", getpid(), (void*)nvlibglx_start,
      (void*)nvlibglx_end
    );
  }
  return (nvlibglx_start <= (uintptr_t)addr && (uintptr_t)addr < nvlibglx_end);
}

void* dlsym(void* __restrict handle, const char* __restrict name)
{
  // Somethimes (for some reason) it's possible, that someone (mostly proton / wine seems to do
  // this) loads us without calling the constructor, hence this explicit call here:
  if (unlikely(!real_dlsym))
    nvglxfix_init();

  // Call the real dlsym.
  void* result = real_dlsym(handle, name);

  // Try to minimize the impact and only hook the functions only for the nvidia driver.
  // From what I've gathered, libGLX_nvidia uses the dlsym'd xcb_flush() (at least on my setup):
  //  1) when creating the dri3 pixmap
  //  2) when presenting the pixmap, right after issuing xcb_present_pixmap(_synced)()
  // The first case only runs once per pixmap, so it's not a big deal if we do
  // an extra synchronization there.
  //
  // The dlsym'd version of xcb_sync_reset_fence() seems to be used only by the presentation logic,
  // so no issues there.
  void* retaddr = __builtin_return_address(0);
  if (unlikely(!strcmp(name, "xcb_flush"))) {
    if (is_inside_nvlibglx(retaddr)) {
      real_xcb_flush = result;
      NVGLXFIX_LOG("Hooked xcb_flush (caller: %p)\n", retaddr);
      return &xcb_flush_hooked;
    }
  } else if (unlikely(!strcmp(name, "xcb_sync_reset_fence"))) {
    if (is_inside_nvlibglx(retaddr)) {
      real_xcb_sync_reset_fence = result;
      NVGLXFIX_LOG("Hooked xcb_sync_reset_fence (caller: %p)\n", retaddr);
      return &xcb_sync_reset_fence_hooked;
    }
  }

  return result;
}