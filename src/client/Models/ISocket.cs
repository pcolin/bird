using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    public interface ISocket
    {
        void Initialize(string address);
        void Start();
        void Stop();
    }
}
