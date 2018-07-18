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
            //socket.Shutdown()
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

        void OnHeartbeat(byte[] bytes, int pos)
        {
            var h = Proto.Heartbeat.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (h != null)
            {
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Heartbeat>>().Publish(h);
            }
        }

        void OnPrice(byte[] bytes, int pos)
        {
            var p = Proto.Price.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (p != null)
            {
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Price>>().Publish(p);
            }
        }

        void OnOrderReq(byte[] bytes, int pos)
        {
            var o = Proto.OrderReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (o != null)
            {
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.OrderReq>>().Publish(o);
            }
        }

        void OnPositionReq(byte[] bytes, int pos)
        {
            var p = Proto.PositionReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (p != null)
            {
                this.positionManager.OnProtoMessage(p);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.PositionReq>>().Publish(p);
            }
        }

        void OnCash(byte[] bytes, int pos)
        {
            var c = Proto.Cash.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (c != null)
            {
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Cash>>().Publish(c);
            }
        }

        void OnInstrumentReq(byte[] bytes, int pos)
        {
            var r = Proto.InstrumentReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (r != null)
            {
                this.productManger.OnProtoMessage(r);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.InstrumentReq>>().Publish(r);
            }
        }

        void OnExchangeParameterReq(byte[] bytes, int pos)
        {
            var e = Proto.ExchangeParameterReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (e != null)
            {
                this.exchangeManager.OnProtoMessage(e);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ExchangeParameterReq>>().Publish(e);
            }
        }

        void OnInterestRateReq(byte[] bytes, int pos)
        {
            var r = Proto.InterestRateReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (r != null)
            {
                this.interestManager.OnProtoMessage(r);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.InterestRateReq>>().Publish(r);
            }
        }

        void OnSSRateReq(byte[] bytes, int pos)
        {
            var s = Proto.SSRateReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (s != null)
            {
                this.ssrateManager.OnProtoMessage(s);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.SSRateReq>>().Publish(s);
            }
        }

        void OnVolatilityCurveReq(byte[] bytes, int pos)
        {
            var v = Proto.VolatilityCurveReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (v != null)
            {
                this.volatilityManager.OnProtoMessage(v);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.VolatilityCurveReq>>().Publish(v);
            }
        }

        void OnDestrikerReq(byte[] bytes, int pos)
        {
            var d = Proto.DestrikerReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (d != null)
            {
                this.destrikerManager.OnProtoMessage(d);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.DestrikerReq>>().Publish(d);
            }
        }

        void OnCreditReq(byte[] bytes, int pos)
        {
            var c = Proto.CreditReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (c != null)
            {
                this.creditManager.OnProtoMessage(c);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.CreditReq>>().Publish(c);
            }
        }

        void OnPricerReq(byte[] bytes, int pos)
        {
            var p = Proto.PricerReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (p != null)
            {
                this.pricerManager.OnProtoMessage(p);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.PricerReq>>().Publish(p);
            }
        }

        void OnStrategySwitchReq(byte[] bytes, int pos)
        {
            var s = Proto.StrategySwitchReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (s != null)
            {
                this.strategyManager.OnProtoMessage(s);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.StrategySwitchReq>>().Publish(s);
            }
        }

        void OnStrategyStatistic(byte[] bytes, int pos)
        {
            var s = Proto.StrategyStatistic.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (s != null)
            {
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.StrategyStatistic>>().Publish(s);
            }
        }

        void OnQuoterReq(byte[] bytes, int pos)
        {
            var q = Proto.QuoterReq.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (q != null)
            {
                this.quoterManager.OnProtoMessage(q);
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.QuoterReq>>().Publish(q);
            }
        }

        void OnServerInfo(byte[] bytes, int pos)
        {
            var i = Proto.ServerInfo.Parser.ParseFrom(bytes, pos, bytes.Count() - pos);
            if (i != null)
            {
                this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ServerInfo>>().Publish(i);
            }
        }

        private void ActionRun()
        {
            try
            {
                var actions = new Dictionary<string, Action<byte[], int>>()
                    {
                        {"Heartbeat", (bytes, pos) => OnHeartbeat(bytes, pos)},
                        {"Price", (bytes, pos) => OnPrice(bytes, pos)},
                        {"OrderReq", (bytes, pos) => OnOrderReq(bytes, pos)},
                        {"PositionReq", (bytes, pos) => OnPositionReq(bytes, pos)},
                        {"Cash", (bytes, pos) => OnCash(bytes, pos)},
                        {"InstrumentReq", (bytes, pos) => OnInstrumentReq(bytes, pos)},
                        {"ExchangeParameterReq", (bytes, pos) => OnExchangeParameterReq(bytes, pos)},
                        {"InterestRateReq", (bytes, pos) => OnInterestRateReq(bytes, pos)},
                        {"SSRateReq", (bytes, pos) => OnSSRateReq(bytes, pos)},
                        {"VolatilityCurveReq", (bytes, pos) => OnVolatilityCurveReq(bytes, pos)},
                        {"DestrikerReq", (bytes, pos) => OnDestrikerReq(bytes, pos)},
                        {"CreditReq", (bytes, pos) => OnCreditReq(bytes, pos)},
                        {"PricerReq", (bytes, pos) => OnPricerReq(bytes, pos)},
                        {"StrategySwitchReq", (bytes, pos) => OnStrategySwitchReq(bytes, pos)},
                        {"StrategyStatistic", (bytes, pos) => OnStrategyStatistic(bytes, pos)},
                        {"QuoterReq", (bytes, pos) => OnQuoterReq(bytes, pos)},
                        {"ServerInfo", (bytes, pos) => OnServerInfo(bytes, pos)},
                        /// to be done...
                    };
                var exch = this.exchange.ToString();
                this.productManger = this.container.Resolve<ProductManager>(exch);
                this.exchangeManager = this.container.Resolve<ExchangeParameterManager>(exch);
                this.interestManager = this.container.Resolve<InterestRateManager>(exch);
                this.ssrateManager = this.container.Resolve<SSRateManager>(exch);
                this.volatilityManager = this.container.Resolve<VolatilityCurveManager>(exch);
                this.creditManager = this.container.Resolve<CreditManager>(exch);
                this.pricerManager = this.container.Resolve<PricerManager>(exch);
                this.quoterManager = this.container.Resolve<QuoterManager>(exch);
                this.positionManager = this.container.Resolve<PositionManager>(exch);
                this.destrikerManager = this.container.Resolve<DestrikerManager>(exch);
                this.strategyManager = this.container.Resolve<StrategySwitchManager>(exch);
                while (running)
                {
                    byte[] bytes = messages.Take();                     
                    int len = bytes[4];
                    string type = Encoding.UTF8.GetString(bytes, 11, len - 6);
                    Action<byte[], int> action = null;
                    if (actions.TryGetValue(type, out action))
                    {
                        action(bytes, len + 5);
                    }                        
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

        private IUnityContainer container;
        private Proto.Exchange exchange;

        private ProductManager productManger;
        private ExchangeParameterManager exchangeManager;
        private InterestRateManager interestManager;
        private SSRateManager ssrateManager;
        private VolatilityCurveManager volatilityManager;
        private CreditManager creditManager;
        private PricerManager pricerManager;
        private QuoterManager quoterManager;
        private PositionManager positionManager;
        private DestrikerManager destrikerManager;
        private StrategySwitchManager strategyManager;
    }
}
