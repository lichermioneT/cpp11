# Linux 下 C 语言线程操作总结

Linux 下的 C 语言多线程主要使用 **POSIX Threads**，简称 **Pthreads**。

相关头文件：

```c
#include <pthread.h>
```

编译时需要加：

```bash
gcc main.c -o main -pthread
```

推荐使用：

```bash
-pthread
```

而不是只写：

```bash
-lpthread
```

因为 `-pthread` 不仅链接线程库，还会启用编译阶段所需的线程相关配置。

------

# 一、Linux 线程的基本概念

线程是进程内部的一条执行流。

同一个进程中的多个线程共享：

- 代码段
- 全局变量
- 静态变量
- 堆内存
- 文件描述符
- 当前工作目录
- 信号处理方式

每个线程独有：

- 线程 ID
- 栈空间
- 寄存器上下文
- `errno`
- 信号屏蔽字
- 调度状态
- 线程局部存储

可以简单理解为：

```text
一个进程
├── 主线程
├── 工作线程1
├── 工作线程2
└── 工作线程3
```

进程中的第一个线程通常称为主线程。

------

# 二、线程创建：`pthread_create`

函数声明：

```c
int pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start_routine)(void *),
    void *arg
);
```

参数说明：

```text
thread          保存新线程的线程 ID
attr            线程属性，通常传 NULL
start_routine   新线程执行的函数
arg             传递给线程函数的参数
```

返回值：

```text
成功：0
失败：错误码
```

需要注意，Pthreads 函数失败时，通常直接返回错误码，不一定设置 `errno`。

## 基础示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

void *worker(void *arg)
{
    const char *name = (const char *)arg;

    printf("子线程开始运行，参数：%s\n", name);

    return NULL;
}

int main(void)
{
    pthread_t tid;

    int ret = pthread_create(&tid, NULL, worker, "thread-1");

    if (ret != 0)
    {
        fprintf(stderr, "pthread_create failed: %s\n",
                strerror(ret));
        return 1;
    }

    pthread_join(tid, NULL);

    return 0;
}
```

编译：

```bash
gcc main.c -o main -pthread
```

------

# 三、线程函数的格式

线程入口函数必须满足：

```c
void *函数名(void *arg);
```

例如：

```c
void *worker(void *arg)
{
    return NULL;
}
```

线程函数接收一个 `void *` 参数，因此可以传递任意类型的数据。

------

# 四、向线程传递参数

## 1. 传递单个整数

不要直接把普通整数当作指针随意传递。可以使用整数指针：

```c
#include <stdio.h>
#include <pthread.h>

void *worker(void *arg)
{
    int value = *(int *)arg;

    printf("value = %d\n", value);

    return NULL;
}

int main(void)
{
    pthread_t tid;
    int num = 100;

    pthread_create(&tid, NULL, worker, &num);
    pthread_join(tid, NULL);

    return 0;
}
```

这里必须保证：

```c
num
```

在线程使用它之前没有被销毁。

------

## 2. 传递结构体

当线程需要多个参数时，通常使用结构体：

```c
#include <stdio.h>
#include <pthread.h>

struct ThreadArg
{
    int id;
    const char *name;
};

void *worker(void *arg)
{
    struct ThreadArg *info = arg;

    printf("id = %d, name = %s\n",
           info->id,
           info->name);

    return NULL;
}

int main(void)
{
    pthread_t tid;

    struct ThreadArg arg = {
        .id = 1,
        .name = "worker"
    };

    pthread_create(&tid, NULL, worker, &arg);
    pthread_join(tid, NULL);

    return 0;
}
```

------

## 3. 循环创建线程时的常见错误

错误写法：

```c
for (int i = 0; i < 5; ++i)
{
    pthread_create(&tid[i], NULL, worker, &i);
}
```

所有线程获得的都是同一个变量 `i` 的地址，因此线程读取时，`i` 的值可能已经改变。

正确写法：

```c
int ids[5];

for (int i = 0; i < 5; ++i)
{
    ids[i] = i;
    pthread_create(&tid[i], NULL, worker, &ids[i]);
}
```

完整示例：

```c
#include <stdio.h>
#include <pthread.h>

