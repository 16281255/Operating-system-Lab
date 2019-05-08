@[toc](Pintos Project1)
小组成员：
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;黄&nbsp;&nbsp;&nbsp;东:16281255
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;苏荣天:16281141
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;罗泽昊:16281294
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;李&nbsp;&nbsp;&nbsp;&nbsp;超:16281194
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;袁恩泽:16281307
# 1 Alarm Clock
## 1.1 源码分析
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508162428267.png)

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508162445909.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508162451202.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/2019050816245535.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508162500663.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508162504281.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/2019050816251268.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508162517445.png)
## 1.2 实现思路
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;这一部分要求我们重新实现定义在device/timer.c中的timer_sleep函数，通过刚刚对源码的分析我们可以发现，现有的timer_sleep函数是一种称为“忙等待”的实现方式，也就是说，当我们调用timer_sleep函数时，它会在循环中检查当前时间是否已经过去了ticks个时钟，如果没有，将会调用thread_yield函数，将当前进程加入到就绪队列，将CPU让给就绪队列中的其他进程。由于在这个过程中，进程在CPU就绪队列和运行队列间来回切换，而且即使没有到达ticks个始终，CPU依然会激活进程，因此，浪费了大量的CPU资源以及时间。
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;为了避免这个问题，我们可以将当进程在调用timer_sleep函数时，直接将其阻塞，同时在结构体加入一个变量，用来记录当前进程距执行还差多少时刻。当时间到0时，便将进程加入到就绪队列中。
## 1.3 实现代码
为实现我们的想法，首先我们需要在线程结构体中加入一个变量，ticks_blocked，用于记录进程距离执行还差的时间

```c
int64_t ticks_blocked;
```
然后我们需要修改thread_create函数，使得在进程被创建时初始化ticks_blocked为0

```c
t->ticks_blocked = 0;
```
现在，我们需要修改timer_interrupt函数，使得每过一个ticks,ticks_blocked就能够相应的减1.为实现这一功能，我们需要在thread.c中新建一个函数blocked_thread_check，当ticks_blocked到0时，调用thread_unblock，将进程加入到就绪队列。

```c
void blocked_thread_check (struct thread *t, void *aux UNUSED)
{
  if (t->status == THREAD_BLOCKED && t->ticks_blocked > 0)
  {
      t->ticks_blocked--;
      if (t->ticks_blocked == 0)
      {
          thread_unblock(t);
      }
  }
}
```
那么我们如何对阻塞队列中的每一个进程都调用这个函数呢，这时我们就需要用到thread_foreach函数，而他的功能，就是对所有的线程执行参数中的thread_action_func
```c
/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}
```
最后，我们重新实现timer_sleep函数

```c
void timer_sleep (int64_t ticks)
{
  if (ticks <= 0)
  {
    return;
  }
  ASSERT (intr_get_level () == INTR_ON);
  enum intr_level old_level = intr_disable ();
  struct thread *current_thread = thread_current ();
  current_thread->ticks_blocked = ticks;
  thread_block ();
  intr_set_level (old_level);
}
```
## 1.4 实验结果
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508201623394.png)
# 2 Priority Scheduling
首先，我们来分析一下源码，可以看到，在线程的结构体中已经为我们定义好了priority，进一分析发现，priority的取值范围是0-63,而且数值越小，优先级越低。
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508171733392.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQwNzQzMTQ0,size_16,color_FFFFFF,t_70)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508171806108.png)
为了实现进程能按优先级执行，我们主要需要实现三点，分别是：一，就绪队列中的进程能够按照优先级排序，二，当高优先级进程进入就绪队列时，低优先级进程立即放弃CPU，同时交给高优先级进程执行。三，当进程同时在等待锁时，应首先唤醒高优先级的进程。线程可以随时提高或降低自己的优先级。
## 2.1就绪队列为优先级队列
为了保证就绪队列为一个优先级队列，我们就需要使得进程每一次加入队列时，都是按照优先级排序。那么现在我们需要考虑的就是进程在什么情况下会被加入就绪队列，通过之前的分析可以发现，当进程调用thread_unblock(),init_thread(),thread_yield()这三个函数时，进程将会被加入就绪队列，进一步阅读这三个函数可以发现，他们调用的都是list_push_back()，也就是说我们修改这个函数就可以实现我们想要的功能。这时我们发现了list_insert_ordered（）函数，从命名上我们就可以看出他的功能。

