/* 
 * Main script that applies the linearizability tester on embb data structures.
 */

// Enable assertions even in Release mode
#ifdef NDEBUG
#undef NDEBUG
#include <assert.h>
#endif

#include <linearizability_tester.h>
#include <sequential_data_structures.h>
#include <tests.h>

#include <embb/base/thread.h>
#include <embb/containers/lock_free_stack.h>
#include <embb/containers/lock_free_mpmc_queue.h>
#include <embb/containers/wait_free_spsc_queue.h>

// Each thread executes quasi randomly operations (TryEqneueu, TryDequeue)
// on the concurrent data structure and construct the history.
template<std::size_t N, class S>
static void embb_worker_stack(
  const WorkerConfiguration& worker_configuration,
  ConcurrentLog<state::Stack<N>>& concurrent_log,
  S& concurrent_stack)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> value_dist('\0', worker_configuration.max_value);
  std::uniform_int_distribution<> percentage_dist(0, 100);

  bool ret;

  char value;
  unsigned percentage;
  EntryPtr<state::Stack<N>> call_entry_ptr;
  for (unsigned number_of_ops{ 0U };
  number_of_ops < worker_configuration.number_of_ops;
    ++number_of_ops)
  {
    value = value_dist(rd);
    percentage = percentage_dist(rd);
    // Note: this threshold affects considerably the running time of the test
    // increasing threshold -> increasing running time
    if (percentage < 30)
    {
      call_entry_ptr = concurrent_log.push_back(state::Stack<N>::make_try_push_call(value));
      ret = concurrent_stack.TryPush(value);
      concurrent_log.push_back(call_entry_ptr, state::Stack<N>::make_try_push_ret(ret));
    }
    else
    {
      call_entry_ptr = concurrent_log.push_back(state::Stack<N>::make_try_pop_call());
      ret = concurrent_stack.TryPop(value);
      concurrent_log.push_back(call_entry_ptr, state::Stack<N>::make_try_pop_ret(ret, value));
    }
  }
}

// Each thread executes quasi randomly operations (TryEnqueue, TryDequeue)
// on the concurrent data structure and construct the history.
template<std::size_t N, class S>
static void embb_worker_queue(
  const WorkerConfiguration& worker_configuration,
  ConcurrentLog<state::Queue<N>>& concurrent_log,
  S& concurrent_queue)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> value_dist('\0', worker_configuration.max_value);
  std::uniform_int_distribution<> percentage_dist(0, 100);

  bool ret;

  char value;
  unsigned percentage;
  EntryPtr<state::Queue<N>> call_entry_ptr;
  for (unsigned number_of_ops{ 0U };
  number_of_ops < worker_configuration.number_of_ops;
    ++number_of_ops)
  {
    value = value_dist(rd);
    percentage = percentage_dist(rd);
    // Note: this threshold affects considerably the running time of the test
    // increasing threshold -> increasing running time
    if (percentage < 20)
    {
      call_entry_ptr = concurrent_log.push_back(state::Queue<N>::make_try_enqueue_call(value));
      ret = concurrent_queue.TryEnqueue(value);
      concurrent_log.push_back(call_entry_ptr, state::Queue<N>::make_try_enqueue_ret(ret));
    }
    else
    {
      call_entry_ptr = concurrent_log.push_back(state::Queue<N>::make_try_dequeue_call());
      ret = concurrent_queue.TryDequeue(value);
      concurrent_log.push_back(call_entry_ptr, state::Queue<N>::make_try_dequeue_ret(ret, value));
    }
  }
}