void *worker(void *arg)
{
    int id = *(int *)arg;

    printf("线程编号：%d\n", id);

    return NULL;
}

int main(void)
{
    pthread_t tids[5];
    int ids[5];

    for (int i = 0; i < 5; ++i)
    {
        ids[i] = i;

        pthread_create(
            &tids[i],
            NULL,
            worker,
            &ids[i]
        );
    }

    for (int i = 0; i < 5; ++i)
    {
        pthread_join(tids[i], NULL);
    }

    return 0;
}
```

------

# 五、等待线程结束：`pthread_join`

函数声明：

```c
int pthread_join(
    pthread_t thread,
    void **retval
);
```

作用：

- 等待指定线程结束
- 回收线程资源
- 获取线程返回值

类似于 C++ 中的：

```cpp
std::thread::join()
```

## 基本用法

```c
pthread_join(tid, NULL);
```

主线程会阻塞，直到 `tid` 对应的线程结束。

------

# 六、线程返回值

线程可以通过以下两种方式结束：

```c
return 指针;
```

或者：

```c
pthread_exit(指针);
```

## 返回动态内存

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *worker(void *arg)
{
    int *result = malloc(sizeof(int));

    if (result == NULL)
    {
        return NULL;
    }

    *result = 123;

    return result;
}

int main(void)
{
    pthread_t tid;
    void *retval = NULL;

    pthread_create(&tid, NULL, worker, NULL);

    pthread_join(tid, &retval);

    if (retval != NULL)
    {
        int *result = retval;

        printf("线程返回值：%d\n", *result);

        free(result);
    }

    return 0;
}
```

不能返回局部变量地址：

```c
void *worker(void *arg)
{
    int value = 100;

    return &value; // 错误
}
```

因为线程函数结束后，局部变量 `value` 的生命周期已经结束。

------

# 七、线程主动退出：`pthread_exit`

函数声明：

```c
void pthread_exit(void *retval);
```

示例：

```c
void *worker(void *arg)
{
    printf("线程运行中\n");

    pthread_exit(NULL);

    printf("不会执行\n");
}
```

## 主线程调用 `pthread_exit`

如果主线程直接：

```c
return 0;
```

或者：

```c
exit(0);
```

整个进程会结束，其他线程也会被终止。

如果主线程调用：

```c
pthread_exit(NULL);
```

主线程结束，但进程中的其他线程仍然可以继续执行。

示例：

```c
int main(void)
{
    pthread_t tid;

    pthread_create(&tid, NULL, worker, NULL);

    pthread_exit(NULL);
}
```

不过在正常程序中，通常仍然优先使用：

```c
pthread_join()
```

回收子线程。

------

# 八、线程分离：`pthread_detach`

函数声明：

```c
int pthread_detach(pthread_t thread);
```

分离后的线程：

- 结束时自动回收资源
- 不能再被 `pthread_join`
- 不能获得线程返回值

示例：

```c
pthread_create(&tid, NULL, worker, NULL);
pthread_detach(tid);
```

类似于 C++：

```cpp
std::thread::detach()
```

## 注意生命周期

分离线程仍然可能继续访问参数。

错误示例：

```c
void test(void)
{
    int value = 10;
    pthread_t tid;

    pthread_create(&tid, NULL, worker, &value);
    pthread_detach(tid);
}
```

`test()` 返回后，`value` 被销毁，但分离线程可能还在访问它，产生未定义行为。

------

# 九、获取线程 ID：`pthread_self`

函数声明：

```c
pthread_t pthread_self(void);
```

示例：

```c
void *worker(void *arg)
{
    pthread_t tid = pthread_self();

    printf("当前线程 ID：%lu\n",
           (unsigned long)tid);

    return NULL;
}
```

线程 ID 的类型：

```c
pthread_t
```

它不保证一定是整数，因此判断两个线程 ID 是否相等时，推荐使用：

```c
pthread_equal()
```

------

# 十、比较线程 ID：`pthread_equal`

