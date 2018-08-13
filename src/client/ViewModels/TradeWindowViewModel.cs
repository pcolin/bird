using client.Models;
using Microsoft.Practices.Unity;
using Prism.Events;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Threading;

namespace client.ViewModels
{
    class TradeWindowViewModel : BindableBase
    {
        public TradeWindowViewModel(IUnityContainer container, Dispatcher dispatcher)
        {
            this.container = container;
            this.dispatcher = dispatcher;

            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
        }

        private ObservableCollection<TradeItem> items;
        public ObservableCollection<TradeItem> Items
        {
            get { return items; }
            set { SetProperty(ref items, value); }
        }        

        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            this.productManagers = new Dictionary<Proto.Exchange, ProductManager>();
            this.orderManagers = new Dictionary<Proto.Exchange, OrderManager>();
            this.theoCalculators = new Dictionary<Proto.Exchange, TheoCalculator>();
            List<Proto.Trade> trades = new List<Proto.Trade>();
            foreach (var exchange in exchanges)
            {
                var tm = this.container.Resolve<TradeManager>(exchange.ToString());
                trades.AddRange(tm.GetTrades());
                this.productManagers[exchange] = this.container.Resolve<ProductManager>(exchange.ToString());
                this.orderManagers[exchange] = this.container.Resolve<OrderManager>(exchange.ToString());
                this.theoCalculators[exchange] = this.container.Resolve<TheoCalculator>(exchange.ToString());
            }

            var items = new ObservableCollection<TradeItem>();
            foreach (var trade in trades.OrderBy(t=>t.Time))
            {
                var inst = productManagers[trade.Exchange].FindId(trade.Instrument);
                if (inst != null)
                {
                    items.Add(new TradeItem(inst, orderManagers[trade.Exchange].FindOrder(trade.OrderId), trade));
                }
            }

            this.Items = items;
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Trade>>().Subscribe(this.ReceiveTrade, ThreadOption.BackgroundThread);
        }

        private void ReceiveTrade(Proto.Trade trade)
        {
            ProductManager pm = null;
            if (this.productManagers.TryGetValue(trade.Exchange, out pm))
            {
                var inst = pm.FindId(trade.Instrument);
                if (inst != null)
                {
                    Proto.Order order = null;
                    OrderManager om = null;
                    if (this.orderManagers.TryGetValue(trade.Exchange, out om))
                    {
                        order = om.FindOrder(trade.OrderId);
                    }
                    var item = new TradeItem(inst, order, trade);
                    TheoCalculator calculator = null;
                    if (this.theoCalculators.TryGetValue(trade.Exchange, out calculator))
                    {
                        double theo = 0;
                        if (inst.Type == Proto.InstrumentType.Option)
                        {
                            var greeks = calculator.GetGreeks(inst as Option);
                            if (greeks != null)
                            {
                                theo = greeks.Greeks.theo;
                            }
                        }
                        else
                        {
                            var price = calculator.GetPrice(inst);
                            if (price != null)
                            {
                                theo = price.Last.Price;
                            }
                        }

                        if (theo > 0)
                        {
                            if (OrderManager.IsBid(trade.Side))
                            {
                                item.PnL = (theo - trade.Price) * trade.Volume * inst.Multiplier * inst.Lot;
                            }
                            else
                            {
                                item.PnL = (trade.Price - theo) * trade.Volume * inst.Multiplier * inst.Lot;
                            }
                        }
                    }

                    this.dispatcher.Invoke((MethodInvoker)delegate { this.Items.Add(item); });
                }
            }
        }

        private IUnityContainer container;
        private Dispatcher dispatcher;

        private Dictionary<Proto.Exchange, ProductManager> productManagers;
        private Dictionary<Proto.Exchange, OrderManager> orderManagers;
        private Dictionary<Proto.Exchange, TheoCalculator> theoCalculators;
    }

    class TradeItem : BindableBase
    {
        public TradeItem(Instrument inst, Proto.Order order, Proto.Trade trade)
        {
            Instrument = inst;
            Trade = trade;
            Order = order;

            if (inst.Type == Proto.InstrumentType.Option)
            {
                this.Type = (inst as Option).OptionType.ToString();
            }
            else
            {
                this.Type = inst.Type.ToString();
            }
            this.Time = new DateTime(1970, 1, 1, 0, 0, 0).ToLocalTime().AddMilliseconds(trade.Time / 1000);
            if (order != null)
            {
                this.OrderTime = new DateTime(1970, 1, 1, 0, 0, 0).ToLocalTime().AddMilliseconds(order.Time / 1000);
            }

            if (inst.CommissionType == Proto.CommissionType.Volume)
            {
                if (OrderManager.IsOpen(trade.Side))
                {
                    Fee = trade.Volume * inst.OpenCommission;
                }
                else if (OrderManager.IsCloseToday(trade.Side))
                {
                    Fee = trade.Volume * inst.CloseTodayCommission;
                }
                else
                {
                    Fee = trade.Volume * inst.CloseCommission;
                }
            }
            else
            {
                if (OrderManager.IsOpen(trade.Side))
                {
                    Fee = trade.Price * trade.Volume * inst.OpenCommission;
                }
                else if (OrderManager.IsCloseToday(trade.Side))
                {
                    Fee = trade.Price * trade.Volume * inst.CloseTodayCommission;
                }
                else
                {
                    Fee = trade.Price * trade.Volume * inst.CloseCommission;
                }
            }
        }

        public Instrument Instrument { get; private set; }
        public Proto.Trade Trade { get; private set; }
        public Proto.Order Order { get; private set; }

        public string Type { get; private set; }
        public DateTime Time { get; private set; }
        public DateTime OrderTime { get; private set; }

        public double Fee { get; private set; }
        public double PnL { get; set; }
    }
}
