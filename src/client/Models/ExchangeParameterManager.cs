using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class ExchangeParameterManager
    {
        public static int AnnalTradingDays = 245;

        public void OnProtoMessage(Proto.ExchangeParameterRep rep)
        {
            if (rep.Parameters.Count > 0)
            {
                OnProtoMessage(rep.Parameters[0]);
            }
        }

        public void OnProtoMessage(Proto.ExchangeParameterReq req)
        {
            if (req.Type == Proto.RequestType.Set)
            {
                if (req.Parameters.Count > 0)
                {
                    OnProtoMessage(req.Parameters[0]);
                }
            }
        }

        private void OnProtoMessage(Proto.ExchangeParameter param)
        {
            Func<IList<Proto.TradingSession>, List<Tuple<DateTime, DateTime>>, int> parseSessionFunc = (protoSessions, sessions) =>
            {
                sessions.Clear();
                int seconds = 0;
                foreach (var s in protoSessions)
                {
                    var d = tradingDay;
                    var begin_time = DateTime.ParseExact(s.Begin, "HH:mm:ss", CultureInfo.InvariantCulture);
                    if (s.Begin.CompareTo(param.NightSessionTime) >= 0)
                    {
                        do
                        {
                            d = d.AddDays(-1);
                        } while (d.DayOfWeek == DayOfWeek.Saturday || d.DayOfWeek == DayOfWeek.Sunday || holidays.ContainsKey(d));
                    }
                    var begin = new DateTime(d.Year, d.Month, d.Day, begin_time.Hour, begin_time.Minute, begin_time.Second);

                    var end_time = DateTime.ParseExact(s.Begin, "HH:mm:ss", CultureInfo.InvariantCulture);
                    if (begin_time > end_time)
                    {
                        d = d.AddDays(1);
                    }
                    var end = new DateTime(d.Year, d.Month, d.Day, end_time.Hour, end_time.Minute, end_time.Second);
                    sessions.Add(Tuple.Create(begin, end));
                    seconds += (int)(end - begin).TotalSeconds;
                }
                return seconds;
            };
            lock (this.mutex)
            {
                try
                {
                    this.parameter = param;
                    this.tradingDay = DateTime.ParseExact(param.TradingDay, "yyyyMMdd", CultureInfo.InvariantCulture);

                    this.holidays.Clear();
                    foreach (var h in param.Holidays)
                    {
                        holidays[DateTime.ParseExact(h.Date, "yyyyMMdd", CultureInfo.InvariantCulture)] = h.Weight;
                    }

                    //DateTime nextTradingDay = this.tradingDay.AddDays(1);
                    //while (nextTradingDay.DayOfWeek == DayOfWeek.Saturday || nextTradingDay.DayOfWeek == DayOfWeek.Sunday || holidays.ContainsKey(nextTradingDay))
                    //{
                    //    nextTradingDay = nextTradingDay.AddDays(1);
                    //}
                    var now = DateTime.Now;
                    var charmTime = DateTime.ParseExact(param.CharmStartTime, "HH:mm:ss", CultureInfo.InvariantCulture);
                    this.charmDateTime = new DateTime(now.Year, now.Month, now.Day, charmTime.Hour, charmTime.Minute, charmTime.Second);

                    this.sessionSeconds = parseSessionFunc(param.Sessions, sessions);
                    this.maturitySessionSeconds = parseSessionFunc(param.MaturitySessions, maturitySessions);

                    this.daysToMaturity.Clear();
                }
                catch (Exception)
                {
                    parameter = null;
                    holidays.Clear();
                    sessions.Clear();
                    maturitySessions.Clear();
                    daysToMaturity.Clear();
                }
            }
        }

        //public void Add(Proto.ExchangeParameter parameter)
        //{
        //    lock (this.mutex)
        //    {
        //        this.parameter = parameter;
        //    }
        //}

        public Proto.ExchangeParameter GetExchangeParameter()
        {
            lock (this.mutex)
            {
                return parameter;
            }
        }

        public double GetTimeValue(DateTime maturity)
        {            
            lock (this.mutex)
            {
                return GetTimeValue(maturity, this.tradingDay, DateTime.Now, this.daysToMaturity);
            }
        }

        public double GetCharmTimeValue(DateTime maturity)
        {
            lock (this.mutex)
            {
                return GetTimeValue(maturity, this.tradingDay.AddDays(1), this.charmDateTime, charmDaysToMaturity);
            }
        }

        public double GetTimeValue(DateTime maturity, DateTime date, DateTime time, Dictionary<DateTime, double> days)
        {
            if (parameter == null) return double.NaN;

            double tradingDays = 0;
            if (days.TryGetValue(maturity, out tradingDays) == false)
            {
                for (var d = date.AddDays(1); d <= maturity; d = d.AddDays(1))
                {
                    if (d.DayOfWeek != DayOfWeek.Saturday && d.DayOfWeek != DayOfWeek.Sunday)
                    {
                        double weight = 0;
                        if (holidays.TryGetValue(d, out weight))
                        {
                            tradingDays += weight;
                        }
                        else
                        {
                            tradingDays += 1;
                        }
                    }
                }
                days.Add(maturity, tradingDays);
            }

            if (date < maturity)
            {
                return (tradingDays + GetFraction(time, this.sessions, sessionSeconds)) / AnnalTradingDays;
            }
            else if (date == maturity)
            {
                return (tradingDays + GetFraction(time, maturitySessions, maturitySessionSeconds)) / AnnalTradingDays;
            }
            else
            {
                return 0;
            }
        }

        double GetFraction(DateTime now, List<Tuple<DateTime, DateTime>> sessions, double seconds)
        {
            double ret = 0;
            for (int i = sessions.Count - 1; i >= 0 && now < sessions[i].Item2; --i)
            {
                if (now > sessions[i].Item1)
                {
                    ret += (sessions[i].Item2 - now).TotalSeconds / seconds;
                }
            }
            return ret;
        }

        double GetTimeValue(DateTime date, DateTime time, DateTime maturity)
        {
            lock (this.mutex)
            {
                if (parameter == null) return double.NaN;

                double tradingDays = 0;
                if (daysToMaturity.TryGetValue(maturity, out tradingDays) == false)
                {
                    for (var d = date.AddDays(1); d <= maturity; d = d.AddDays(1))
                    {
                        if (d.DayOfWeek != DayOfWeek.Saturday && d.DayOfWeek != DayOfWeek.Sunday)
                        {
                            double weight = 0;
                            if (holidays.TryGetValue(d, out weight))
                            {
                                tradingDays += weight;
                            }
                            else
                            {
                                tradingDays += 1;
                            }
                        }
                    }
                    daysToMaturity.Add(maturity, tradingDays);
                }

                Func<List<Tuple<DateTime, DateTime>>, int, double> fractionFunc = (sessions, seconds) =>
                {
                    double ret = 0;
                    for (int i = sessions.Count - 1; i >= 0 && time < sessions[i].Item2; --i)
                    {
                        if (time > sessions[i].Item1)
                        {
                            ret += (sessions[i].Item2 - time).TotalSeconds / seconds;
                        }
                    }
                    return ret;
                };

                if (date < maturity)
                {
                    return (tradingDays + fractionFunc(this.sessions, sessionSeconds)) / AnnalTradingDays;
                }
                else 
                {
                    return (tradingDays + fractionFunc(maturitySessions, maturitySessionSeconds)) / AnnalTradingDays;
                }
            }
        }

        Proto.ExchangeParameter parameter = null;
        private object mutex = new object();

        DateTime tradingDay;
        DateTime charmDateTime;
        SortedList<DateTime, double> holidays = new SortedList<DateTime, double>();
        List<Tuple<DateTime, DateTime>> sessions = new List<Tuple<DateTime, DateTime>>();
        List<Tuple<DateTime, DateTime>> maturitySessions = new List<Tuple<DateTime, DateTime>>();
        int sessionSeconds;
        int maturitySessionSeconds;
        Dictionary<DateTime, double> daysToMaturity = new Dictionary<DateTime, double>();
        Dictionary<DateTime, double> charmDaysToMaturity = new Dictionary<DateTime, double>();
    }
}