函数声明：

```c
int pthread_equal(
    pthread_t t1,
    pthread_t t2
);
```

示例：

```c
if (pthread_equal(tid1, tid2))
{
    printf("是同一个线程\n");
}
```

不要默认使用：

```c
tid1 == tid2
```

虽然部分 Linux 实现中可行，但可移植的 POSIX 写法是 `pthread_equal`。

------

# 十一、线程取消：`pthread_cancel`

函数声明：

```c
int pthread_cancel(pthread_t thread);
```

它的作用是向目标线程发送取消请求。

```c
pthread_cancel(tid);
```

但目标线程不一定立即退出。

线程取消受到以下因素影响：

- 取消状态
- 取消类型
- 是否到达取消点

常见取消点包括：

- `sleep`
- `read`
- `write`
- `pthread_cond_wait`
- `pthread_join`

线程可以手动测试取消请求：

```c
pthread_testcancel();
```

## 不建议随意取消线程

线程可能在持有互斥锁时被取消，从而造成：

- 互斥锁永久不释放
- 资源泄漏
- 数据结构处于不一致状态

实际项目中更推荐通过共享退出标志，让线程主动、安全地退出。

例如：

```c
int stop = 0;

void *worker(void *arg)
{
    while (!stop)
    {
        // 执行任务
    }

    return NULL;
}
```

不过 `stop` 被多线程访问时，仍然需要互斥锁或原子变量保护。

------

# 十二、互斥锁：`pthread_mutex_t`

多个线程同时访问共享数据，至少有一个线程执行写操作时，必须同步，否则会产生数据竞争。

## 初始化互斥锁

静态初始化：

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
```

动态初始化：

```c
pthread_mutex_t mutex;

pthread_mutex_init(&mutex, NULL);
```

销毁：

```c
pthread_mutex_destroy(&mutex);
```

------

## 加锁和解锁

```c
pthread_mutex_lock(&mutex);

/* 临界区 */

pthread_mutex_unlock(&mutex);
```

完整示例：

```c
#include <stdio.h>
#include <pthread.h>

int count = 0;

pthread_mutex_t mutex =
    PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
    for (int i = 0; i < 100000; ++i)
    {
        pthread_mutex_lock(&mutex);

        ++count;

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(void)
{
    pthread_t t1;
    pthread_t t2;

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("count = %d\n", count);

    pthread_mutex_destroy(&mutex);

    return 0;
}
```

理论结果：

```text
count = 200000
```

------

# 十三、尝试加锁：`pthread_mutex_trylock`

函数声明：

```c
int pthread_mutex_trylock(
    pthread_mutex_t *mutex
);
```

它不会一直阻塞。

```c
int ret = pthread_mutex_trylock(&mutex);

if (ret == 0)
{
    printf("获得锁\n");

    pthread_mutex_unlock(&mutex);
}
else if (ret == EBUSY)
{
    printf("锁正在被其他线程持有\n");
}
```

需要包含：

```c
#include <errno.h>
```

------

# 十四、互斥锁类型

Pthreads 支持多种互斥锁类型。

## 1. 普通互斥锁

```c
PTHREAD_MUTEX_NORMAL
```

同一个线程重复加锁可能导致死锁。

------

## 2. 递归互斥锁

```c
PTHREAD_MUTEX_RECURSIVE
```

允许同一个线程多次加锁，但加锁和解锁次数必须一致。

```c
pthread_mutexattr_t attr;

pthread_mutexattr_init(&attr);

pthread_mutexattr_settype(
    &attr,
    PTHREAD_MUTEX_RECURSIVE
);

pthread_mutex_init(&mutex, &attr);

pthread_mutexattr_destroy(&attr);
```

------

## 3. 错误检查互斥锁

```c
PTHREAD_MUTEX_ERRORCHECK
```

对于重复加锁、错误解锁等情况，可能返回错误码，便于调试。

通常生产代码仍以普通互斥锁为主。

------

# 十五、死锁

典型死锁：

```c
pthread_mutex_t mutex1 =
    PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex2 =
    PTHREAD_MUTEX_INITIALIZER;

void *thread1(void *arg)
{
    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);

    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);

    return NULL;
}

