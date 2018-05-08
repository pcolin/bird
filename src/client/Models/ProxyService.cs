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
            lock (this.actions)
            {
                List<Action<IMessage>> list = null;
                if (actions.TryGetValue(type, out list) == false)
                {
                    list = new List<Action<IMessage>>();
                    actions.Add(type, list);
                }
                list.Add(action);
            }
        }

        public void UnRegisterAction(string type, Action<IMessage> action)
        {
            lock (this.actions)
            {
                List<Action<IMessage>> list = null;
                if (actions.TryGetValue(type, out list))
                {
                    list.Remove(action);
                }
            }
        }

        private void ReceiveRun()
        {
            try
            {
                socket.Subscribe("");
                int serialNum = 0;
                while (running)
                {
                    byte[] bytes = socket.ReceiveImmediate();
                    if (bytes != null)
                    {
                        Array.Reverse(bytes, 0, 4);
                        int sn = BitConverter.ToInt32(bytes, 0);
                        ++serialNum;
                        if (serialNum != 1 && serialNum != sn)
                        {
                            /// !!!lost message!!!
                        }
                        serialNum = sn;
                        messages.Enqueue(bytes);
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
                    byte[] bytes = null;
                    if (messages.TryDequeue(out bytes))
                    {
                        int len = bytes[4];
                        IMessage msg = null;
                        string type = Encoding.UTF8.GetString(bytes, 11, len - 6);
                        if (type == "Heartbeat")
                        {
                            var heartbeat = Proto.Heartbeat.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                            /// to be done...
                        }
                        else if (type == "Price")
                        {
                            msg = Proto.Price.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        }
                        else if (type == "Cash")
                        {
                            msg = Proto.Cash.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        }
                        /// to be done...
                        
                        if (msg != null)
                        lock (this.actions)
                        {
                            List<Action<IMessage>> list = null;
                            if (actions.TryGetValue(type, out list))
                            {
                                foreach (var action in list)
                                {
                                    action(msg);
                                }
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
        private ConcurrentQueue<byte[]> messages = new ConcurrentQueue<byte[]>();
        private Dictionary<string, List<Action<IMessage>>> actions = new Dictionary<string,List<Action<IMessage>>>();
    }
}