// Creates the history and apply the tester on it
template <class S>
static void embb_experiment_stack()
{
  constexpr std::chrono::hours max_duration{ 1 };
  constexpr std::size_t N = 560000U;
  constexpr unsigned number_of_threads = 4U;
  constexpr WorkerConfiguration worker_configuration = { '\24', 70000U };
  constexpr unsigned log_size = number_of_threads * worker_configuration.number_of_ops;

  Result<state::Stack<N>> result;
  ConcurrentLog<state::Stack<N>> concurrent_log{ 2U * log_size };
  S concurrent_stack(N);

  // Check if push and pop operations are possible
  char value;
  assert(concurrent_stack.TryPush(5));
  assert(concurrent_stack.TryPop(value));
  
  // create history
  start_threads(number_of_threads, embb_worker_stack<N, S>, std::cref(worker_configuration),
    std::ref(concurrent_log), std::ref(concurrent_stack));

  const std::size_t number_of_entries{ concurrent_log.number_of_entries() };
  const LogInfo<state::Stack<N>> log_info{ concurrent_log.info() };

  auto start = std::chrono::system_clock::now();
  auto end = std::chrono::system_clock::now();
  std::chrono::seconds seconds;

  start = std::chrono::system_clock::now();
  {
    Log<state::Stack<N>> log_copy{ log_info };
    assert(log_copy.number_of_entries() == number_of_entries);

    LinearizabilityTester<state::Stack<N>, Option::LRU_CACHE> tester{ log_copy.info(), max_duration };
    tester.check(result);
    // If structure is not linearizabile break run using assertion
    assert(result.is_timeout() || result.is_linearizable());
  }
  end = std::chrono::system_clock::now();
  seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "History length: " << number_of_entries
    << ", elapsed time: "
    << seconds.count() << " s " << std::endl;
}

// Creates the history and apply the tester on it
template <class S>
static void embb_experiment_queue()
{
  constexpr std::chrono::hours max_duration{ 1 };
  constexpr std::size_t N = 560000U;
  constexpr unsigned number_of_threads = 4U;
  constexpr WorkerConfiguration worker_configuration = { '\24', 70000U };
  constexpr unsigned log_size = number_of_threads * worker_configuration.number_of_ops;


  Result<state::Queue<N>> result;
  ConcurrentLog<state::Queue<N>> concurrent_log{ 2U * log_size };
  S concurrent_queue(N);

  // check if enqueue and dequeue operations are possible
  char value;
  assert(concurrent_queue.TryEnqueue(5));
  assert(concurrent_queue.TryDequeue(value));

  // create history
  start_threads(number_of_threads, embb_worker_queue<N, S>, std::cref(worker_configuration),
    std::ref(concurrent_log), std::ref(concurrent_queue));
  const std::size_t number_of_entries{ concurrent_log.number_of_entries() };
  const LogInfo<state::Queue<N>> log_info{ concurrent_log.info() };

  auto start = std::chrono::system_clock::now();
  auto end = std::chrono::system_clock::now();
  std::chrono::seconds seconds;

  start = std::chrono::system_clock::now();
  {
    Log<state::Queue<N>> log_copy{ log_info };
    assert(log_copy.number_of_entries() == number_of_entries);
    LinearizabilityTester<state::Queue<N>, Option::LRU_CACHE> tester{ log_copy.info(), max_duration };
    tester.check(result);
    // If structure is not linearizabile break run using assertion
    assert(result.is_timeout() || result.is_linearizable());
  }
  end = std::chrono::system_clock::now();
  seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  std::cout << "History length: " << number_of_entries
    << ", elapsed time:  "
    << seconds.count() << " s " << std::endl;
}


int main()
{  

  // Test functions and structures in linearizability_tester.h and sequential_data_structures.h
  run_tests();
  embb::base::Thread::SetThreadsMaxCount(255);
  
  std::cout << "Linearizability test on LockFreeMPMCQueue" << std::endl;
  embb_experiment_queue<embb::containers::LockFreeMPMCQueue<char>>();

  std::cout << "Linearizability test on WaitFreeSPSCQueue" << std::endl;
  embb_experiment_queue<embb::containers::WaitFreeSPSCQueue<char>>();

  std::cout << "Linearizability test on LockFreeStack" << std::endl;
  embb_experiment_stack<embb::containers::LockFreeStack<char>>();
  return 0;
}