```c
/* Inserts ELEM in the proper position in LIST, which must be
   sorted according to LESS given auxiliary data AUX.
   Runs in O(n) average case in the number of elements in LIST. */
void
list_insert_ordered (struct list *list, struct list_elem *elem,
                     list_less_func *less, void *aux)
{
  struct list_elem *e;

  ASSERT (list != NULL);
  ASSERT (elem != NULL);
  ASSERT (less != NULL);

  for (e = list_begin (list); e != list_end (list); e = list_next (e))
    if (less (elem, e, aux))
      break;
  return list_insert (e, elem);
}
```
于是，我们将这三个函数里的list_push_back()全部修改为list_insert_ordered()

```c
list_insert_ordered (&ready_list, &t->elem, (list_less_func *) &thread_cmp_priority, NULL);//     thread_unblock()
list_insert_ordered (&all_list, &t->allelem, (list_less_func *) &thread_cmp_priority, NULL);//    init_thread()
list_insert_ordered (&ready_list, &cur->elem, (list_less_func *) &thread_cmp_priority, NULL); //  thread_yield()
```
现在，我们需要实现thread_cmp_priority()

```c
/* priority compare function. */
bool
thread_cmp_priority (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
    return list_entry(a, struct thread, elem)->priority > list_entry(b, struct thread, elem)->priority;
}
```
## 2.2 高优先级线程优先执行
在设置一个线程优先级时要立即重新考虑所有线程的执行顺序，重新安排执行顺序。为了实现这一目标，我们可以在线程设置优先级时调用thread_yield，这样就可以把当前线程重新加入到就绪队列中继续执行，也就重新安排了执行顺序。
```c
/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority)
{
  thread_current ()->priority = new_priority;
  thread_yield ();
}
```
除此之外，在新线程在创建时，如果优先级比当前线程优先级高的话也要调用thread_yield。需要在thread_create最后当把线程unblock之后加上下面的代码。

```c
if (thread_current ()->priority < priority)
{
   thread_yield ();
 }
```
## 2.3 优先级捐赠
通过实验指导书，我们可以初步理解优先级捐赠的概念，即对于三个不同优先级的进程H,M,L,其中H代表高优先级进程，M代表中优先级进程，L代表低优先级进程。如果进程L占有一个锁，而同时H要请求这个锁，但是M又在就绪队列中，那么H将永远不会得到CPU，因为L不会得到任何CPU时间，也就不会释放锁，H也将得不到锁，不能被处理。为解决这个问题，我们需要实施优先级捐赠。
查阅资料发现，实现优先级捐赠需要注意以下几点：
1.在一个线程获取一个锁的时候， 如果拥有这个锁的线程优先级比自己低就提高它的优先级，并且如果这个锁还被别的锁锁着， 将会递归地捐赠优先级， 然后在这个线程释放掉这个锁之后恢复未捐赠逻辑下的优先级。
2.如果一个线程被多个线程捐赠， 维持当前优先级为捐赠优先级中的最大值（acquire和release之时）。
3.在对一个线程进行优先级设置的时候， 如果这个线程处于被捐赠状态， 则对original_priority进行设置， 然后如果设置的优先级大于当前优先级， 则改变当前优先级， 否则在捐赠状态取消的时候恢复original_priority。
4.在释放锁对一个锁优先级有改变的时候应考虑其余被捐赠优先级和当前优先级。
5.将信号量的等待队列实现为优先级队列。
6.将condition的waiters队列实现为优先级队列。
7.释放锁的时候若优先级改变则可以发生抢占。

为实现以上功能，我们需要先修改进程的数据结构，加入以下成员：

