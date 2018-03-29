using Microsoft.Practices.Unity;
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
                    ProductManager instruments = new ProductManager();
                    instruments.Add(rep.Instruments);
                    this.container.RegisterInstance<ProductManager>(instruments);
                    return true;
                }
            }

            return false;
        }

        private bool QueryPositions()
        {
            return false;
        }

        private RequestSocket socket;
        private Proto.Exchange exchange;
        private string user;
        private IUnityContainer container;
    }
}
