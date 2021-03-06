﻿using Microsoft.Practices.Unity;
using NNanomsg.Protocols;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class DatabaseService : ISocket
    {
        public DatabaseService(IUnityContainer container, Proto.Exchange exchange, string user)
        {
            this.container = container;
            this.exchange = exchange;
            this.user = user;
        }

        public void Initialize(string address)
        {
            socket = new RequestSocket();
            socket.Options.TcpNoDelay = true;
            socket.Options.ReceiveTimeout = new TimeSpan(0, 0, 5);
            socket.Connect(address);
        }

        public bool Start()
        {
            bool ok = QueryInstruments();
            ok = ok && QueryPositions();
            ok = ok && QueryOrders();
            ok = ok && QueryTrades();
            ok = ok && QueryExchangeParameters();
            ok = ok && QueryInterestRates();
            ok = ok && QuerySSRates();
            ok = ok && QueryVolatilityCurves();
            ok = ok && QueryDestrikers();
            ok = ok && QueryCredits();
            ok = ok && QueryPricers();
            ok = ok && QueryQuoters();
            ok = ok && QueryStrategySwitches();
            return ok;
        }

        public void Stop()
        {
        }

        private bool QueryInstruments()
        {
            Proto.InstrumentReq req = new Proto.InstrumentReq();
            req.Type = Proto.RequestType.Get;
            req.Exchange = exchange;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.InstrumentRep>(bytes);
                if (rep.Result.Result)
                {
                    ProductManager pm = new ProductManager();
                    pm.OnProtoMessage(rep);
                    this.container.RegisterInstance<ProductManager>(exchange.ToString(), pm);
                    return true;
                }
            }

            return false;
        }

        private bool QueryPositions()
        {
            Proto.PositionReq req = new Proto.PositionReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.PositionRep>(bytes);
                if (rep.Result.Result)
                {
                    PositionManager positions = new PositionManager();
                    positions.OnProtoMessage(rep);
                    this.container.RegisterInstance<PositionManager>(exchange.ToString(), positions);
                    return true;
                }
            }
            return false;
        }

        private bool QueryOrders()
        {
            Proto.OrderReq req = new Proto.OrderReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.OrderRep>(bytes);
                if (rep.Result.Result)
                {
                    OrderManager manager = new OrderManager();
                    manager.OnProtoMessage(rep);
                    this.container.RegisterInstance<OrderManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private bool QueryTrades()
        {
            Proto.TradeReq req = new Proto.TradeReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.TradeRep>(bytes);
                if (rep.Result.Result)
                {
                    TradeManager manager = new TradeManager();
                    manager.OnProtoMessage(rep);
                    this.container.RegisterInstance<TradeManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private bool QueryExchangeParameters()
        {
            Proto.ExchangeParameterReq req = new Proto.ExchangeParameterReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.ExchangeParameterRep>(bytes);
                if (rep.Result.Result)
                {
                    ExchangeParameterManager manager = new ExchangeParameterManager();
                    manager.OnProtoMessage(rep);
                    this.container.RegisterInstance<ExchangeParameterManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private bool QueryInterestRates()
        {
            Proto.InterestRateReq req = new Proto.InterestRateReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.InterestRateRep>(bytes);
                if (rep.Result.Result)
                {
                    InterestRateManager manager = new InterestRateManager();
                    manager.OnProtoMessage(rep);
                    this.container.RegisterInstance<InterestRateManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private bool QuerySSRates()
        {
            Proto.SSRateReq req = new Proto.SSRateReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.SSRateRep>(bytes);
                if (rep.Result.Result)
                {
                    SSRateManager manager = new SSRateManager();
                    manager.OnProtoMessage(rep);
                    this.container.RegisterInstance<SSRateManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private bool QueryVolatilityCurves()
        {
            Proto.VolatilityCurveReq req = new Proto.VolatilityCurveReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.VolatilityCurveRep>(bytes);
                if (rep.Result.Result)
                {
                    VolatilityCurveManager manager = new VolatilityCurveManager();
                    if (rep.Curves.Count > 0)
                    {
                        manager.OnProtoMessage(rep);
                    }
                    this.container.RegisterInstance<VolatilityCurveManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private bool QueryDestrikers()
        {
            Proto.DestrikerReq req = new Proto.DestrikerReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;
            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.DestrikerRep>(bytes);
                if (rep.Result.Result)
                {
                    DestrikerManager manager = new DestrikerManager();
                    if (rep.Destrikers.Count > 0)
                    {
                        manager.OnProtoMessage(rep);
                    }
                    this.container.RegisterInstance<DestrikerManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private bool QueryCredits()
        {
            Proto.CreditReq req = new Proto.CreditReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.CreditRep>(bytes);
                if (rep.Result.Result)
                {
                    var manager = new CreditManager();
                    if (rep.Credits.Count > 0)
                    {
                        manager.OnProtoMessage(rep);
                    }
                    this.container.RegisterInstance<CreditManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private bool QueryPricers()
        {
            Proto.PricerReq req = new Proto.PricerReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.PricerRep>(bytes);
                if (rep.Result.Result)
                {
                    var manager = new PricerManager();
                    if (rep.Pricers.Count > 0)
                    {
                        manager.OnProtoMessage(rep);
                    }
                    this.container.RegisterInstance<PricerManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }
        
        private bool QueryQuoters()
        {
            Proto.QuoterReq req = new Proto.QuoterReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.QuoterRep>(bytes);
                if (rep.Result.Result)
                {
                    var manager = new QuoterManager();
                    if (rep.Quoters.Count > 0)
                    {
                        manager.OnProtoMessage(rep);
                    }
                    this.container.RegisterInstance<QuoterManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private bool QueryStrategySwitches()
        {
            Proto.StrategySwitchReq req = new Proto.StrategySwitchReq();
            req.Type = Proto.RequestType.Get;
            req.User = user;

            byte[] bytes = ProtoMessageCoder.Encode(req);
            socket.Send(bytes);

            bytes = socket.Receive();
            if (bytes != null)
            {
                var rep = ProtoMessageCoder.Decode<Proto.StrategySwitchRep>(bytes);
                if (rep.Result.Result)
                {
                    var manager = new StrategySwitchManager();
                    if (rep.Switches.Count > 0)
                    {
                        manager.OnProtoMessage(rep);
                    }
                    this.container.RegisterInstance<StrategySwitchManager>(exchange.ToString(), manager);
                    return true;
                }
            }
            return false;
        }

        private RequestSocket socket;
        private Proto.Exchange exchange;
        private string user;
        private IUnityContainer container;
    }
}
