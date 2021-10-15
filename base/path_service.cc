#include "base/path_service.h"

#include <unordered_map>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "build/build_config.h"

namespace base {

bool PathProvider(int key, FilePath* result);

#if defined(OS_WIN)
bool PathProviderWin(int key, FilePath* result);
#endif

namespace {

typedef std::unordered_map<int, FilePath> PathMap;

// We keep a linked list of providers.  In a debug build we ensure that no two
// providers claim overlapping keys.
struct Provider {
  PathService::ProviderFunc func;
  struct Provider* next;
#ifndef NDEBUG
  int key_start;
  int key_end;
#endif
  bool is_static;
};

Provider base_provider = {PathProvider, nullptr,
#ifndef NDEBUG
                          PATH_START, PATH_END,
#endif
                          true};

#if defined(OS_WIN)
Provider base_provider_win = {PathProviderWin, &base_provider,
#ifndef NDEBUG
                              PATH_WIN_START, PATH_WIN_END,
#endif
                              true};
#endif

struct PathData {
  Lock lock;
  PathMap cache;        // Cache mappings from path key to path value.
  PathMap overrides;    // Track path overrides.
  Provider* providers;  // Linked list of path service providers.
  bool cache_disabled;  // Don't use cache if true;

  PathData() : cache_disabled(false) {
#if defined(OS_WIN)
    providers = &base_provider_win;
#elif defined(OS_APPLE)
    providers = &base_provider_mac;
#elif defined(OS_ANDROID)
    providers = &base_provider_android;
#elif defined(OS_FUCHSIA)
    providers = &base_provider_fuchsia;
#elif defined(OS_POSIX)
    providers = &base_provider_posix;
#endif
  }
};

static PathData* GetPathData() {
  static auto* path_data = new PathData();
  return path_data;
}

// Tries to find |key| in the cache.
bool LockedGetFromCache(int key, const PathData* path_data, FilePath* result)
    EXCLUSIVE_LOCKS_REQUIRED(path_data->lock) {
  if (path_data->cache_disabled)
    return false;
  // check for a cached version
  auto it = path_data->cache.find(key);
  if (it != path_data->cache.end()) {
    *result = it->second;
    return true;
  }
  return false;
}

// Tries to find |key| in the overrides map.
bool LockedGetFromOverrides(int key, PathData* path_data, FilePath* result)
    EXCLUSIVE_LOCKS_REQUIRED(path_data->lock) {
  // check for an overridden version.
  PathMap::const_iterator it = path_data->overrides.find(key);
  if (it != path_data->overrides.end()) {
    if (!path_data->cache_disabled)
      path_data->cache[key] = it->second;
    *result = it->second;
    return true;
  }
  return false;
}

}  // namespace

// TODO(brettw): this function does not handle long paths (filename > MAX_PATH)
// characters). This isn't supported very well by Windows right now, so it is
// moot, but we should keep this in mind for the future.
// static
bool PathService::Get(int key, FilePath* result) {
  PathData* path_data = GetPathData();
  DCHECK(path_data);
  DCHECK(result);
  DCHECK_GE(key, DIR_CURRENT);

  // special case the current directory because it can never be cached
  if (key == DIR_CURRENT)
    return GetCurrentDirectory(result);

  Provider* provider = nullptr;
  {
    AutoLock scoped_lock(path_data->lock);
    if (LockedGetFromCache(key, path_data, result))
      return true;

    if (LockedGetFromOverrides(key, path_data, result))
      return true;

    // Get the beginning of the list while it is still locked.
    provider = path_data->providers;
  }

  FilePath path;

  // Iterating does not need the lock because only the list head might be
  // modified on another thread.
  while (provider) {
    if (provider->func(key, &path))
      break;
    DCHECK(path.empty()) << "provider should not have modified path";
    provider = provider->next;
  }

  if (path.empty())
    return false;

  if (path.ReferencesParent()) {
    // Make sure path service never returns a path with ".." in it.
    path = MakeAbsoluteFilePath(path);
    if (path.empty())
      return false;
  }
  *result = path;

  AutoLock scoped_lock(path_data->lock);
  if (!path_data->cache_disabled)
    path_data->cache[key] = path;

  return true;
}

}  // namespace base