```c
int base_priority;                  /* 优先级 */
struct list locks;                  /* 进程占有的锁*/
struct lock *lock_waiting;          /* 进程请求的锁 */
```
然后给lock增加以下成员

```c
struct list_elem elem;      /* 优先级捐赠队列 */
int max_priority;          /* 请求锁的进程中，最大的优先级 */
```
修改lock_acquire函数：

```c
void
lock_acquire (struct lock *lock)
{
  struct thread *current_thread = thread_current ();
  struct lock *l;
  enum intr_level old_level;

  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  if (lock->holder != NULL && !thread_mlfqs)
  {
    current_thread->lock_waiting = lock;
    l = lock;
    while (l && current_thread->priority > l->max_priority)
    {
      l->max_priority = current_thread->priority;
      thread_donate_priority (l->holder);
      l = l->holder->lock_waiting;
    }
  }

  sema_down (&lock->semaphore);

  old_level = intr_disable ();

  current_thread = thread_current ();
  if (!thread_mlfqs)
  {
    current_thread->lock_waiting = NULL;
    lock->max_priority = current_thread->priority;
    thread_hold_the_lock (lock);
  }
  lock->holder = current_thread;

  intr_set_level (old_level);
}
```
在P操作之前递归地实现优先级捐赠， 然后在被唤醒之后（此时这个线程已经拥有了这个锁），成为这个锁的拥有者。
这里优先级捐赠是通过直接修改锁的最高优先级， 然后调用update的时候把现成优先级更新实现的。

```c
/* Let thread hold a lock */
void
thread_hold_the_lock(struct lock *lock)
{
  enum intr_level old_level = intr_disable ();
  list_insert_ordered (&thread_current ()->locks, &lock->elem, lock_cmp_priority, NULL);

  if (lock->max_priority > thread_current ()->priority)
  {
    thread_current ()->priority = lock->max_priority;
    thread_yield ();
  }

  intr_set_level (old_level);
}
```

```c
/* Donate current priority to thread t. */
void
thread_donate_priority (struct thread *t)
{
  enum intr_level old_level = intr_disable ();
  thread_update_priority (t);

  if (t->status == THREAD_READY)
  {
    list_remove (&t->elem);
    list_insert_ordered (&ready_list, &t->elem, thread_cmp_priority, NULL);
  }
  intr_set_level (old_level);
}
```
锁队列排序函数lock_cmp_priority:

```c
/* lock comparation function */
bool
lock_cmp_priority (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
   return list_entry (a, struct lock, elem)->max_priority > list_entry (b, struct lock, elem)->max_priority;
}
```
然后在lock_release函数加入以下语句：

```c
if (!thread_mlfqs)
     thread_remove_lock (lock);
```
thread_remove_lock实现如下：

```c
/* Remove a lock. */
void
thread_remove_lock (struct lock *lock)
{
  enum intr_level old_level = intr_disable ();
  list_remove (&lock->elem);
  thread_update_priority (thread_current ());
  intr_set_level (old_level);
}
```
当释放掉一个锁的时候， 当前线程的优先级可能发生变化， 我们用thread_update_priority来处理这个逻辑：

```c
/* Update priority. */
void
thread_update_priority (struct thread *t)
{
  enum intr_level old_level = intr_disable ();
  int max_priority = t->base_priority;
  int lock_priority;

  if (!list_empty (&t->locks))
  {
    list_sort (&t->locks, lock_cmp_priority, NULL);
    lock_priority = list_entry (list_front (&t->locks), struct lock, elem)->max_priority;
    if (lock_priority > max_priority)
      max_priority = lock_priority;
  }

  t->priority = max_priority;
  intr_set_level (old_level);
}
```
这里如果这个线程还有锁， 就先获取这个线程拥有锁的最大优先级（可能被更高级线程捐赠）， 然后如果这个优先级比base_priority大的话更新的应该是被捐赠的优先级。然后在init_thread中加入初始化：

```c
t->base_priority = priority;
list_init (&t->locks);
t->lock_waiting = NULL;
```
修改一下thread_set_priority：

