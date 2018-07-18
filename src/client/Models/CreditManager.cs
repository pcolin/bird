using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class CreditManager
    {
        public void OnProtoMessage(Proto.CreditRep rep)
        {
            lock (this.mutex)
            {
                //this.curves.Clear();
                foreach (var c in rep.Credits)
                {
                    int idx = (int)c.Strategy;
                    SortedList<DateTime, Proto.Credit> tmp = null;
                    if (this.credits[idx].TryGetValue(c.Underlying, out tmp) == false)
                    {
                        tmp = new SortedList<DateTime, Proto.Credit>();
                        this.credits[idx].Add(c.Underlying, tmp);
                    }
                    try
                    {
                        tmp.Add(DateTime.ParseExact(c.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture), c);
                    }
                    catch (Exception) { }
                }
            }
        }

        public void OnProtoMessage(Proto.CreditReq req)
        {
            lock (this.mutex)
            {
                if (req.Type == Proto.RequestType.Set)
                {
                    foreach (var c in req.Credits)
                    {
                        int idx = (int)c.Strategy;
                        SortedList<DateTime, Proto.Credit> tmp = null;
                        if (this.credits[idx].TryGetValue(c.Underlying, out tmp) == false)
                        {
                            tmp = new SortedList<DateTime, Proto.Credit>();
                            this.credits[idx].Add(c.Underlying, tmp);
                        }
                        try
                        {
                            tmp[DateTime.ParseExact(c.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture)] = c;
                        }
                        catch (Exception) { }
                    }
                }
            }
        }

        public Proto.Credit GetCredit(Proto.StrategyType strategy, string underlying, DateTime maturity)
        {
            SortedList<DateTime, Proto.Credit> tmp = null;
            Proto.Credit credit = null;
            lock (this.mutex)
            {
                if (this.credits[(int)strategy].TryGetValue(underlying, out tmp) && tmp.TryGetValue(maturity, out credit)) { }
            }
            return credit;
        }

        Dictionary<string, SortedList<DateTime, Proto.Credit>>[] credits = new Dictionary<string, SortedList<DateTime, Proto.Credit>>[(int)Proto.StrategyType.DummyQuoter]
            {
                new Dictionary<string, SortedList<DateTime, Proto.Credit>>(),
                new Dictionary<string, SortedList<DateTime, Proto.Credit>>(),
                new Dictionary<string, SortedList<DateTime, Proto.Credit>>(),
            };
        private object mutex = new object();
    }
}
