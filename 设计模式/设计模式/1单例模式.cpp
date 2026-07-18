#include <iostream>
using namespace std;

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

int main()
{
	TaskQueue* taskq = TaskQueue::getInstance();
	TaskQueue* taskq2 = TaskQueue::getInstance();

	taskq->print();

	cout << taskq << endl;
	cout << taskq2 << endl;
	return	0;
}

// c++的静态成员方法或者成员属性只能通过 突破类域进行访问。
// 静态成员属性，类实例化的对象，只能共享一份的。
// 总结：实现一个类只能够通过静态方法访问，然后访问的资源放在静态属性