```c
void
thread_set_priority (int new_priority)
{
  if (thread_mlfqs)
    return;

  enum intr_level old_level = intr_disable ();

  struct thread *current_thread = thread_current ();
  int old_priority = current_thread->priority;
  current_thread->base_priority = new_priority;

  if (list_empty (&current_thread->locks) || new_priority > old_priority)
  {
    current_thread->priority = new_priority;
    thread_yield ();
  }

  intr_set_level (old_level);
}
```
然后把condition的队列改成优先级队列， 修改如下， 修改cond_signal函数：

```c
void
cond_signal (struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters))
  {
    list_sort (&cond->waiters, cond_sema_cmp_priority, NULL);
    sema_up (&list_entry (list_pop_front (&cond->waiters), struct semaphore_elem, elem)->semaphore);
  }
}
```
比较函数：

```c
/* cond sema comparation function */
bool
cond_sema_cmp_priority (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  struct semaphore_elem *sa = list_entry (a, struct semaphore_elem, elem);
  struct semaphore_elem *sb = list_entry (b, struct semaphore_elem, elem);
  return list_entry(list_front(&sa->semaphore.waiters), struct thread, elem)->priority > list_entry(list_front(&sb->semaphore.waiters), struct thread, elem)->priority;
}
```
然后把信号量的等待队列实现为优先级队列，修改sema_up：

```c
void
sema_up (struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters))
  {
    list_sort (&sema->waiters, thread_cmp_priority, NULL);
    thread_unblock (list_entry (list_pop_front (&sema->waiters), struct thread, elem));
  }

  sema->value++;
  thread_yield ();
  intr_set_level (old_level);
}
```
修改sema_down：

```c
void
sema_down (struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0)
    {
      list_insert_ordered (&sema->waiters, &thread_current ()->elem, thread_cmp_priority, NULL);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}
```
## 2.4 实验结果
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190508201653866.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQwNzQzMTQ0,size_16,color_FFFFFF,t_70)
# 3 Advanced Scheduler
## 3.1 实验准备
每个线程在-20到20之间都有一个nice值。每个线程的优先级介于0(PRI_MIN)到63(PRI_MAX)之间.每四个tick使用以下公式重新计算该优先级：
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)
recent_cpu的值是由线程收到recently的CPU时间计算得到的，在每个计时器的tick上，recenet_cpu的值加1，每一秒线程的CPU按以下公式更新：
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;recent_cpu = (2*load_avg)/(2*load_avg + 1) * recent_cpu + nice
load_avg估计在过去一分钟内准备运行的线程的平均数。它在启动时初始化为0，每秒重新计算一次，如下所示：
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;load_avg = (59/60)*load_avg + (1/60)*ready_threads.
其中ready_threads是更新时正在运行或准备运行的线程数（不包括空闲线程）。
简单来说，这里是维持了64个队列， 每个队列对应一个优先级， 从PRI_MIN到PRI_MAX。然后通过一些公式计算来计算出线程当前的优先级， 系统调度的时候会从高优先级队列开始选择线程执行， 这里线程的优先级随着操作系统的运转数据而动态改变。

由于涉及到了浮点数运算的问题， 以及pintos本身并没有实现这个， 需要我们自己进行实现
下表总结了fixed-point运算如何在C中实现。在表中，x和y是fixed-point，nis是整数，fixed-point是有符号p.q格式，其中p+q=31，f是1<<q： 
 &nbsp;|&nbsp;
---|---
Convert n to fixed point:|	n * f
Convert x to integer (rounding toward zero):|x / f
Convert x to integer (rounding to nearest)|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(x + f / 2) / f if x >= 0, &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(x - f / 2) / f if x <= 0.
Add x and y:|x + y
Subtract y from x:|	x - y
Add x and n:	|x + n * f
Subtract n from x:	|x - n * f
Multiply x by y:|	((int64_t) x) * y / f
Multiply x by n:|	x * n
Divide x by y:	|((int64_t) x) * f / y
Divide x by n:|	x / n
## 3.2实现思路
在timer_interrupt中固定一段时间计算更新线程的优先级，这里是每TIMER_FREQ时间更新一次系统load_avg和所有线程的recent_cpu， 每4个timer_ticks更新一次线程优先级， 每个timer_tick running线程的recent_cpu加一， 虽然这里说的是维持64个优先级队列调度， 其本质还是优先级调度， 我们保留之前写的优先级调度代码即可， 去掉优先级捐赠。
## 3.3实验代码
修改thread结构体，加入以下成员