void *thread2(void *arg)
{
    pthread_mutex_lock(&mutex2);
    pthread_mutex_lock(&mutex1);

    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);

    return NULL;
}
```

可能出现：

```text
线程1持有 mutex1，等待 mutex2
线程2持有 mutex2，等待 mutex1
```

避免死锁的方法：

1. 所有线程按照相同顺序加锁。
2. 临界区尽可能短。
3. 不要在持锁期间执行耗时操作。
4. 不要在持锁期间调用不可控函数。
5. 必要时使用 `trylock`。
6. 尽量减少同时持有多把锁。

------

# 十六、条件变量：`pthread_cond_t`

条件变量用于线程等待某个条件成立。

常见场景：

```text
消费者发现队列为空
    ↓
等待

生产者添加任务
    ↓
通知消费者
```

条件变量必须和互斥锁配合使用。

------

## 初始化条件变量

静态初始化：

```c
pthread_cond_t cond =
    PTHREAD_COND_INITIALIZER;
```

动态初始化：

```c
pthread_cond_init(&cond, NULL);
```

销毁：

```c
pthread_cond_destroy(&cond);
```

------

## 等待条件变量

```c
pthread_cond_wait(&cond, &mutex);
```

调用前，当前线程必须已经持有 `mutex`。

`pthread_cond_wait` 会自动完成：

```text
1. 释放 mutex
2. 当前线程进入阻塞状态
3. 收到通知后重新竞争 mutex
4. 获得 mutex 后返回
```

------

## 唤醒线程

唤醒一个等待线程：

```c
pthread_cond_signal(&cond);
```

唤醒全部等待线程：

```c
pthread_cond_broadcast(&cond);
```

分别类似于 C++：

```cpp
notify_one()
notify_all()
```

------

# 十七、条件变量为什么必须配合 `while`

正确写法：

```c
pthread_mutex_lock(&mutex);

while (!条件成立)
{
    pthread_cond_wait(&cond, &mutex);
}

/* 处理共享数据 */

pthread_mutex_unlock(&mutex);
```

不推荐：

```c
if (!条件成立)
{
    pthread_cond_wait(&cond, &mutex);
}
```

因为可能发生：

- 虚假唤醒
- 多个消费者竞争
- 被唤醒后条件又被其他线程改变

因此线程被唤醒后，必须重新检查条件。

------

# 十八、生产者—消费者模型

下面是一个有界阻塞队列示例。

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define CAPACITY 5

struct TaskQueue
{
    int data[CAPACITY];

    int front;
    int rear;
    int size;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

void task_queue_init(struct TaskQueue *q)
{
    q->front = 0;
    q->rear = 0;
    q->size = 0;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void task_queue_destroy(struct TaskQueue *q)
{
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

void task_queue_put(
    struct TaskQueue *q,
    int value
)
{
    pthread_mutex_lock(&q->mutex);

    while (q->size == CAPACITY)
    {
        pthread_cond_wait(
            &q->not_full,
            &q->mutex
        );
    }

    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % CAPACITY;
    ++q->size;

    printf("生产任务：%d，当前数量：%d\n",
           value,
           q->size);

    pthread_cond_signal(&q->not_empty);

    pthread_mutex_unlock(&q->mutex);
}

int task_queue_take(struct TaskQueue *q)
{
    pthread_mutex_lock(&q->mutex);

    while (q->size == 0)
    {
        pthread_cond_wait(
            &q->not_empty,
            &q->mutex
        );
    }

    int value = q->data[q->front];

    q->front = (q->front + 1) % CAPACITY;
    --q->size;

    printf("消费任务：%d，当前数量：%d\n",
           value,
           q->size);

    pthread_cond_signal(&q->not_full);

    pthread_mutex_unlock(&q->mutex);

    return value;
}
```

生产者：

```c
void *producer(void *arg)
{
    struct TaskQueue *q = arg;

    for (int i = 0; i < 10; ++i)
    {
        task_queue_put(q, i);
    }

    return NULL;
}
```

