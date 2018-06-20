using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class DestrikerManager
    {
        public void OnProtoMessage(Proto.DestrikerRep rep)
        {
            lock (this.destrikers)
            {
                foreach (var d in rep.Destrikers)
                {
                    this.destrikers.Add(d.Instrument, d.Destriker_);
                }
            }
        }

        public void OnProtoMessage(Proto.DestrikerReq req)
        {
            lock (this.destrikers)
            {
                if (req.Type == Proto.RequestType.Set)
                {
                    foreach (var d in req.Destrikers)
                    {
                        destrikers[d.Instrument] = d.Destriker_;
                    }
                }
            }
        }

        public double GetDestriker(string option)
        {
            double d = 0;
            lock (this.destrikers)
            {
                this.destrikers.TryGetValue(option, out d);
            }
            return d;
        }

        Dictionary<string, double> destrikers = new Dictionary<string, double>();
    }
}
