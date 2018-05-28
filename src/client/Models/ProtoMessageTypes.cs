using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class ProtoMessageTypes
    {
        public static readonly string Heartbeat = "Heartbeat";
        public static readonly string Price = "Price";
        public static readonly string Cash = "Cash";
        public static readonly string InterestRateReq = "InterestRateReq";
        public static readonly string SSRateReq = "SSRateReq";
        public static readonly string VolatilityCurveReq = "VolatilityCurveReq";
    }
}