消费者：

```c
void *consumer(void *arg)
{
    struct TaskQueue *q = arg;

    for (int i = 0; i < 10; ++i)
    {
        int task = task_queue_take(q);
        printf("消费者获得：%d\n", task);
    }

    return NULL;
}
```

主函数：

```c
int main(void)
{
    struct TaskQueue queue;

    task_queue_init(&queue);

    pthread_t producer_tid;
    pthread_t consumer_tid;

    pthread_create(
        &producer_tid,
        NULL,
        producer,
        &queue
    );

    pthread_create(
        &consumer_tid,
        NULL,
        consumer,
        &queue
    );

    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    task_queue_destroy(&queue);

    return 0;
}
```

通知关系：

```text
生产者添加任务
    ↓
队列变为非空
    ↓
pthread_cond_signal(not_empty)

消费者取走任务
    ↓
队列变为非满
    ↓
pthread_cond_signal(not_full)
```

------

# 十九、信号量：`sem_t`

POSIX 无名信号量通常用于线程同步和资源计数。

头文件：

```c
#include <semaphore.h>
```

初始化：

```c
sem_t sem;

sem_init(&sem, 0, 初始值);
```

第二个参数：

```text
0：线程之间共享
非0：进程之间共享
```

等待信号量：

```c
sem_wait(&sem);
```

增加信号量：

```c
sem_post(&sem);
```

销毁：

```c
sem_destroy(&sem);
```

------

## 信号量基础示例

```c
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

sem_t sem;

void *worker(void *arg)
{
    sem_wait(&sem);

    printf("线程开始执行\n");

    sem_post(&sem);

    return NULL;
}

int main(void)
{
    pthread_t tids[3];

    sem_init(&sem, 0, 1);

    for (int i = 0; i < 3; ++i)
    {
        pthread_create(
            &tids[i],
            NULL,
            worker,
            NULL
        );
    }

    for (int i = 0; i < 3; ++i)
    {
        pthread_join(tids[i], NULL);
    }

    sem_destroy(&sem);

    return 0;
}
```

当初始值为 `1` 时，信号量可以实现类似互斥锁的效果。

------

## 信号量控制并发数量

例如最多允许三个线程同时访问资源：

```c
sem_init(&sem, 0, 3);
```

每个线程进入资源前：

```c
sem_wait(&sem);
```

离开资源后：

```c
sem_post(&sem);
```

------

# 二十、读写锁：`pthread_rwlock_t`

读写锁适合：

```text
读操作很多
写操作很少
```

特点：

- 多个读线程可以同时持有读锁
- 写线程需要独占
- 写锁存在时，其他读写线程不能进入

初始化：

```c
pthread_rwlock_t rwlock =
    PTHREAD_RWLOCK_INITIALIZER;
```

获取读锁：

```c
pthread_rwlock_rdlock(&rwlock);
```

获取写锁：

```c
pthread_rwlock_wrlock(&rwlock);
```

解锁：

```c
pthread_rwlock_unlock(&rwlock);
```

销毁：

```c
pthread_rwlock_destroy(&rwlock);
```

示例：

```c
int shared_data = 0;

pthread_rwlock_t rwlock =
    PTHREAD_RWLOCK_INITIALIZER;

void *reader(void *arg)
{
    pthread_rwlock_rdlock(&rwlock);

    printf("读取数据：%d\n", shared_data);

    pthread_rwlock_unlock(&rwlock);

    return NULL;
}

void *writer(void *arg)
{
    pthread_rwlock_wrlock(&rwlock);

    ++shared_data;

    pthread_rwlock_unlock(&rwlock);

    return NULL;
}
```

------

# 二十一、一次性初始化：`pthread_once`

对应 C++11 的：

```cpp
std::call_once
```

定义：

```c
pthread_once_t once_control =
    PTHREAD_ONCE_INIT;
```

初始化函数：

```c
void init_resource(void)
{
    printf("资源只初始化一次\n");
}
```

调用：

```c
pthread_once(
    &once_control,
    init_resource
);
```

