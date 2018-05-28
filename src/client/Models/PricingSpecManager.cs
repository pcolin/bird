using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class PricingSpecManager
    {
        public void OnProtoMessage(Proto.PricingSpecRep rep)
        {
            lock (this.pricings)
            {
                foreach (var p in rep.Pricings)
                {
                    pricings.Add(p.Underlying, p);
                }
            }
        }

        public Proto.PricingSpec GetPricingSpec(string underlying)
        {
            Proto.PricingSpec ret = null;
            lock (this.pricings)
            {
                this.pricings.TryGetValue(underlying, out ret);
            }
            return ret;
        }

        Dictionary<string, Proto.PricingSpec> pricings = new Dictionary<string, Proto.PricingSpec>();
    }
}
