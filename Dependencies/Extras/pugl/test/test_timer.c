// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests that update events are received and than redisplays they trigger happen
  immediately in the same event loop iteration.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define NUM_TIMERS 4U // NOLINT(modernize-macro-to-enum)

#ifdef __APPLE__
static const double timeout = 1 / 60.0;
#else
static const double timeout = -1.0;
#endif

// Windows SetTimer has a maximum resolution of 10ms
static const double tolerance = 0.015;

typedef enum {
  START,
  EXPOSED,
} State;

typedef struct {
  size_t numAlarms;
  double firstAlarmTime;
  double lastAlarmTime;
} TimerStats;

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  TimerStats      stats[NUM_TIMERS];
  State           state;
} PuglTest;

static double
timerPeriod(const uintptr_t timerId)
{
  return 1.0 / (60.0 / ((double)timerId + 1.0));
}

static void
resetStats(PuglTest* const test)
{
  for (unsigned i = 0U; i < NUM_TIMERS; ++i) {
    test->stats[i].numAlarms      = 0;
    test->stats[i].firstAlarmTime = 0.0;
    test->stats[i].lastAlarmTime  = 0.0;
  }
}

static void
onTimer(PuglView* const view, const PuglTimerEvent* const event)
{
  assert(event->id < NUM_TIMERS);

  PuglTest* const   test  = (PuglTest*)puglGetHandle(view);
  TimerStats* const stats = &test->stats[event->id];

  stats->lastAlarmTime = puglGetTime(puglGetWorld(view));
  if (++stats->numAlarms == 1U) {
    stats->firstAlarmTime = stats->lastAlarmTime;
  }
}

static PuglStatus
onEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  switch (event->type) {
  case PUGL_EXPOSE:
    test->state = EXPOSED;
    break;

  case PUGL_TIMER:
    onTimer(view, &event->timer);
    break;

  default:
    break;
  }

  return PUGL_SUCCESS;
}

static double
roundPeriod(const double period)
{
  return floor(period * 1000.0) / 1000.0; // Round down to milliseconds
}

static double
averagePeriod(const TimerStats* const stats)
{
  if (stats->numAlarms < 2U) {
    return (double)INFINITY;
  }

  const double duration = stats->lastAlarmTime - stats->firstAlarmTime;

  return roundPeriod(duration / (double)(stats->numAlarms - 1U));
}

static void
checkTimerPeriod(const TimerStats* const stats, const double expectedPeriod)
{
  const double expected   = roundPeriod(expectedPeriod);
  const double actual     = averagePeriod(stats);
  const double difference = fabs(actual - expected);

  if (difference > tolerance) {
    fprintf(stderr, "error: Period not within %f of %f\n", tolerance, expected);
    fprintf(stderr, "note: Actual period %f\n", actual);
  }

  assert(difference <= tolerance);
}

int
main(int argc, char** argv)
{
  const double updateDuration   = 0.6;
  const double otherTimerPeriod = timerPeriod(0) * 3.0;

  PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
                   NULL,
                   puglParseTestOptions(&argc, &argv),
                   {{0, 0.0, 0.0}, {0, 0.0, 0.0}, {0, 0.0, 0.0}},
                   START};

  // Set up view
  test.view = puglNewView(test.world);
  puglSetWorldString(test.world, PUGL_CLASS_NAME, "PuglTest");
  puglSetViewString(test.view, PUGL_WINDOW_TITLE, "Pugl Timer Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(test.view, 896, 384);

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view, PUGL_SHOW_RAISE));
  while (test.state != EXPOSED) {
    assert(!puglUpdate(test.world, timeout));
  }

  // Register timers with different periods
  for (unsigned i = 0U; i < NUM_TIMERS; ++i) {
    assert(!puglStartTimer(test.view, i, timerPeriod(i)));
  }

  // Check that stopping an unknown timer fails gracefully
  assert(!!puglStopTimer(test.view, NUM_TIMERS));

  // Run and check that each timer fired and has the expected period
  assert(!puglUpdate(test.world, updateDuration));
  for (unsigned i = 0U; i < NUM_TIMERS; ++i) {
    checkTimerPeriod(&test.stats[i], timerPeriod(i));
  }

  // Stop the first timer and change the frequency of the last
  resetStats(&test);
  assert(!puglStopTimer(test.view, 0U));
  assert(!puglStartTimer(test.view, NUM_TIMERS - 1U, otherTimerPeriod));
  assert(!puglUpdate(test.world, updateDuration));
  assert(test.stats[0].numAlarms == 0);
  checkTimerPeriod(&test.stats[NUM_TIMERS - 1U], otherTimerPeriod);
  for (unsigned i = 1U; i < NUM_TIMERS - 1U; ++i) {
    checkTimerPeriod(&test.stats[i], timerPeriod(i));
  }

  // Stop the last timer
  resetStats(&test);
  assert(!puglStopTimer(test.view, NUM_TIMERS - 1U));
  assert(!puglUpdate(test.world, updateDuration));
  assert(test.stats[0].numAlarms == 0);
  assert(test.stats[NUM_TIMERS - 1U].numAlarms == 0);
  for (unsigned i = 1U; i < NUM_TIMERS - 1U; ++i) {
    checkTimerPeriod(&test.stats[i], timerPeriod(i));
  }

  // Stop all other timers
  resetStats(&test);
  for (unsigned i = 1U; i < NUM_TIMERS - 1U; ++i) {
    assert(!puglStopTimer(test.view, i));
  }
  assert(!puglUpdate(test.world, 0.0));
  for (unsigned i = 0U; i < NUM_TIMERS; ++i) {
    assert(test.stats[0].numAlarms == 0);
  }

  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
