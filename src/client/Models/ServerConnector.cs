using NNanomsg.Protocols;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    public interface IConnector
    {
        void Initialize();
        bool Login(string user, string passwd);
    }

    public class ServerConnector : IConnector
    {
        public bool Login(string user, string passwd)
        {
            Proto.Login login = new Proto.Login();
            login.User = user;
            login.Password = passwd;
            login.Role = Proto.Role.Trader;
            login.Ip = GetLocalIP();
            login.Mac = GetLocalMac();
            login.Version = "0.0.2";
            using (MemoryStream ms = new MemoryStream())
            {
                string type = login.GetType().ToString();
                int len = type.Count();
                int nLen = IPAddress.HostToNetworkOrder(len);
                ms.Write(BitConverter.GetBytes(nLen), 0, 4);
                byte[] bType = System.Text.Encoding.UTF8.GetBytes(type);
                ms.Write(bType, 0, type.Count());
                Google.Protobuf.CodedOutputStream stream = new Google.Protobuf.CodedOutputStream(ms);
                login.WriteTo(stream);
                stream.Flush();
                byte[] send = ms.ToArray();
                //send[4] = 112;
                serverSocket.Send(send);
                byte[] ret = serverSocket.Receive();
                Array.Reverse(ret, 0, 4);
                len = BitConverter.ToInt32(ret, 0) + 4;
                Proto.LoginRep reply = Proto.LoginRep.Parser.ParseFrom(ret, len, ret.Length - len);

                //Google.Protobuf.MessageParser parse = new Google.Protobuf.MessageParser();
                //var msg = parse.ParseFrom(ret, len, ret.Length - len);
                
                return reply.Result;
            }
        }

        public void Initialize()
        {
            serverSocket = new RequestSocket();
            serverSocket.Connect("tcp://172.28.1.53:8005");
        }

        public string GetLocalIP()
        {
            IPHostEntry ipEntry = Dns.GetHostEntry(Dns.GetHostName());
            foreach (IPAddress ip in ipEntry.AddressList)
            {
                if (ip.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                    return ip.ToString();
            }
            return string.Empty;
        }

        public string GetLocalMac()
        {
            if (NetworkInterface.GetIsNetworkAvailable())
            {
                return NetworkInterface.GetAllNetworkInterfaces()[0].GetPhysicalAddress().ToString();
            }
            return string.Empty;
        }

        private RequestSocket serverSocket;
        private RequestSocket databaseSocket;
        private SubscribeSocket proxySocket;
    }
}
