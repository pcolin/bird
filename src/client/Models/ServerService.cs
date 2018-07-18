using Google.Protobuf;
using NNanomsg.Protocols;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace client.Models
{
    class ServerService : ISocket
    {
        public void Initialize(string address)
        {
            socket = new RequestSocket();
            socket.Options.TcpNoDelay = true;
            socket.Options.ReceiveTimeout = new TimeSpan(0, 0, 10);
            socket.Connect(address);
        }

        public bool Start()
        {
            running = true;
            thread = new Thread(this.Run);
            //thread.IsBackground = true;
            thread.Start();

            /// Send PriceReq
            Proto.PriceReq req = new Proto.PriceReq();
            req.Type = Proto.RequestType.Get;
            req.User = this.User;
            Request(req);

            return true;
        }

        public void Stop()
        {
            running = false;
            if (thread != null)
            {
                thread.Abort();
                thread.Join();
            }
        }

        public Proto.Reply Login(string user, string password, string version)
        {
            Proto.Login login = new Proto.Login();
            login.User = user;
            //try
            //{
            //    IntPtr pwd = Marshal.SecureStringToBSTR(password);
            //    login.Password = Marshal.PtrToStringBSTR(pwd);
            //}
            //catch
            //{
            //    return null;
            //}
            login.Password = password;
            login.Role = Proto.Role.Trader;
            login.Ip = GetLocalIP();
            login.Mac = GetLocalMac();
            login.Version = version;

            byte[] bytes = ProtoMessageCoder.Encode(login);
            socket.Send(bytes);

            this.User = user;
            this.lastSentTime = DateTime.Now;

            bytes = socket.Receive();
            if (bytes != null)
            {
                return ProtoMessageCoder.Decode<Proto.Reply>(bytes);
            }
            return null;
        }

        public Proto.Reply Logout(string user)
        {
            Proto.Logout logout = new Proto.Logout();
            logout.User = user;
            byte[] bytes = ProtoMessageCoder.Encode(logout);
            socket.Send(bytes);            
            bytes = socket.Receive();
            if (bytes != null)
            {
                return ProtoMessageCoder.Decode<Proto.Reply>(bytes);
            }
            return null; ;
        }

        public string User { get; set; }

        public void Request(IMessage req)
        {
            messages.Enqueue(req);
        }

        private void Run()
        {
            const int TimeoutInterval = 5;
            Proto.Heartbeat heartbeat = new Proto.Heartbeat();
            heartbeat.Type = Proto.ProcessorType.Gui;
            heartbeat.Name = this.User;
            try
            {
                while (running)
                {
                    IMessage msg = null;
                    if (messages.TryDequeue(out msg))
                    {
                        RequestReply(msg);
                    }
                    else if ((DateTime.Now - lastSentTime).TotalSeconds >= TimeoutInterval)
                    {
                        RequestReply(heartbeat);
                    }
                    else
                    {
                        System.Threading.Thread.Sleep(10);
                    }
                }
            }
            catch (ThreadAbortException)
            {
                return;
            }
        }

        void RequestReply(IMessage msg)
        {
            for (int i = 0; i < 3; ++i)
            {
                var bytes = ProtoMessageCoder.Encode(msg);
                socket.Send(bytes);
                lastSentTime = DateTime.Now;
                var reply_bytes = socket.Receive();
                if (reply_bytes != null)
                {
                    var reply = Proto.Reply.Parser.ParseFrom(reply_bytes, 16, reply_bytes.Count() - 16);
                    if (reply != null && reply.Result)
                    {
                        return;
                    }
                }
            }
        }
        
        string GetLocalIP()
        {
            IPHostEntry ipEntry = Dns.GetHostEntry(Dns.GetHostName());
            foreach (IPAddress ip in ipEntry.AddressList)
            {
                if (ip.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                    return ip.ToString();
            }
            return string.Empty;
        }

        string GetLocalMac()
        {
            if (NetworkInterface.GetIsNetworkAvailable())
            {
                return NetworkInterface.GetAllNetworkInterfaces()[0].GetPhysicalAddress().ToString();
            }
            return string.Empty;
        }

        private volatile bool running = false;
        private RequestSocket socket;
        private ConcurrentQueue<IMessage> messages = new ConcurrentQueue<IMessage>();
        private Thread thread;

        private DateTime lastSentTime;
    }
}
