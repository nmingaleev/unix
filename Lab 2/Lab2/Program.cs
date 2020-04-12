using System;
using System.Threading;
using System.Collections.Generic;

namespace Lab2
{
    class ProducerConsumerQueue: IDisposable
    {
        // Класс AutoResetEvent, как и Monitor, служит цели синхронизации потоков.
        // Этот класс позволяет переключать объект-событие из сигнального в несигнальное состояние.
        EventWaitHandle _wh = new AutoResetEvent(false);

        // Новый поток.
        Thread _worker;

        // Объект _locker нужен для синхронизации потоков.
        readonly object _locker = new object();

        // Очередь из задач.
        Queue<string> _tasks = new Queue<string>();

        public ProducerConsumerQueue()
        {
            // Создаем новый поток, который будет выполнять метод Work.
            _worker = new Thread(Work);
            // Запускаем поток.
            _worker.Start();
        }

       // Метод для добавления задач в очередь.
        public void EnqueueTask(string task)
        {
            lock (_locker)
            {
                _tasks.Enqueue(task);
            }

            // После завершения работы с очередью вызываем метод Set, который оповещает все лжидающие потоки,
            // что объект _wh находится в сигнальном состоянии, и его может захватить другой поток.
            _wh.Set();
        }

        public void Dispose()
        {
            EnqueueTask(null); // Говорим потребителю, что пора заканчивать.
            _worker.Join(); // Ждем, пока поток потребителя не закончит свою работу.
            _wh.Close(); // Освобождаем ресурсы ОС.
        }

        void Work()
        {
            while (true)
            {
                string task = null;

                // Блокируем очередь.
                lock (_locker)
                {
                    // Если в очереди есть задание, то достаем его.
                    if (_tasks.Count > 0)
                    {
                        task = _tasks.Dequeue();

                        // Если отправлен сигнал на завершение, то выходим из цикла.
                        if (task == null)
                        {
                            return;
                        }
                    }
                }

                if (task != null)
                {
                    Console.WriteLine("Consumer says: " + task);
                } else
                {
                    _wh.WaitOne(); // Задач нет, ожидаем поступления сигнала.
                }
            }
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            // При выходе из using будет вызван метод ProducerConsumerQueue.Dispose()
            using (ProducerConsumerQueue q = new ProducerConsumerQueue())
            {
                for (int i = 0; i < 10; i++)
                {
                    Console.WriteLine("Provider says: " + i);

                    q.EnqueueTask(i.ToString());

                    Thread.Sleep(1000);
                }
            }
        }
    }
}