即使多个线程同时调用，也只会成功执行一次。

示例：

```c
#include <stdio.h>
#include <pthread.h>

pthread_once_t once =
    PTHREAD_ONCE_INIT;

void init(void)
{
    printf("初始化执行一次\n");
}

void *worker(void *arg)
{
    pthread_once(&once, init);

    return NULL;
}
```

------

# 二十二、线程属性：`pthread_attr_t`

线程属性可以控制：

- 分离状态
- 栈大小
- 栈地址
- 调度策略
- 继承调度方式

基本流程：

```c
pthread_attr_t attr;

pthread_attr_init(&attr);

/* 设置属性 */

pthread_create(&tid, &attr, worker, NULL);

pthread_attr_destroy(&attr);
```

------

## 创建分离线程

```c
pthread_attr_t attr;

pthread_attr_init(&attr);

pthread_attr_setdetachstate(
    &attr,
    PTHREAD_CREATE_DETACHED
);

pthread_create(
    &tid,
    &attr,
    worker,
    NULL
);

pthread_attr_destroy(&attr);
```

可选状态：

```c
PTHREAD_CREATE_JOINABLE
PTHREAD_CREATE_DETACHED
```

默认是：

```c
PTHREAD_CREATE_JOINABLE
```

------

## 设置线程栈大小

```c
pthread_attr_setstacksize(
    &attr,
    1024 * 1024
);
```

例如设置为 1 MB。

需要注意，栈大小不能小于：

```c
PTHREAD_STACK_MIN
```

------

# 二十三、线程局部存储

有些变量需要做到：

```text
变量名相同
但每个线程拥有独立副本
```

这叫线程局部存储。

## 使用 `_Thread_local`

C11 提供：

```c
_Thread_local int value;
```

示例：

```c
_Thread_local int thread_count = 0;
```

每个线程都有自己的 `thread_count`。

GCC 也支持：

```c
__thread int value;
```

不过 `_Thread_local` 是 C11 标准写法。

------

## 使用 Pthreads 线程特定数据

相关接口：

```c
pthread_key_create()
pthread_setspecific()
pthread_getspecific()
pthread_key_delete()
```

适合动态管理每个线程独立的数据。

------

# 二十四、线程与信号

在多线程程序中，每个线程拥有独立的信号屏蔽字。

线程中通常使用：

```c
pthread_sigmask()
```

而不是直接使用：

```c
sigprocmask()
```

函数声明：

```c
int pthread_sigmask(
    int how,
    const sigset_t *set,
    sigset_t *oldset
);
```

一种常见设计是：

```text
所有工作线程屏蔽特定信号
        ↓
专门创建一个信号处理线程
        ↓
使用 sigwait 等待信号
```

这样比在异步信号处理函数中执行复杂逻辑更安全。

------

# 二十五、线程安全和可重入函数

多线程程序中应注意某些传统函数可能不是线程安全的。

例如传统的：

```c
strtok()
localtime()
asctime()
gethostbyname()
```

可能使用内部静态缓冲区。

有些函数提供可重入版本：

```c
strtok_r()
localtime_r()
asctime_r()
gethostbyname_r()
```

例如：

```c
struct tm result;

localtime_r(&timestamp, &result);
```

不过不同接口的推荐替代方案还需要根据实际场景选择。

------

# 二十六、线程资源回收

线程分为两种状态：

## 1. 可连接线程

默认创建的线程是可连接线程。

结束后资源不会立即完全回收，需要：

```c
pthread_join()
```

------

## 2. 分离线程

调用：

```c
pthread_detach()
```

或者通过线程属性创建分离线程。

线程结束后资源自动回收，不能再 `join`。

关系如下：

```text
可连接线程
    ↓
pthread_join
    ↓
回收资源

分离线程
    ↓
执行结束
    ↓
系统自动回收资源
```

一个线程不能既 `join` 又 `detach`。

------

# 二十七、常见错误

## 1. 忘记回收线程

```c
pthread_create(&tid, NULL, worker, NULL);
```

既不：

```c
pthread_join(tid, NULL);
```

