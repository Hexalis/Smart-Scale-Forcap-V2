#pragma once
#include <Arduino.h>

// Initialize FS + mutex. Creates the spool file if needed.
bool spool_init();

// Append one measurement (timestamp + diff in grams).
bool spool_enqueue(uint32_t epoch, float diff);

// Try to upload queued measurements using the provided callback.
// The callback should return true on successful POST.
// Returns the number of successfully uploaded entries.
typedef bool (*spool_post_fn)(uint32_t ts, float diff);
size_t spool_flush(spool_post_fn post);

// How many entries are queued (approx; O(n))
size_t spool_count();
