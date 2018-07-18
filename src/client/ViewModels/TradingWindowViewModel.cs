using client.Models;
using Microsoft.Practices.Unity;
using Prism.Commands;
using Prism.Events;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Threading;

namespace client.ViewModels
{
    class TradingWindowViewModel : BindableBase
    {
        public ICommand PriceCopyCommand { get; set; }
        public ICommand CancelBuyOrdersCommand { get; set; }
        public ICommand CancelSellOrdersCommand { get; set; }
        public DelegateCommand BuyOrderCommand { get; set; }
        public DelegateCommand SellOrderCommand { get; set; }

        private Proto.Exchange exchange;
        public Proto.Exchange Exchange
        {
            get { return exchange; }
        }

        private Proto.InstrumentType type;
        public Proto.InstrumentType Type
        {
            get { return type; }
        }

        private bool isOption;
        public bool IsOption
        {
            get { return isOption; }
            set
            {
                if (SetProperty(ref isOption, value))
                {
                    if (value)
                    {
                        this.Items.Filter = x =>
                            {
                                TradingItem item = x as TradingItem;
                                return item.Instrument.Type == Proto.InstrumentType.Option;
                            };
                    }
                    else
                    {
                        this.Items.Filter = x =>
                        {
                            TradingItem item = x as TradingItem;
                            return item.Instrument.Type != Proto.InstrumentType.Option;
                        };
                    }
                }
            }
        }

        private double price;
        public double Price
        {
            get { return price; }
            set
            {
                if (SetProperty(ref price, value))
                {
                    BuyOrderCommand.RaiseCanExecuteChanged();
                    SellOrderCommand.RaiseCanExecuteChanged();
                }
            }
        }

        private int volume;
        public int Volume
        {
            get { return volume; }
            set
            {
                if (SetProperty(ref volume, value))
                {
                    BuyOrderCommand.RaiseCanExecuteChanged();
                    SellOrderCommand.RaiseCanExecuteChanged();
                }
            }
        }

        private bool isCover;
        public bool IsCover
        {
            get { return isCover; }
            set
            {
                if (SetProperty(ref isCover, value))
                {
                    BuyOrderCommand.RaiseCanExecuteChanged();
                    SellOrderCommand.RaiseCanExecuteChanged();
                }
            }
        }        

        private TradingItem selectedItem;
        public TradingItem SelectedItem
        {
            get { return selectedItem; }
            set
            {
                if (SetProperty(ref selectedItem, value))
                {
                    Price = value.Last;
                    BuyOrderCommand.RaiseCanExecuteChanged();
                    SellOrderCommand.RaiseCanExecuteChanged();
                }
            }
        }

        private ICollectionView items;
        public ICollectionView Items
        {
            get { return items; }
            set { SetProperty(ref items, value); }
        }        

        //private ObservableCollection<TradingItem> items;
        //public ObservableCollection<TradingItem> Items
        //{
        //    get { return items; }
        //    set { SetProperty(ref items, value); }
        //}        
        
