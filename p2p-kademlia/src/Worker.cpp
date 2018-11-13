
#include "Worker.h"

#include <chrono>
#include <thread>
using namespace std;
using namespace wd;

void Worker::startWorking()
{
	std::cout << "startWorking for thread" << m_name << std::endl;
	std::unique_lock<std::mutex> l(x_work);
	if (m_work)
	{
		WorkerState ex = WorkerState::Stopped;
		m_state.compare_exchange_strong(ex, WorkerState::Starting);
		m_state_notifier.notify_all();
	}
	else
	{
		m_state = WorkerState::Starting;
		m_state_notifier.notify_all();
		m_work.reset(new thread([&]()
		{
			std::cout << "Thread begins" << std::endl;
			while (m_state != WorkerState::Killing)
			{
				WorkerState ex = WorkerState::Starting;
				{
					// the condition variable-related lock
					unique_lock<mutex> l(x_work);
					m_state = WorkerState::Started;
				}
				std::cout << "Trying to set Started: Thread was" << (unsigned)ex << "; " << std::endl;
				m_state_notifier.notify_all();

				try
				{
					startedWorking();
					workLoop();
					doneWorking();
				}
				catch (std::exception const& _e)
				{
					std::cout << "Exception thrown in Worker thread: " << std::endl;
				}
				{
					unique_lock<mutex> l(x_work);
					ex = m_state.exchange(WorkerState::Stopped);
					if (ex == WorkerState::Killing || ex == WorkerState::Starting)
						m_state.exchange(ex);
				}
				m_state_notifier.notify_all();
				{
					unique_lock<mutex> l(x_work);
					while (m_state == WorkerState::Stopped)
						m_state_notifier.wait(l);
				}
			}
		}));
	}
		while (m_state == WorkerState::Starting)
			m_state_notifier.wait(l);
}

void Worker::stopWorking()
{
	std::unique_lock<std::mutex> l(x_work);
	if (m_work)
	{
		WorkerState ex = WorkerState::Started;
		if (!m_state.compare_exchange_strong(ex, WorkerState::Stopping))
			return;
		m_state_notifier.notify_all();
		m_state_notifier.wait(l); // but yes who can wake this up, when the mutex is taken.
	}
}

void Worker::terminate()
{
//	cnote << "stopWorking for thread" << m_name;
	std::unique_lock<std::mutex> l(x_work);
	if (m_work)
	{
		if (m_state.exchange(WorkerState::Killing) == WorkerState::Killing)
			return; // Somebody else is doing this
		l.unlock();
		m_state_notifier.notify_all();
		m_work->join();

		l.lock();
		m_work.reset();
	}
}

void Worker::workLoop()
{
	std::cout << "into worklopp ..." << std::endl;
	while (m_state == WorkerState::Started)
	{
		if (m_idleWaitMs)
			this_thread::sleep_for(chrono::milliseconds(m_idleWaitMs));
		doWork();
	}
}
