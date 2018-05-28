using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class InterestRateManager
    {
        //public void Set(IList<Proto.InterestRate> rates)
        //{
        //    lock (this.mutex)
        //    {
        //        this.rates.Clear();
        //        foreach (var r in rates)
        //        {
        //            this.rates.Add(r.Days, r.Rate);
        //        }
        //    }
        //}

        public void OnProtoMessage(Proto.InterestRateRep rep)
        {
            if (rep.Rates.Count > 0)
            {
                lock (this.mutex)
                {
                    this.rates.Clear();
                    foreach (var r in rep.Rates)
                    {
                        this.rates.Add(r.Days, r.Rate);
                    }
                }
            }
        }

        public void OnProtoMessage(Proto.InterestRateReq req)
        {
            lock (this.mutex)
            {
                if (req.Type == Proto.RequestType.Set)
                {
                    this.rates.Clear();
                    foreach (var r in req.Rates)
                    {
                        this.rates.Add(r.Days, r.Rate);
                    }
                }
            }
        }

        public void Foreach(Action<int, double> action)
        {
            lock (this.mutex)
            {
                foreach (var kvp in this.rates)
                {
                    action(kvp.Key, kvp.Value);
                }
            }
        }

        public double? GetInterestRate(System.DateTime maturity)
        {
            lock (this.mutex)
            {
                if (rates.Count > 0)
                {
                    int days = (int)(maturity - DateTime.Now).TotalDays + 1;
                    using (var it = rates.GetEnumerator())
                    {
                        KeyValuePair<int, double> prev = new KeyValuePair<int,double>(0, 0);
                        while (it.MoveNext())
                        {
                            if (it.Current.Key < days)
                            {
                                prev = it.Current;
                            }
                            else if (prev.Key > 0)
                            {
                                return prev.Value + (it.Current.Value - prev.Value) * (days - prev.Key) / (it.Current.Key - prev.Key);
                            }
                            else
                            {
                                return it.Current.Value;
                            }
                        }
                        return prev.Value;
                    }
                }
            }
            return null;
        }

        private SortedList<int, double> rates = new SortedList<int, double>();
        private object mutex = new object();
    }
}