        public TradingWindowViewModel(Proto.Exchange exchange, IUnityContainer container, Dispatcher dispatcher)
        {
            this.exchange = exchange;
            this.container = container;
            this.dispatcher = dispatcher;

            this.PriceCopyCommand = new DelegateCommand<object>(this.PriceCopyExecute);
            this.CancelBuyOrdersCommand = new DelegateCommand<object>(this.CancelBuyOrdersExecute);
            this.CancelSellOrdersCommand = new DelegateCommand<object>(this.CancelSellOrdersExecute);
            this.BuyOrderCommand = new DelegateCommand(this.BuyOrderExecute, this.CanBuyOrder);
            this.SellOrderCommand = new DelegateCommand(this.SellOrderExecute, this.CanSellOrder);

            var pm = container.Resolve<ProductManager>(exchange.ToString());
            var positons = container.Resolve<PositionManager>(exchange.ToString());
            if (pm != null && positons != null)
            {
                //this.items = new ObservableCollection<TradingItem>();
                var items = new List<TradingItem>();
                this.tradingItems = new Dictionary<string, TradingItem>();
                var instruments = pm.GetInstrusments().OrderBy(x => x.Id);
                foreach (var inst in instruments)
                {
                    var item = new TradingItem() { Instrument = inst, };
                    Proto.Position position = null;
                    if (positons.GetPosition(inst.Id, out position))
                    {
                        item.LongPosition = position.LiquidLong;
                        item.ShortPosition = position.LiquidShort;
                    }
                    items.Add(item);
                    this.tradingItems.Add(inst.Id, item);
                }
                if (items.Count > 0)
                {
                    this.SelectedItem = items[0];
                    this.type = this.SelectedItem.Instrument.Underlying.Type;
                    this.Items = new ListCollectionView(items);
                    this.Items.Filter = x =>
                    {
                        TradingItem item = x as TradingItem;
                        return item.Instrument.Type == Proto.InstrumentType.Future;
                    };
                }

                container.Resolve<EventAggregator>().GetEvent<GreeksEvent>().Subscribe(this.ReceiveTheo, ThreadOption.BackgroundThread);
                container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Price>>().Subscribe(this.ReceivePrice, ThreadOption.BackgroundThread);
                container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.OrderReq>>().Subscribe(this.ReceiveOrders, ThreadOption.BackgroundThread);
                container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.PositionReq>>().Subscribe(this.ReceivePositions, ThreadOption.BackgroundThread);
            }
        }

        private void ReceivePrice(Proto.Price p)
        {
            if (p.Exchange == this.exchange)
            {
                TradingItem item = null;
                if (this.tradingItems.TryGetValue(p.Instrument, out item))
                {
                    this.dispatcher.Invoke((MethodInvoker)delegate { item.ReceivePrice(p); });
                }
            }
        }

        private void ReceiveTheo(GreeksData greeks)
        {
            if (greeks.Option.Exchange == this.exchange)
            {
                TradingItem item = null;
                if (this.tradingItems.TryGetValue(greeks.Option.Id, out item))
                {
                    this.dispatcher.Invoke((MethodInvoker)delegate { item.ReceiveTheo(greeks); });
                }
            }
        }

        private void ReceiveOrders(Proto.OrderReq req)
        {
            if (req.Exchange == this.exchange)
            {
                foreach (var order in req.Orders)
                {
                    TradingItem item = null;
                    if (this.tradingItems.TryGetValue(order.Instrument, out item))
                    {
                        this.dispatcher.Invoke((MethodInvoker)delegate { item.ReceiveOrder(order); });
                    }
                }
            }
        }

        private void ReceivePositions(Proto.PositionReq req)
        {
            if (req.Exchange == this.exchange && req.Type == Proto.RequestType.Set)
            {
                foreach (var p in req.Positions)
                {
                    TradingItem item = null;
                    if (this.tradingItems.TryGetValue(p.Instrument, out item))
                    {
                        this.dispatcher.Invoke((MethodInvoker)delegate { item.ReceivePosition(p); });
                    }
                }
            }
        }

        private void PriceCopyExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            PriceItem item = cell.Item as PriceItem;
            this.Price = item.Price;
        }

