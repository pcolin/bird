using client.Models;
using Microsoft.Practices.Unity;
using Prism.Events;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;
using System.Windows.Forms;
using System.Windows.Threading;

namespace client.ViewModels
{
    class TradeWindowViewModel : BindableBase
    {
        public TradeWindowViewModel(IUnityContainer container, Dispatcher dispatcher)
        {
            this.Items = new ObservableCollection<TradeItem>();
            var exchanges = new ObservableCollection<FilterItem<Proto.Exchange>>();
            exchanges.Add(new FilterItem<Proto.Exchange>(this.FilterExchange));
            this.Exchanges = exchanges;

            //Func<Instrument, string> instrumentFunc = i => i.Id;
            var instruments = new ObservableCollection<FilterItem<Instrument>>();
            instruments.Add(new FilterItem<Instrument>(this.FilterInstrument));
            this.Instruments = instruments;

            var underlyings = new ObservableCollection<FilterItem<Instrument>>();
            underlyings.Add(new FilterItem<Instrument>(this.FilterUnderlying));
            this.Underlyings = underlyings;

            //Func<Proto.Side, string> sideFunc = s => s.ToString();
            var sides = new ObservableCollection<FilterItem<Proto.Side>>();
            sides.Add(new FilterItem<Proto.Side>(this.FilterSide));
            foreach (var s in Enum.GetValues(typeof(Proto.Side)))
            {
                sides.Add(new FilterItem<Proto.Side>(this.FilterSide, x => x.ToString(), (Proto.Side)s));
            }
            this.Sides = sides;
            
            //Func<Proto.StrategyType, string> strategyFunc = s => s.ToString();
            var strategies = new ObservableCollection<FilterItem<Proto.StrategyType>>();
            strategies.Add(new FilterItem<Proto.StrategyType>(this.FilterStrategy));
            foreach (var s in Enum.GetValues(typeof(Proto.StrategyType)))
            {
                strategies.Add(new FilterItem<Proto.StrategyType>(this.FilterStrategy, x => x.ToString(), (Proto.StrategyType)s));
            }
            this.Strategies = strategies;

            //Func<Proto.InstrumentType, string> typeFunc = t => t.ToString();
            var types = new ObservableCollection<FilterItem<Proto.InstrumentType>>();
            types.Add(new FilterItem<Proto.InstrumentType>(this.FilterType));
            foreach (var t in Enum.GetValues(typeof(Proto.InstrumentType)))
            {
                types.Add(new FilterItem<Proto.InstrumentType>(this.FilterType, x => x.ToString(), (Proto.InstrumentType)t));
            }
            this.Types = types;

            this.container = container;
            this.dispatcher = dispatcher;

            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
        }

        public ICollectionView TradeView { get; set; }
        private ObservableCollection<TradeItem> items;
        private ObservableCollection<TradeItem> Items
        {
            get { return items; }
            set
            {
                if (SetProperty(ref items, value))
                {
                    TradeView = CollectionViewSource.GetDefaultView(value);
                    TradeView.Filter = this.Filter;
                }
            }
        }

