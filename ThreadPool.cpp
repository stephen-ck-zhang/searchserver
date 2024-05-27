/*
 * Copyright ©2024 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2024 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <unistd.h>
#include <iostream>

#include "./ThreadPool.hpp"

namespace searchserver {

// This is the thread start routine, i.e., the function that threads
// are born into.
void *thread_loop(void *t_pool);

ThreadPool::ThreadPool(uint32_t num_threads) {
  // Initialize our member variables.
  num_threads_running_ = 0;
  killthreads_ = false;
  pthread_mutex_init(&q_lock_, nullptr);
  pthread_cond_init(&q_cond_, nullptr);

  // Allocate the array of pthread structures.
  thread_array_ = new pthread_t[num_threads];

  // Spawn the threads one by one, passing them a pointer to self
  // as the argument to the thread start routine.
  pthread_mutex_lock(&q_lock_);
  for (uint32_t i = 0; i < num_threads; i++) {
    pthread_create(&(thread_array_[i]),
                             nullptr,
                             &thread_loop,
                             static_cast<void *>(this));
  }

  // Wait for all of the threads to be born and initialized.
  while (num_threads_running_ != num_threads) {
    pthread_mutex_unlock(&q_lock_);
    sleep(1);  // give another thread the chance to acquire the lock
    pthread_mutex_lock(&q_lock_);
  }
  pthread_mutex_unlock(&q_lock_);

  // Done!  The thread pool is ready, and all of the worker threads
  // are initialized and waiting on q_cond_ to be notified of available
  // work.
}

ThreadPool:: ~ThreadPool() {
  pthread_mutex_lock(&q_lock_);
  uint32_t num_threads = num_threads_running_;

  // Tell all of the worker threads to kill themselves.
  killthreads_ = true;

  // Join with the running threads 1-by-1 until they have all died.
  for (uint32_t i = 0; i < num_threads; i++) {
    // Use a sledgehammer and broadcast every loop iteration, just to
    // be extra-certain that worker threads wake up and see the "kill
    // yourself" flag.
    pthread_cond_broadcast(&q_cond_);
    pthread_mutex_unlock(&q_lock_);
    pthread_join(thread_array_[i], nullptr);
    pthread_mutex_lock(&q_lock_);
  }

  // All of the worker threads are dead, so clean up the thread
  // structures.
  if (thread_array_ != nullptr) {
    delete[] thread_array_;
  }
  thread_array_ = nullptr;
  pthread_mutex_unlock(&q_lock_);

  // Empty the task queue, serially issuing any remaining work.
  while (!work_queue_.empty()) {
    Task *nextTask = work_queue_.front();
    work_queue_.pop_front();
    nextTask->func_(nextTask);
  }
}

// Enqueue a Task for dispatch.
void ThreadPool::dispatch(Task *t) {
  pthread_mutex_lock(&q_lock_);
  work_queue_.push_back(t);
  pthread_cond_signal(&q_cond_);
  pthread_mutex_unlock(&q_lock_);
}

// This is the main loop that all worker threads are born into.  They
// wait for a signal on the work queue condition variable, then they
// grab work off the queue.  Threads return (i.e., kill themselves)
// when they notice that killthreads_ is true.
void *thread_loop(void *t_pool) {
  ThreadPool *pool = static_cast<ThreadPool *>(t_pool);

  // Grab the lock, increment the thread count so that the ThreadPool
  // constructor knows this new thread is alive.
  pthread_mutex_lock(&(pool->q_lock_));
  pool->num_threads_running_++;

  // This is our main thread work loop.
  while (pool->killthreads_ == false) {
    // Wait to be signaled that something has happened.
    pthread_cond_wait(&(pool->q_cond_), &(pool->q_lock_));

    // Keep trying to dequeue work until the work queue is empty.
    while (!pool->work_queue_.empty() && (pool->killthreads_ == false)) {
      ThreadPool::Task *nextTask = pool->work_queue_.front();
      pool->work_queue_.pop_front();

      // We picked up a Task, so invoke the task function with the
      // lock released, then check so see if more tasks are waiting to
      // be picked up.
      pthread_mutex_unlock(&(pool->q_lock_));
      nextTask->func_(nextTask);
      pthread_mutex_lock(&(pool->q_lock_));
    }
  }

  // All done, exit.
  pool->num_threads_running_--;
  pthread_mutex_unlock(&(pool->q_lock_));
  return nullptr;
}

}  // namespace searchserver