        private void CancelBuyOrdersExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            PriceItem item = cell.Item as PriceItem;
            CancelOrders(item.Bids);
        }

        private void CancelSellOrdersExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            PriceItem item = cell.Item as PriceItem;
            CancelOrders(item.Asks);
        }

        private void BuyOrderExecute()
        {
            SubmitOrder(this.IsCover ? Proto.Side.BuyCover : Proto.Side.Buy);
        }

        private bool CanBuyOrder()
        {
            return this.Price >= this.SelectedItem.Instrument.Lowest && this.Price <= this.SelectedItem.Instrument.Highest &&
                this.Volume > 0 && (!this.IsCover || this.Volume <= this.SelectedItem.ShortPosition);
        }

        private void SellOrderExecute()
        {
            SubmitOrder(this.IsCover ? Proto.Side.SellCover : Proto.Side.Sell);
        }

        private bool CanSellOrder()
        {
            return this.Price >= this.SelectedItem.Instrument.Lowest && this.Price <= this.SelectedItem.Instrument.Highest &&
                this.Volume > 0 && (!this.IsCover || this.Volume <= this.SelectedItem.LongPosition);
        }

        private void SubmitOrder(Proto.Side side)
        {
            Proto.OrderRequest req = new Proto.OrderRequest() {  Action = Proto.OrderAction.Submit, };
            req.Orders.Add(new Proto.Order()
            {
                //Id = (ulong)Guid.NewGuid().GetHashCode(),
                Instrument = this.SelectedItem.Instrument.Id,
                Price = this.Price,
                Volume = this.Volume,
                Exchange = this.Exchange,
                StrategyType = Proto.StrategyType.Manual,
                Side = side,
                TimeCondition = Proto.TimeCondition.Gtd,
                Type = Proto.OrderType.Limit,
                Time = (ulong)(DateTime.Now - new DateTime(1970, 1, 1, 8, 0, 0)).TotalMilliseconds * 1000,
                Status = Proto.OrderStatus.Local,
            });

            var server = this.container.Resolve<ServerService>(this.exchange.ToString());
            server.Request(req);
        }

        private void CancelOrders(List<Proto.Order> orders)
        {
            if (orders.Count > 0)
            {
                Proto.OrderRequest req = new Proto.OrderRequest() { Action = Proto.OrderAction.Cancel, };
                req.Orders.AddRange(orders);

                var server = this.container.Resolve<ServerService>(this.exchange.ToString());
                server.Request(req);
            }
        }

        private IUnityContainer container;
        private Dispatcher dispatcher;
        private Dictionary<string, TradingItem> tradingItems;
    }

    class TradingItem : BindableBase
    {
        public TradingItem()
        {
            Prices = new RangeObservableCollection<PriceItem>();
        }

        public void ReceivePrice(Proto.Price p)
        {
            Last = p.Last.Price;
            Open = p.Open;
            High = p.High;
            Low = p.Low;
            Close = p.PreClose;
            if (p.AdjustedPrice > 0)
            {
                Theo = p.AdjustedPrice;
            }

            int i = p.Asks.Count - 1;
            int j = 0;
            while (i >= 0 && j < Prices.Count)
            {
                if (p.Asks[i].Price == Prices[j].Price)
                {
                    Prices[j].MarketBidVolume = 0;
                    Prices[j].MarketAskVolume = p.Asks[i].Volume;
                    --i;
                    ++j;
                }
                else if (p.Asks[i].Price < Prices[j].Price)
                {
                    int beg = j;
                    do
                    {
                        if (Prices[j].AskVolume > 0)
                        {
                            Prices[j].MarketAskVolume = 0;
                            Prices.RemoveRange(beg, j - beg);
                            ++beg;
                            j = beg;
                        }
                        else
                        {
                            ++j;
                        }
                    } while (j < Prices.Count && p.Asks[i].Price < Prices[j].Price);
                    Prices.RemoveRange(beg, j - beg);
                    j = beg;
                }
                else
                {
                    var items = new List<PriceItem>();
                    do
                    {
                        items.Add(new PriceItem()
                            {
                                Price = p.Asks[i].Price,
                                MarketAskVolume = p.Asks[i].Volume,
                            });
                        --i;
                    } while (i >= 0 && p.Asks[i].Price > Prices[j].Price);
                    Prices.InsertRange(j, items);
                    j += items.Count;
                }
            }

            if (i >= 0)
            {
                var items = new List<PriceItem>();
                do
                {
                    items.Add(new PriceItem()
                        {
                            Price = p.Asks[i].Price,
                            MarketAskVolume = p.Asks[i].Volume,
                        });
                    --i;
                }
                while (i >= 0);

                foreach (var bid in p.Bids)
                {
                    items.Add(new PriceItem()
                        {
                            Price = bid.Price,
                            MarketBidVolume = bid.Volume,
                        });
                }
                Prices.AddRange(items);
            }
            else
            {
                i = 0;
                while (i < p.Bids.Count && j < Prices.Count)
                {
                    if (p.Bids[i].Price == Prices[j].Price)
                    {
                        Prices[j].MarketAskVolume = 0;
                        Prices[j].MarketBidVolume = p.Bids[i].Volume;
                        ++i;
                        ++j;
                    }
                    else if (p.Bids[i].Price < Prices[j].Price)
                    {
                        int beg = j;
                        do
                        {
                            if (Prices[j].BidVolume > 0)
                            {
                                Prices[j].MarketBidVolume = 0;
                                Prices.RemoveRange(beg, j - beg);
                                ++beg;
                                j = beg;
                            }
                            else
                            {
                                ++j;
                            }
                        } while (j < Prices.Count && p.Bids[i].Price < Prices[j].Price);
                        Prices.RemoveRange(beg, j - beg);
                        j = beg;
                    }
                    else
                    {
                        var items = new List<PriceItem>();
                        do
                        {
                            items.Add(new PriceItem()
                            {
                                Price = p.Bids[i].Price,
                                MarketBidVolume = p.Bids[i].Volume,
                            });
                            ++i;
                        } while (i < p.Bids.Count && p.Bids[i].Price > Prices[j].Price);
                        Prices.InsertRange(j, items);
                        j += items.Count;
                    }
                }
                if (i < p.Bids.Count)
                {
                    var items = new List<PriceItem>();
                    do
                    {
                        items.Add(new PriceItem()
                        {
                            Price = p.Bids[i].Price,
                            MarketBidVolume = p.Bids[i].Volume,
                        });
                        ++i;
                    }
                    while (i < p.Bids.Count);
                    Prices.AddRange(items);
                }
                else if (j < Prices.Count)
                {
                    int beg = j;
                    do
                    {
                        if (Prices[j].BidVolume > 0)
                        {
                            Prices[j].MarketBidVolume = 0;
                            Prices.RemoveRange(beg, j - beg);
                            ++beg;
                            j = beg;
                        }
                        else
                        {
                            ++j;
                        }
                    } while (j < Prices.Count);
                    Prices.RemoveRange(beg, j - beg);
                }
            }
        }

        public void ReceiveTheo(GreeksData greeks)
        {
            Theo = greeks.Greeks.theo;
        }

        public void ReceiveOrder(Proto.Order order)
        {
            int idx = 0;
            while (idx < this.Prices.Count)
            {
                if (order.Price == this.Prices[idx].Price)
                {
                    if (OrderManager.IsBid(order))
                    {
                        var bids = this.Prices[idx].Bids;
                        if (OrderManager.IsInactive(order))
                        {
                            for (int i = 0; i < bids.Count; ++i)
                            {
                                if (order.Id == bids[i].Id)
                                {
                                    this.Prices[idx].BidVolume -= (order.Volume - bids[i].ExecutedVolume);
                                    if (this.Prices[idx].BidVolume == 0 && this.Prices[idx].AskVolume == 0 && this.Prices[idx].MarketBidVolume == 0 && this.Prices[idx].MarketAskVolume == 0)
                                    {
                                        this.Prices.RemoveAt(idx);
                                    }
                                    else
                                    {
                                        bids.RemoveAt(i);
                                    }
                                    return;
                                }
                            }
                        }
                        else
                        {
                            for (int i = 0; i < bids.Count; ++i)
                            {
                                if (order.Id == bids[i].Id)
                                {
                                    this.Prices[idx].BidVolume -= (order.ExecutedVolume - bids[i].ExecutedVolume);
                                    bids[i] = order;
                                    return;
                                }
                            }
                            this.Prices[idx].BidVolume += order.Volume - order.ExecutedVolume;
                            bids.Add(order);
                            return;
                        }
                    }
                    else
                    {
                        var asks = this.Prices[idx].Asks;
                        if (OrderManager.IsInactive(order))
                        {
                            for (int i = 0; i < asks.Count; ++i)
                            {
                                if (order.Id == asks[i].Id)
                                {
                                    this.Prices[idx].BidVolume -= (order.Volume - asks[i].ExecutedVolume);
                                    if (this.Prices[idx].BidVolume == 0 && this.Prices[idx].AskVolume == 0 && this.Prices[idx].MarketBidVolume == 0 && this.Prices[idx].MarketAskVolume == 0)
                                    {
                                        this.Prices.RemoveAt(idx);
                                    }
                                    else
                                    {
                                        asks.RemoveAt(i);
                                    }
                                    return;
                                }
                            }
                        }
                        else
                        {
                            for (int i = 0; i < asks.Count; ++i)
                            {
                                if (order.Id == asks[i].Id)
                                {
                                    this.Prices[idx].BidVolume -= (order.ExecutedVolume - asks[i].ExecutedVolume);
                                    asks[i] = order;
                                    return;
                                }
                            }
                            this.Prices[idx].AskVolume += (order.Volume - order.ExecutedVolume);
                            asks.Add(order);
                            return;
                        }
                    }
                }
                else if (order.Price > this.Prices[idx].Price)
                {
                    break;
                }
                ++idx;
            }

            if (OrderManager.IsInactive(order) == false)
            {
                var item = new PriceItem()
                    {
                        Price = order.Price,
                    };
                if (OrderManager.IsBid(order))
                {
                    item.BidVolume = order.Volume - order.ExecutedVolume;
                    item.Bids.Add(order);
                }
                else
                {
                    item.AskVolume = order.Volume - order.ExecutedVolume;
                    item.Asks.Add(order);
                }
                this.Prices.Insert(idx, item);
            }
        }

        public void ReceivePosition(Proto.Position position)
        {
            this.LongPosition = position.LiquidLong;
            this.ShortPosition = position.LiquidShort;
        }

        public Instrument Instrument { get; set; }

        //private Proto.Price price;
        //public Proto.Price Price
        //{
        //    get { return price; }
        //    set { SetProperty(ref price, value); }
        //}

        private double open;
        public double Open
        {
            get { return open; }
            set { SetProperty(ref open, value); }
        }

        private double high;
        public double High
        {
            get { return high; }
            set { SetProperty(ref high, value); }
        }

        private double low;
        public double Low
        {
            get { return low; }
            set { SetProperty(ref low, value); }
        }

        private double close;
        public double Close
        {
            get { return close; }
            set { SetProperty(ref close, value); }
        }

        private double last;
        public double Last
        {
            get { return last; }
            set { SetProperty(ref last, value); }
        }

        private double theo;
        public double Theo
        {
            get { return theo; }
            set { SetProperty(ref theo, value); }
        }        
        
        //private Proto.Position position;
        //public Proto.Position Position
        //{
        //    get { return position; }
        //    set { SetProperty(ref position, value); }
        //}

        private int longPosition;
        public int LongPosition
        {
            get { return longPosition; }
            set { SetProperty(ref longPosition, value); }
        }

        private int shortPosition;
        public int ShortPosition
        {
            get { return shortPosition; }
            set { SetProperty(ref shortPosition, value); }
        }        

        public RangeObservableCollection<PriceItem> Prices { get; set; }
    }

    class PriceItem : BindableBase
    {
        private double price;
	    public double Price
	    {
		    get { return price;}
		    set { SetProperty(ref price, value); }
	    }

        private int marketBidVolume;
        public int MarketBidVolume
        {
            get { return marketBidVolume; }
            set { SetProperty(ref marketBidVolume, value); }
        }

        private int marketAskVolume;
        public int MarketAskVolume
        {
            get { return marketAskVolume; }
            set { SetProperty(ref marketAskVolume, value); }
        }

        private int bidVolume;
        public int BidVolume
        {
            get { return bidVolume; }
            set { SetProperty(ref bidVolume, value); }
        }

        private int askVolume;
        public int AskVolume
        {
            get { return askVolume; }
            set { SetProperty(ref askVolume, value); }
        }

        private List<Proto.Order> bids = new List<Proto.Order>();
        public List<Proto.Order> Bids
        {
            get { return bids; }
            set { SetProperty(ref bids, value); }
        }

        private List<Proto.Order> asks = new List<Proto.Order>();
        public List<Proto.Order> Asks
        {
            get { return asks; }
            set { SetProperty(ref asks, value); }
        }
    }
}
