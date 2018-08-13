using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class OrderManager
    {
        public void OnProtoMessage(Proto.OrderRep rep)
        {
            lock (this.orders)
            {
                foreach (var order in rep.Orders)
                {
                    this.orders.Add(order.Id, order);
                }
            }
        }

        public void OnProtoMessage(Proto.OrderReq req)
        {
            lock (this.orders)
            {
                foreach (var order in req.Orders)
                {
                    if (order.Status == Proto.OrderStatus.Canceled)
                    {
                        this.orders.Remove(order.Id);
                    }
                    else
                    {
                        this.orders[order.Id] = order;
                    }
                }
            }
        }

        public List<Proto.Order> GetOrders()
        {
            lock (this.orders)
            {
                return this.orders.Values.ToList();
            }
        }
                
        public Proto.Order FindOrder(ulong id)
        {
            Proto.Order order = null;
            lock (this.orders)
            {
                this.orders.TryGetValue(id, out order);
            }
            return order;
        }

        static public bool IsBid(Proto.Side side)
        {
            return side == Proto.Side.Buy || side == Proto.Side.BuyCover || side == Proto.Side.BuyCoverToday || side == Proto.Side.BuyCoverYesterday;
        }

        static public bool IsOpen(Proto.Side side)
        {
            return side == Proto.Side.Buy || side == Proto.Side.Sell;
        }

        static public bool IsCloseToday(Proto.Side side)
        {
            return side == Proto.Side.BuyCoverToday || side == Proto.Side.SellCoverToday;
        }

        static public bool IsInactive(Proto.Order order)
        {
            return order.Status >= Proto.OrderStatus.Filled;
        }

        static public bool IsQuoter(Proto.Order order)
        {
            return order.StrategyType == Proto.StrategyType.Quoter || order.StrategyType == Proto.StrategyType.DummyQuoter;
        }

        private Dictionary<ulong, Proto.Order> orders = new Dictionary<ulong, Proto.Order>();
    }
}
