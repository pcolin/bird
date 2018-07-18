using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class PricerManager
    {
        public void OnProtoMessage(Proto.PricerRep rep)
        {
            lock (this.pricers)
            {
                foreach (var p in rep.Pricers)
                {
                    pricers.Add(p.Underlying, p);
                }
            }
        }

        public void OnProtoMessage(Proto.PricerReq req)
        {
            lock (this.pricers)
            {
                if (req.Type == Proto.RequestType.Set)
                {
                    foreach (var p in req.Pricers)
                    {
                        pricers[p.Underlying] = p;
                    }
                }
                else if (req.Type == Proto.RequestType.Del)
                {
                    foreach (var p in req.Pricers)
                    {
                        pricers.Remove(p.Underlying);
                    }
                }
            }
        }

        public Proto.Pricer GetPricer(string underlying)
        {
            Proto.Pricer ret = null;
            lock (this.pricers)
            {
                this.pricers.TryGetValue(underlying, out ret);
            }
            return ret;
        }

        public List<Proto.Pricer> GetPricers()
        {
            lock (this.pricers)
            {
                return this.pricers.Values.ToList();
            }
        }

        Dictionary<string, Proto.Pricer> pricers = new Dictionary<string, Proto.Pricer>();
    }
}
