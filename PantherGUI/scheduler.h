#pragma once

#include "blockingconcurrentqueue.h"
#include "utils.h"
#include "thread.h"
#include <vector>

struct Task;

typedef void(*PGThreadFunctionParams)(Task*, void*);

enum PGTaskUrgency {
	PGTaskUrgent,
	PGTaskNotUrgent
};

struct Task {
	PGThreadFunctionParams function;
	void* parameter;
	bool active;

	Task(PGThreadFunctionParams function, void* parameter) : function(function), parameter(parameter), active(true) { }
};

class Scheduler {
public:
	static void Initialize() { GetInstance(); }

	static void SetThreadCount(lng count) { GetInstance()._SetThreadCount(count); }
	static lng GetThreadCount() { return GetInstance().threads.size(); }

	static bool IsRunning() { return GetInstance().running; }

	static void RegisterTask(Task* task, PGTaskUrgency urgency) { GetInstance()._RegisterTask(task, urgency); }
private:
	Scheduler();
	void _SetThreadCount(lng threads);
	void _RegisterTask(Task* task, PGTaskUrgency);
	static void RunThread(void);
	static Scheduler& GetInstance()
	{
		static Scheduler instance;
		return instance;
	}

	bool running = true;
	moodycamel::BlockingConcurrentQueue<Task*> urgent_queue;
	moodycamel::ConcurrentQueue<Task*> nonurgent_queue;
	std::vector<PGThreadHandle> threads;
};