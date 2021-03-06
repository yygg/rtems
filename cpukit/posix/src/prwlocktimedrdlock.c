/**
 * @file
 *
 * @brief Attempt to Obtain a Read Lock on a RWLock Instance
 * @ingroup POSIXAPI
 */

/*
 *  COPYRIGHT (c) 1989-2008.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/posix/rwlockimpl.h>
#include <rtems/posix/posixapi.h>
#include <rtems/score/todimpl.h>

int pthread_rwlock_timedrdlock(
  pthread_rwlock_t      *rwlock,
  const struct timespec *abstime
)
{
  POSIX_RWLock_Control                    *the_rwlock;
  Thread_queue_Context                     queue_context;
  Watchdog_Interval                        ticks;
  bool                                     do_wait;
  TOD_Absolute_timeout_conversion_results  timeout_status;
  Status_Control                           status;

  /*
   *  POSIX requires that blocking calls with timeouts that take
   *  an absolute timeout must ignore issues with the absolute
   *  time provided if the operation would otherwise succeed.
   *  So we check the abstime provided, and hold on to whether it
   *  is valid or not.  If it isn't correct and in the future,
   *  then we do a polling operation and convert the STATUS_UNAVAILABLE
   *  status into the appropriate error.
   *
   *  If the timeout status is TOD_ABSOLUTE_TIMEOUT_INVALID,
   *  TOD_ABSOLUTE_TIMEOUT_IS_IN_PAST, or TOD_ABSOLUTE_TIMEOUT_IS_NOW,
   *  then we should not wait.
   */
  timeout_status = _TOD_Absolute_timeout_to_ticks( abstime, CLOCK_REALTIME, &ticks );
  do_wait = ( timeout_status == TOD_ABSOLUTE_TIMEOUT_IS_IN_FUTURE );

  the_rwlock = _POSIX_RWLock_Get( rwlock, &queue_context );

  if ( the_rwlock == NULL ) {
    return EINVAL;
  }

  _Thread_queue_Context_set_relative_timeout( &queue_context, ticks );
  status = _CORE_RWLock_Seize_for_reading(
    &the_rwlock->RWLock,
    _Thread_Executing,
    do_wait,
    &queue_context
  );

  if ( !do_wait && status == STATUS_UNAVAILABLE ) {
    if ( timeout_status == TOD_ABSOLUTE_TIMEOUT_INVALID ) {
      return EINVAL;
    }

    if (
      timeout_status == TOD_ABSOLUTE_TIMEOUT_IS_IN_PAST
        || timeout_status == TOD_ABSOLUTE_TIMEOUT_IS_NOW
    ) {
      return ETIMEDOUT;
    }
  }

  return _POSIX_Get_error( status );
}