也不：

```c
pthread_detach(tid);
```

线程结束后可能持续占用部分系统资源。

------

## 2. 返回局部变量地址

```c
void *worker(void *arg)
{
    int value = 10;

    return &value;
}
```

错误，因为局部变量在线程函数结束后失效。

------

## 3. 多个线程同时访问共享变量

```c
int count = 0;

++count;
```

`++count` 并不是原子操作。

需要：

```c
pthread_mutex_lock(&mutex);
++count;
pthread_mutex_unlock(&mutex);
```

------

## 4. 检查和操作分开加锁

错误：

```c
if (!queue_is_empty())
{
    queue_pop();
}
```

即使 `queue_is_empty()` 内部加锁也不安全，因为检查结束后锁已经释放。

正确做法：

```c
pthread_mutex_lock(&mutex);

if (!queue_empty)
{
    /* pop */
}

pthread_mutex_unlock(&mutex);
```

判断和修改必须位于同一个临界区。

------

## 5. 条件变量使用 `if`

错误：

```c
if (queue_empty)
{
    pthread_cond_wait(&cond, &mutex);
}
```

正确：

```c
while (queue_empty)
{
    pthread_cond_wait(&cond, &mutex);
}
```

------

## 6. 忘记解锁

```c
pthread_mutex_lock(&mutex);

if (error)
{
    return NULL; // 没有解锁
}
```

正确处理：

```c
pthread_mutex_lock(&mutex);

if (error)
{
    pthread_mutex_unlock(&mutex);
    return NULL;
}
```

C 没有 C++ 的 `lock_guard` 自动管理锁，因此必须特别注意每一条退出路径。

------

## 7. 持锁期间执行耗时操作

不推荐：

```c
pthread_mutex_lock(&mutex);

sleep(5);
read(fd, buf, size);
复杂计算();

pthread_mutex_unlock(&mutex);
```

这样会让其他线程长期等待。

应尽可能只在访问共享数据时持锁。

------

## 8. 错误使用 `volatile`

```c
volatile int stop;
```

`volatile` 不能保证：

- 原子性
- 线程同步
- 多步操作安全
- 完整的内存可见性语义

线程同步应使用：

- 互斥锁
- 条件变量
- 信号量
- C11 原子变量

------

## 9. 所有线程传递同一个循环变量地址

错误：

```c
for (int i = 0; i < 5; ++i)
{
    pthread_create(&tid[i], NULL, worker, &i);
}
```

正确：

```c
int ids[5];

for (int i = 0; i < 5; ++i)
{
    ids[i] = i;
    pthread_create(&tid[i], NULL, worker, &ids[i]);
}
```

------

# 二十八、Pthreads 与 C++11 线程库对照

| Linux C Pthreads         | C++11                              |
| ------------------------ | ---------------------------------- |
| `pthread_create`         | `std::thread`                      |
| `pthread_join`           | `thread::join`                     |
| `pthread_detach`         | `thread::detach`                   |
| `pthread_self`           | `this_thread::get_id`              |
| `pthread_mutex_t`        | `std::mutex`                       |
| `pthread_mutex_lock`     | `mutex::lock`                      |
| `pthread_mutex_unlock`   | `mutex::unlock`                    |
| `pthread_cond_t`         | `std::condition_variable`          |
| `pthread_cond_wait`      | `condition_variable::wait`         |
| `pthread_cond_signal`    | `notify_one`                       |
| `pthread_cond_broadcast` | `notify_all`                       |
| `pthread_once`           | `std::call_once`                   |
| `pthread_rwlock_t`       | C++17 的 `std::shared_mutex`       |
| `sem_t`                  | C++20 的 `std::counting_semaphore` |
| `pthread_key_t`          | `thread_local` 或线程特定数据      |

C++11 的线程库底层在 Linux 上通常仍然基于 Pthreads 实现，但它提供了更安全的 RAII 封装。

------

# 二十九、常用接口速查表

## 线程管理

