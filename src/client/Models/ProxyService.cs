using Google.Protobuf;
using NNanomsg.Protocols;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace client.Models
{
    class ProxyService : ISocket
    {
        public void Initialize(string address)
        {
            socket = new SubscribeSocket();
            socket.Options.TcpNoDelay = true;
            socket.Connect(address);
        }

        public bool Start()
        {
            running = true;
            actionThread = new Thread(this.ActionRun);
            //actionThread.IsBackground = true;
            actionThread.Start();
            receiveThread = new Thread(this.ReceiveRun);
            //receiveThread.IsBackground = true;
            receiveThread.Start();

            return true;
        }

        public void Stop()
        {
            running = false;
            if (receiveThread != null)
            {
                receiveThread.Abort();
                receiveThread.Join();
            }
            if (actionThread != null)
            {
                actionThread.Abort();
                actionThread.Join();
            }
        }

        public void RegisterAction(string type, Action<IMessage> action)
        {
            List<Action<IMessage>> list = null;
            if (actions.TryGetValue(type, out list) == false)
            {
                list = new List<Action<IMessage>>();
                actions.Add(type, list);
            }
            list.Add(action);
        }

        public void UnRegisterAction(string type, Action<IMessage> action)
        {
            List<Action<IMessage>> list = null;
            if (actions.TryGetValue(type, out list))
            {
                list.Remove(action);
            }
        }

        private void ReceiveRun()
        {
            try
            {
                while (running)
                {
                    byte[] bytes = socket.ReceiveImmediate();
                    if (bytes != null)
                    {
                        Array.Reverse(bytes, 0, 4);
                        int len = BitConverter.ToInt32(bytes, 0) + 4;
                        string type = Encoding.UTF8.GetString(bytes, 10, len - 6);
                        if (type == "Heartbeat")
                        {
                            var heartbeat = Proto.Heartbeat.Parser.ParseFrom(bytes, 4 + len, bytes.Count() - 4 - len);
                            messages.Enqueue(heartbeat);
                        }
                        else if (type == "Price")
                        {
                            var price = Proto.Price.Parser.ParseFrom(bytes, 4 + len, bytes.Count() - 4 - len);
                            messages.Enqueue(price);
                        }
                        /// to be done...
                    }
                    else
                    {
                        Thread.Sleep(50);
                    }
                }
            }
            catch (ThreadAbortException)
            {
                //socket.Shutdown();
                return;
            }
        }

        private void ActionRun()
        {
            try
            {
                while (running)
                {
                    IMessage msg = null;
                    if (messages.TryDequeue(out msg))
                    {
                        List<Action<IMessage>> list = null;
                        if (actions.TryGetValue(msg.GetType().ToString(), out list))
                        {
                            foreach (var action in list)
                            {
                                action(msg);
                            }
                        }
                    }
                }
            }
            catch (ThreadAbortException)
            {
                return;
            }
        }

        private volatile bool running = false;
        private Thread receiveThread;
        private Thread actionThread;
        private SubscribeSocket socket;
        private ConcurrentQueue<IMessage> messages = new ConcurrentQueue<IMessage>();
        private Dictionary<string, List<Action<IMessage>>> actions = new Dictionary<string,List<Action<IMessage>>>();
    }
}