```c
int nice;                           /* Niceness. */
fixed_t recent_cpu;                 /* Recent CPU. */
```
线程初始化时，初始化刚加入的成员变量

```c
t->nice = 0;
t->recent_cpu = FP_CONST (0);
```
在thread.c中加入全局变量fixed_t load_avg，并在thread_start中初始化

修改timer_interrupt，加入以下代码

```c
if (thread_mlfqs)
  {
    thread_mlfqs_increase_recent_cpu_by_one ();
    if (ticks % TIMER_FREQ == 0)
      thread_mlfqs_update_load_avg_and_recent_cpu ();
    else if (ticks % 4 == 0)
      thread_mlfqs_update_priority (thread_current ());
  }
```
然后，在thread.c中加入下列函数

```c
/* Increase recent_cpu by 1. */
void
thread_mlfqs_increase_recent_cpu_by_one (void)
{
  ASSERT (thread_mlfqs);
  ASSERT (intr_context ());

  struct thread *current_thread = thread_current ();
  if (current_thread == idle_thread)
    return;
  current_thread->recent_cpu = FP_ADD_MIX (current_thread->recent_cpu, 1);
}

```

```c
/* Every per second to refresh load_avg and recent_cpu of all threads. */
void
thread_mlfqs_update_load_avg_and_recent_cpu (void)
{
  ASSERT (thread_mlfqs);
  ASSERT (intr_context ());

  size_t ready_threads = list_size (&ready_list);
  if (thread_current () != idle_thread)
    ready_threads++;
  load_avg = FP_ADD (FP_DIV_MIX (FP_MULT_MIX (load_avg, 59), 60), FP_DIV_MIX (FP_CONST (ready_threads), 60));

  struct thread *t;
  struct list_elem *e = list_begin (&all_list);
  for (; e != list_end (&all_list); e = list_next (e))
  {
    t = list_entry(e, struct thread, allelem);
    if (t != idle_thread)
    {
      t->recent_cpu = FP_ADD_MIX (FP_MULT (FP_DIV (FP_MULT_MIX (load_avg, 2), FP_ADD_MIX (FP_MULT_MIX (load_avg, 2), 1)), t->recent_cpu), t->nice);
      thread_mlfqs_update_priority (t);
    }
  }
}
```

```c
/* Update priority. */
void
thread_mlfqs_update_priority (struct thread *t)
{
  if (t == idle_thread)
    return;

  ASSERT (thread_mlfqs);
  ASSERT (t != idle_thread);

  t->priority = FP_INT_PART (FP_SUB_MIX (FP_SUB (FP_CONST (PRI_MAX), FP_DIV_MIX (t->recent_cpu, 4)), 2 * t->nice));
  t->priority = t->priority < PRI_MIN ? PRI_MIN : t->priority;
  t->priority = t->priority > PRI_MAX ? PRI_MAX : t->priority;
}
```
修改thread.c中的以下函数：

```c
/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice)
{
  thread_current ()->nice = nice;
  thread_mlfqs_update_priority (thread_current ());
  thread_yield ();
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void)
{
  return thread_current ()->nice;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void)
{
  return FP_ROUND (FP_MULT_MIX (load_avg, 100));
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void)
{
  return FP_ROUND (FP_MULT_MIX (thread_current ()->recent_cpu, 100));
}
```
---
参考资料:
[1]:https://web.stanford.edu/class/cs140/projects/pintos/pintos_2.html#SEC15
[2]:https://www.cnblogs.com/laiy/p/pintos_project1_thread.html#timer_sleep
[3]:http://www.ccs.neu.edu/home/amislove/teaching/cs5600/fall10/pintos/pintos_7.html

