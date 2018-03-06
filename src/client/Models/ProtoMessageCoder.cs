using Google.Protobuf;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    public class ProtoMessageCoder
    {
        public static byte[] Encode(IMessage msg)
        {
            string type = msg.GetType().ToString();
            int len = type.Count();
            byte[] buffer = new byte[4 + len + msg.CalculateSize()];
            MemoryStream ms = new MemoryStream(buffer);
            int networkLen = IPAddress.HostToNetworkOrder(len);
            ms.Write(BitConverter.GetBytes(networkLen), 0, 4);
            ms.Write(Encoding.UTF8.GetBytes(type), 0, len);
            CodedOutputStream cos = new CodedOutputStream(ms);
            msg.WriteTo(cos);
            cos.Flush();
            return buffer;
        }

        public static T Decode<T>(byte[] bytes) where T : IMessage<T>, new()
        {
            Array.Reverse(bytes, 0, 4);
            int len = BitConverter.ToInt32(bytes, 0) + 4;
            MessageParser<T> parser = new MessageParser<T>(()=>new T());
            
            return parser.ParseFrom(bytes, len, bytes.Length - len);
        }
    }
}