        public ICollectionView ExchangesView { get; set; }
        private ObservableCollection<FilterItem<Proto.Exchange>> exchanges;
        private ObservableCollection<FilterItem<Proto.Exchange>> Exchanges
        {
            get { return exchanges; }
            set
            {
                if (SetProperty(ref exchanges, value))
                {
                    ExchangesView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView InstrumentsView { get; set; }
        private ObservableCollection<FilterItem<Instrument>> instruments;
        private ObservableCollection<FilterItem<Instrument>> Instruments
        {
            get { return instruments; }
            set
            {
                if (SetProperty(ref instruments, value))
                {
                    InstrumentsView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView SidesView { get; set; }
        private ObservableCollection<FilterItem<Proto.Side>> sides;
        private ObservableCollection<FilterItem<Proto.Side>> Sides
        {
            get { return sides; }
            set
            {
                if (SetProperty(ref sides, value))
                {
                    SidesView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView StrategiesView { get; set; }
        private ObservableCollection<FilterItem<Proto.StrategyType>> strategies;
        public ObservableCollection<FilterItem<Proto.StrategyType>> Strategies
        {
            get { return strategies; }
            set
            {
                if (SetProperty(ref strategies, value))
                {
                    StrategiesView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView TypesView { get; set; }
        private ObservableCollection<FilterItem<Proto.InstrumentType>> types;
        private ObservableCollection<FilterItem<Proto.InstrumentType>> Types
        {
            get { return types; }
            set
            {
                if (SetProperty(ref types, value))
                {
                    TypesView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView UnderlyingsView { get; set; }
        private ObservableCollection<FilterItem<Instrument>> underlyings;
        private ObservableCollection<FilterItem<Instrument>> Underlyings
        {
            get { return underlyings; }
            set
            {
                if (SetProperty(ref underlyings, value))
                {
                    UnderlyingsView = CollectionViewSource.GetDefaultView(value);
                }
            }
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

                var pm = this.container.Resolve<ProductManager>(exchange.ToString());
                this.productManagers[exchange] = pm;
                this.orderManagers[exchange] = this.container.Resolve<OrderManager>(exchange.ToString());
                this.theoCalculators[exchange] = this.container.Resolve<TheoCalculator>(exchange.ToString());

                this.dispatcher.Invoke((MethodInvoker)delegate
                {
                    this.Exchanges.Add(new FilterItem<Proto.Exchange>(this.FilterExchange, x => x.ToString(), exchange));
                    foreach (var underlying in pm.GetUnderlyings().OrderBy(x => x.Id))
                    {
                        this.Underlyings.Add(new FilterItem<Instrument>(this.FilterUnderlying, x => x.Id, underlying));
                        this.Instruments.Add(new FilterItem<Instrument>(this.FilterInstrument, x => x.Id, underlying));
                        foreach (var option in pm.GetOptions(underlying).OrderBy(x => x.Id))
                        {
                            this.Instruments.Add(new FilterItem<Instrument>(this.FilterInstrument, x => x.Id, option));
                        }
                    }
                });
            }

            //var items = new ObservableCollection<TradeItem>();
            this.dispatcher.Invoke((MethodInvoker)delegate
                {
                    foreach (var trade in trades.OrderBy(t => t.Time))
                    {
                        var inst = productManagers[trade.Exchange].FindId(trade.Instrument);
                        if (inst != null)
                        {
                            this.Items.Add(new TradeItem(inst, orderManagers[trade.Exchange].FindOrder(trade.OrderId), trade));
                        }
                    }
                });

            //this.Items = items;
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

        private bool Filter(object item)
        {
            var it = item as TradeItem;
            return !(excludedExchanges.Contains(it.Instrument.Exchange) || excludedInstruments.Contains(it.Instrument) || excludedSides.Contains(it.Order.Side) ||
                excludedStrategies.Contains(it.Order.StrategyType) || excludedTypes.Contains(it.Instrument.Type) || excludedUnderlyings.Contains(it.Instrument.Underlying));
        }

        private void FilterExchange(bool selected, bool all, Proto.Exchange exchange)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedExchanges.Clear();
                    for (int i = 1; i < this.Exchanges.Count; ++i)
                    {
                        this.Exchanges[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedExchanges.Remove(exchange);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Exchanges.Count; ++i)
                {
                    this.Exchanges[i].SetIsSelected(false);
                    this.excludedExchanges.Add(this.Exchanges[i].Item);
                }
            }
            else
            {
                this.Exchanges[0].SetIsSelected(false);
                this.excludedExchanges.Add(exchange);
            }
            ExchangesView.Refresh();
            TradeView.Refresh();
        }

        private void FilterInstrument(bool selected, bool all, Instrument instrument)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedInstruments.Clear();
                    for (int i = 1; i < this.Instruments.Count; ++i)
                    {
                        this.Instruments[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedInstruments.Remove(instrument);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Instruments.Count; ++i)
                {
                    this.Instruments[i].SetIsSelected(false);
                    this.excludedInstruments.Add(this.Instruments[i].Item);
                }
            }
            else
            {
                this.Instruments[0].SetIsSelected(false);
                this.excludedInstruments.Add(instrument);
            }
            InstrumentsView.Refresh();
            TradeView.Refresh();
        }

        private void FilterUnderlying(bool selected, bool all, Instrument underlying)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedUnderlyings.Clear();
                    for (int i = 1; i < this.Underlyings.Count; ++i)
                    {
                        this.Underlyings[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedUnderlyings.Remove(underlying);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Underlyings.Count; ++i)
                {
                    this.Underlyings[i].SetIsSelected(false);
                    this.excludedUnderlyings.Add(this.Underlyings[i].Item);
                }
            }
            else
            {
                this.Underlyings[0].SetIsSelected(false);
                this.excludedUnderlyings.Add(underlying);
            }
            UnderlyingsView.Refresh();
            TradeView.Refresh();
        }

        private void FilterSide(bool selected, bool all, Proto.Side side)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedSides.Clear();
                    for (int i = 1; i < this.Sides.Count; ++i)
                    {
                        this.Sides[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedSides.Remove(side);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Sides.Count; ++i)
                {
                    this.Sides[i].SetIsSelected(false);
                    this.excludedSides.Add(this.Sides[i].Item);
                }
            }
            else
            {
                this.Sides[0].SetIsSelected(true);
                this.excludedSides.Add(side);
            }
            SidesView.Refresh();
            TradeView.Refresh();
        }
        
        private void FilterStrategy(bool selected, bool all, Proto.StrategyType strategy)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedStrategies.Clear();
                    for (int i = 1; i < this.Strategies.Count; ++i)
                    {
                        this.Strategies[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedStrategies.Remove(strategy);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Strategies.Count; ++i)
                {
                    this.Strategies[i].SetIsSelected(false);
                    this.excludedStrategies.Add(this.Strategies[i].Item);
                }
            }
            else
            {
                this.Strategies[0].SetIsSelected(true);
                this.excludedStrategies.Add(strategy);
            }
            StrategiesView.Refresh();
            TradeView.Refresh();
        }

        private void FilterType(bool selected, bool all, Proto.InstrumentType type)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedTypes.Clear();
                    for (int i = 1; i < this.Types.Count; ++i)
                    {
                        this.Types[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedTypes.Remove(type);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Types.Count; ++i)
                {
                    this.Types[i].SetIsSelected(false);
                    this.excludedTypes.Add(this.Types[i].Item);
                }
            }
            else
            {
                this.Types[0].SetIsSelected(true);
                this.excludedTypes.Add(type);
            }
            TypesView.Refresh();
            TradeView.Refresh();
        }

        private IUnityContainer container;
        private Dispatcher dispatcher;

        private Dictionary<Proto.Exchange, ProductManager> productManagers;
        private Dictionary<Proto.Exchange, OrderManager> orderManagers;
        private Dictionary<Proto.Exchange, TheoCalculator> theoCalculators;
        private HashSet<Proto.Exchange> excludedExchanges = new HashSet<Proto.Exchange>();
        private HashSet<Instrument> excludedInstruments = new HashSet<Instrument>();
        private HashSet<Proto.Side> excludedSides = new HashSet<Proto.Side>();
        private HashSet<Proto.StrategyType> excludedStrategies = new HashSet<Proto.StrategyType>();
        private HashSet<Proto.InstrumentType> excludedTypes = new HashSet<Proto.InstrumentType>();
        private HashSet<Instrument> excludedUnderlyings = new HashSet<Instrument>();
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

            Fee = TradeManager.GetFee(inst, trade);
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
