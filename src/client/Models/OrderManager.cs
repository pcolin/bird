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

        }

        public void OnProtoMessage(Proto.OrderReq req)
        {

        }

        static public bool IsBid(Proto.Order order)
        {
            return order.Side == Proto.Side.Buy || order.Side == Proto.Side.BuyCover || order.Side == Proto.Side.BuyCoverToday || order.Side == Proto.Side.BuyCoverYesterday;
        }

        static public bool IsInactive(Proto.Order order)
        {
            return order.Status >= Proto.OrderStatus.Filled;
        }
    }
}