| 接口             | 作用            |
| ---------------- | --------------- |
| `pthread_create` | 创建线程        |
| `pthread_join`   | 等待并回收线程  |
| `pthread_detach` | 分离线程        |
| `pthread_exit`   | 当前线程退出    |
| `pthread_cancel` | 请求取消线程    |
| `pthread_self`   | 获取当前线程 ID |
| `pthread_equal`  | 比较线程 ID     |

## 互斥同步

| 接口                    | 作用         |
| ----------------------- | ------------ |
| `pthread_mutex_init`    | 初始化互斥锁 |
| `pthread_mutex_lock`    | 加锁         |
| `pthread_mutex_trylock` | 尝试加锁     |
| `pthread_mutex_unlock`  | 解锁         |
| `pthread_mutex_destroy` | 销毁互斥锁   |

## 条件变量

| 接口                     | 作用           |
| ------------------------ | -------------- |
| `pthread_cond_init`      | 初始化条件变量 |
| `pthread_cond_wait`      | 等待条件       |
| `pthread_cond_timedwait` | 限时等待       |
| `pthread_cond_signal`    | 唤醒一个线程   |
| `pthread_cond_broadcast` | 唤醒全部线程   |
| `pthread_cond_destroy`   | 销毁条件变量   |

## 读写锁

| 接口                     | 作用         |
| ------------------------ | ------------ |
| `pthread_rwlock_init`    | 初始化读写锁 |
| `pthread_rwlock_rdlock`  | 获取读锁     |
| `pthread_rwlock_wrlock`  | 获取写锁     |
| `pthread_rwlock_unlock`  | 解锁         |
| `pthread_rwlock_destroy` | 销毁读写锁   |

## 信号量

| 接口            | 作用             |
| --------------- | ---------------- |
| `sem_init`      | 初始化信号量     |
| `sem_wait`      | 信号量减一并等待 |
| `sem_trywait`   | 非阻塞等待       |
| `sem_timedwait` | 限时等待         |
| `sem_post`      | 信号量加一       |
| `sem_destroy`   | 销毁信号量       |

------

# 三十、建议的学习顺序

## 第一阶段：线程基础

```text
pthread_create
pthread_join
pthread_detach
pthread_exit
pthread_self
线程参数传递
线程返回值
```

## 第二阶段：互斥同步

```text
pthread_mutex_t
pthread_mutex_lock
pthread_mutex_unlock
临界区
数据竞争
死锁
```

## 第三阶段：线程通信

```text
pthread_cond_t
pthread_cond_wait
pthread_cond_signal
pthread_cond_broadcast
生产者消费者模型
阻塞任务队列
```

## 第四阶段：高级同步

```text
sem_t
pthread_rwlock_t
pthread_once
线程属性
线程局部存储
```

## 第五阶段：工程实践

```text
线程池
任务队列
优雅退出
线程安全日志
网络服务器中的工作线程
Reactor + 线程池
```

------

# 核心结论

```text
1. pthread_create 负责创建线程。

2. pthread_join 负责等待线程结束并回收资源。

3. pthread_detach 让线程结束后自动回收资源。

4. pthread_mutex_t 负责保护共享数据。

5. pthread_cond_t 负责线程等待和通知，但不保护共享数据。

6. 条件变量必须与互斥锁一起使用。

7. pthread_cond_wait 外层应使用 while，而不是 if。

8. 判断共享状态和修改共享状态必须位于同一个临界区。

9. 可连接线程最终必须 join，或者提前设置为 detach。

10. C 语言没有 RAII，必须人工保证每条路径都正确解锁和释放资源。

11. 多线程中不要无保护地共享局部变量地址、全局变量和容器。

12. 线程池本质上通常是：
    多个 pthread 工作线程
    + 互斥锁
    + 条件变量
    + 任务队列。
```

Linux C 语言线程库可以整体理解为：

```text
pthread_create / pthread_join
    负责线程生命周期

pthread_mutex_t
    负责互斥访问共享数据

pthread_cond_t
    负责等待条件和线程唤醒

sem_t
    负责资源计数和同步

pthread_rwlock_t
    负责读多写少场景

pthread_once
    负责一次性初始化
```