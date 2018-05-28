using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class SSRateManager
    {
        public void OnProtoMessage(Proto.SSRateRep rep)
        {
            lock (this.ssrates)
            {
                //this.ssrates.Clear();
                foreach (var r in rep.Rates)
                {
                    SortedList<DateTime, double> tmp = null;
                    if (ssrates.TryGetValue(r.Underlying, out tmp) == false)
                    {
                        tmp = new SortedList<DateTime,double>();
                        ssrates.Add(r.Underlying, tmp);
                    }
                    try
                    {
                        tmp[DateTime.ParseExact(r.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture)] = r.Rate;
                    }
                    catch (Exception) { }
                }
            }
        }

        public void OnProtoMessage(Proto.SSRateReq req)
        {
            lock (this.ssrates)
            {
                if (req.Type == Proto.RequestType.Set)
                {
                    foreach (var r in req.Rates)
                    {
                        SortedList<DateTime, double> tmp = null;
                        if (ssrates.TryGetValue(r.Underlying, out tmp) == false)
                        {
                            tmp = new SortedList<DateTime, double>();
                            ssrates.Add(r.Underlying, tmp);
                        }
                        try
                        {
                            tmp[DateTime.ParseExact(r.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture)] = r.Rate;
                        }
                        catch (Exception) { }
                    }
                }
            }
        }

        public double? GetSSRate(string underlying, DateTime maturity)
        {
            lock (this.ssrates)
            {
                SortedList<DateTime, double> lists = null;
                double ssr = 0;
                if (ssrates.TryGetValue(underlying, out lists) && lists.TryGetValue(maturity, out ssr))
                {
                    return ssr;
                }
            }
            return null;
        }

        private Dictionary<string, SortedList<DateTime, double>> ssrates = new Dictionary<string, SortedList<DateTime, double>>();
    }
}
