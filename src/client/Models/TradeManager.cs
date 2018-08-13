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

        Dictionary<string, Proto.Trade> trades = new Dictionary<string, Proto.Trade>();
    }
}
