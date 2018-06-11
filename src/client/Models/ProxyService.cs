using Google.Protobuf;
using Microsoft.Practices.Unity;
using NNanomsg.Protocols;
using Prism.Events;
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
        public ProxyService(IUnityContainer container, Proto.Exchange exchange)
        {
            this.container = container;
            this.exchange = exchange;
        }

        public void Initialize(string address)
        {
            socket = new SubscribeSocket();
            socket.Options.TcpNoDelay = true;
            socket.Options.ReceiveBuffer = 10000;
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

        //public void RegisterAction(string type, Action<IMessage> action)
        //{
        //    lock (this.actions)
        //    {
        //        List<Action<IMessage>> list = null;
        //        if (actions.TryGetValue(type, out list) == false)
        //        {
        //            list = new List<Action<IMessage>>();
        //            actions.Add(type, list);
        //        }
        //        list.Add(action);
        //    }
        //}

        //public void UnRegisterAction(string type, Action<IMessage> action)
        //{
        //    lock (this.actions)
        //    {
        //        List<Action<IMessage>> list = null;
        //        if (actions.TryGetValue(type, out list))
        //        {
        //            list.Remove(action);
        //        }
        //    }
        //}

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
                            var err = new Proto.ServerInfo()
                                {
                                    Time = (ulong)((DateTime.Now - new DateTime(1970, 1, 1, 8, 0, 0)).TotalSeconds),
                                    Type = Proto.ServerInfo.Types.Type.Warn,
                                    Exchange = this.exchange,
                                    Info = "Lost proxy messages(" + serialNum + ", " + (sn - 1) + ")"
                                };
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ServerInfo>>().Publish(err);
                        }
                        serialNum = sn;
                        messages.Add(bytes);
                    }
                    else
                    {
                        Thread.Sleep(10);
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
                var exch = this.exchange.ToString();
                var pm = this.container.Resolve<ProductManager>(exch);
                var em = this.container.Resolve<ExchangeParameterManager>(exch);
                var im = this.container.Resolve<InterestRateManager>(exch);
                var sm = this.container.Resolve<SSRateManager>(exch);
                var vm = this.container.Resolve<VolatilityCurveManager>(exch);
                if (vm == null) return;
                while (running)
                {
                    byte[] bytes = messages.Take();
                     
                    int len = bytes[4];
                    //IMessage msg = null;
                    string type = Encoding.UTF8.GetString(bytes, 11, len - 6);
                    if (type == "Heartbeat")
                    {
                        var heartbeat = Proto.Heartbeat.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        /// to be done...
                    }
                    else if (type == "Price")
                    {
                        var p = Proto.Price.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        if (p != null)
                        {
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Price>>().Publish(p);
                        }
                    }
                    else if (type == "Cash")
                    {
                        var c = Proto.Cash.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        if (c != null)
                        {
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Cash>>().Publish(c);
                        }
                    }
                    else if (type == "InstrumentReq")
                    {
                        var r = Proto.InstrumentReq.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        if (r != null)
                        {
                            pm.OnProtoMessage(r);
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.InstrumentReq>>().Publish(r);
                        }
                    }
                    else if (type == "ExchangeParameterReq")
                    {
                        var e = Proto.ExchangeParameterReq.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        if (e != null && em != null)
                        {
                            em.OnProtoMessage(e);
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ExchangeParameterReq>>().Publish(e);
                        }
                    }
                    else if (type == "InterestRateReq")
                    {
                        var r = Proto.InterestRateReq.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        if (r != null && im != null)
                        {
                            im.OnProtoMessage(r);
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.InterestRateReq>>().Publish(r);
                        }
                    }
                    else if (type == "SSRateReq")
                    {
                        var s = Proto.SSRateReq.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        if (s != null && sm != null)
                        {
                            sm.OnProtoMessage(s);
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.SSRateReq>>().Publish(s);
                        }
                    }
                    else if (type == "VolatilityCurveReq")
                    {
                        var v = Proto.VolatilityCurveReq.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        if (v != null)
                        {
                            vm.OnProtoMessage(v);
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.VolatilityCurveReq>>().Publish(v);
                        }
                    }
                    else if (type == "ServerInfo")
                    {
                        var i = Proto.ServerInfo.Parser.ParseFrom(bytes, len + 5, bytes.Count() - len - 5);
                        if (i != null)
                        {
                            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ServerInfo>>().Publish(i);
                        }
                    }
                    /// to be done...
                        
                }
            }
            catch (InvalidOperationException)
            {
                return;
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
        private BlockingCollection<byte[]> messages = new BlockingCollection<byte[]>();
        //private Dictionary<string, List<Action<IMessage>>> actions = new Dictionary<string,List<Action<IMessage>>>();

        private IUnityContainer container;
        private Proto.Exchange exchange;
    }
}
