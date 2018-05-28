using Prism.Events;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    public class ImpliedVolatilityData
    {
        public Option Option { get; set; }
        public double LastIV { get; set; }
        public double BidIV { get; set; }
        public double AskIV { get; set; }
    }

    class ImpliedVolatilityEvent : PubSubEvent<ImpliedVolatilityData>
    {
    }
}
