using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class TradeManager
    {
        public void OnProtoMessage(Proto.TradeRep rep)
        {
            lock(this.trades)
            {
                foreach (var trade in rep.Trades)
                {
                    this.trades[trade.Id] = trade;
                }
            }
        }

        public void OnProtoMessage(Proto.Trade trade)
        {
            lock (this.trades)
            {
                this.trades[trade.Id] = trade;
            }
        }

        public List<Proto.Trade> GetTrades()
        {
            lock(this.trades)
            {
                return this.trades.Values.ToList();
            }
        }

        public IEnumerable<Proto.Trade> GetTrades(string instrument)
        {
            //List<Proto.Trade> ret = new List<Proto.Trade>();
            lock (this.trades)
            {
                return (from t in this.trades.Values where t.Instrument == instrument select t);
            }
        }

        public int GetTurnover(string instrument)
        {
            int turnover = 0;
            lock (this.trades)
            {
                foreach (var kvp in trades)
                {
                    if (kvp.Value.Instrument == instrument)
                    {
                        turnover += kvp.Value.Volume;
                    }
                }
            }
            return turnover;
        }

        Dictionary<string, Proto.Trade> trades = new Dictionary<string, Proto.Trade>();


        static public double GetFee(Instrument inst, Proto.Trade trade)
        {
            if (inst.CommissionType == Proto.CommissionType.Volume)
            {
                if (OrderManager.IsOpen(trade.Side))
                {
                    return trade.Volume * inst.OpenCommission;
                }
                else if (OrderManager.IsCloseToday(trade.Side))
                {
                    return trade.Volume * inst.CloseTodayCommission;
                }
                else
                {
                    return trade.Volume * inst.CloseCommission;
                }
            }
            else
            {
                if (OrderManager.IsOpen(trade.Side))
                {
                    return trade.Price * trade.Volume * inst.OpenCommission;
                }
                else if (OrderManager.IsCloseToday(trade.Side))
                {
                    return trade.Price * trade.Volume * inst.CloseTodayCommission;
                }
                else
                {
                    return trade.Price * trade.Volume * inst.CloseCommission;
                }
            }
        }
    }
}
