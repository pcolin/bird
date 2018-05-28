using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class VolatilityCurveManager
    {
        //public void Set(IList<Proto.VolatilityCurve> curves)
        //{
        //    lock (this.mutex)
        //    {
        //        this.curves.Clear();
        //        foreach (var c in curves)
        //        {
        //            SortedList<DateTime, Proto.VolatilityCurve> tmp = null;
        //            if (this.curves.TryGetValue(c.Underlying, out tmp) == false)
        //            {
        //                tmp = new SortedList<DateTime, Proto.VolatilityCurve>();
        //                this.curves.Add(c.Underlying, tmp);
        //            }
        //            try
        //            {
        //                tmp.Add(DateTime.ParseExact(c.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture), c);
        //            }
        //            catch (Exception) { }
        //        }
        //    }
        //}

        public void OnProtoMessage(Proto.VolatilityCurveRep rep)
        {
            lock (this.mutex)
            {
                //this.curves.Clear();
                foreach (var c in rep.Curves)
                {
                    SortedList<DateTime, Proto.VolatilityCurve> tmp = null;
                    if (this.curves.TryGetValue(c.Underlying, out tmp) == false)
                    {
                        tmp = new SortedList<DateTime, Proto.VolatilityCurve>();
                        this.curves.Add(c.Underlying, tmp);
                    }
                    try
                    {
                        tmp.Add(DateTime.ParseExact(c.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture), c);
                    }
                    catch (Exception) { }
                }
            }
        }

        public void OnProtoMessage(Proto.VolatilityCurveReq req)
        {
            lock (this.mutex)
            {
                if (req.Type == Proto.RequestType.Set)
                {
                    foreach (var c in req.Curves)
                    {
                        SortedList<DateTime, Proto.VolatilityCurve> tmp = null;
                        if (this.curves.TryGetValue(c.Underlying, out tmp) == false)
                        {
                            tmp = new SortedList<DateTime, Proto.VolatilityCurve>();
                            this.curves.Add(c.Underlying, tmp);
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

        public Proto.VolatilityCurve GetVolatilityCurve(string underlying, DateTime maturity)
        {
            SortedList<DateTime, Proto.VolatilityCurve> tmp = null;
            Proto.VolatilityCurve curve = null;
            lock (this.mutex)
            {
                if (this.curves.TryGetValue(underlying, out tmp) && tmp.TryGetValue(maturity, out curve)) { }
            }
            return curve;
        }

        Dictionary<string, SortedList<DateTime, Proto.VolatilityCurve>> curves = new Dictionary<string, SortedList<DateTime, Proto.VolatilityCurve>>();
        private object mutex = new object();
    }
}
