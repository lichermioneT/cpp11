#include <iostream>
#include <mutex>
#include <atomic>
using namespace std;

// 饿汉模式
/*
class TaskQueue
{
public:
	//TaskQueue() = delete;
	TaskQueue(const TaskQueue& t) = delete;
	TaskQueue& operator=(const TaskQueue& t) = delete;
		
	static TaskQueue* getInstance()
	{
		return _taskQ;
	}

	void print()
	{
		cout << "单例模式的一个成员函数" << endl;
	}

private:
	// 方法1
	// 设置为私有，属性设置为模型的行为
	 TaskQueue() = default;
	// TaskQueue(const TaskQueue& t) = default;
	// TaskQueue& operator=(const TaskQueue& t) = default;

	// 只能通过静态属性或者方法访问
	static TaskQueue* _taskQ;
};
TaskQueue* TaskQueue::_taskQ = new TaskQueue();
*/

// 懒汉模式
/*
class TaskQueue
{
public:
	//TaskQueue() = delete;
	TaskQueue(const TaskQueue& t) = delete;
	TaskQueue& operator=(const TaskQueue& t) = delete;

	static TaskQueue* getInstance()
	{	
		// 第一次abc
		// 第二次xyz
		if (_taskQ == nullptr) // 第二次 xyz
		{
			_mutex.lock();
			if (_taskQ == nullptr) // 第一次 abc
			{
				_taskQ = new TaskQueue();
			}
			_mutex.unlock();
		}
		return _taskQ;
	}

	void print()
	{
		cout << "单例模式的一个成员函数" << endl;
	}

private:
	// 方法1
	// 设置为私有，属性设置为模型的行为
	TaskQueue() = default;
	// TaskQueue(const TaskQueue& t) = default;
	// TaskQueue& operator=(const TaskQueue& t) = default;

	// 只能通过静态属性或者方法访问
	static TaskQueue* _taskQ;
	static mutex _mutex;
};
TaskQueue* TaskQueue::_taskQ = nullptr;
mutex TaskQueue::_mutex;
// 懒汉模式有线程安全问题的。

*/

class TaskQueue
{
public:
	//TaskQueue() = delete;
	TaskQueue(const TaskQueue& t) = delete;
	TaskQueue& operator=(const TaskQueue& t) = delete;

	static TaskQueue* getInstance()
	{
		// 第一次abc
		// 第二次xyz
		TaskQueue* task = _taskQ.load();
		if (task == nullptr) // 第二次 xyz
		{
			_mutex.lock();
			task = _taskQ.load();
			if (task == nullptr) // 第一次 abc
			{
				task = new TaskQueue();
				_taskQ.store(task);
			}
			_mutex.unlock();
		}
		return task;
	}

	void print()
	{
		cout << "单例模式的一个成员函数" << endl;
	}

private:
	// 方法1
	// 设置为私有，属性设置为模型的行为
	TaskQueue() = default;
	// TaskQueue(const TaskQueue& t) = default;
	// TaskQueue& operator=(const TaskQueue& t) = default;

	// 只能通过静态属性或者方法访问
	static mutex _mutex;
	static atomic<TaskQueue*> _taskQ;
};
mutex TaskQueue::_mutex;
atomic<TaskQueue*> TaskQueue::_taskQ;
// 懒汉模式有线程安全问题的。


int main()
{
	TaskQueue* taskq = TaskQueue::getInstance();
	TaskQueue* taskq2 = TaskQueue::getInstance();

	taskq->print();

	cout << taskq << endl;
	cout << taskq2 << endl;
	return	0;
}

// 1.c++的静态成员方法或者成员属性只能通过 突破类域进行访问。
// 2.静态成员属性，类实例化的对象，只能共享一份的。
// 总结：实现一个类只能够通过静态方法访问，然后访问的资源放在静态属性