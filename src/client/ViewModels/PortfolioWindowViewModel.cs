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
    class PortfolioWindowViewModel : BindableBase
    {
        private RangeObservableCollection<PortfolioItem> display;
        public RangeObservableCollection<PortfolioItem> Display
        {
            get { return display; }
            set { SetProperty(ref display, value); }
        }
        
        public PortfolioWindowViewModel(IUnityContainer container, Dispatcher dispatcher)
        {
            this.container = container;
            this.dispatcher = dispatcher;
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);

            PortfolioItem.ExpandingAction = new Action<PortfolioItem>(this.Expanding);
            PortfolioItem.CollapsingAction = new Action<PortfolioItem>(this.Collapsing);
        }

        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            int index = 0;
            this.portfolios = new Dictionary<Proto.Exchange,Dictionary<Instrument,PortfolioItem>>();
            PortfolioItem root = new PortfolioItem()
                {
                    ItemType = PortfolioItemType.Portfolio,
                    Name = "Portfolio",
                    Children = new List<PortfolioItem>(),
                    Index = index++,
                };

            root.SetExpanded();
            var displayItems = new RangeObservableCollection<PortfolioItem>();
            displayItems.Add(root);

            foreach (var exchange in exchanges)
            {
                var pm = this.container.Resolve<ProductManager>(exchange.ToString());
                var positionManager = this.container.Resolve<PositionManager>(exchange.ToString());
                var tradeManager = this.container.Resolve<TradeManager>(exchange.ToString());
                var ssrateManager = this.container.Resolve<SSRateManager>(exchange.ToString());
                if (pm != null && positionManager != null && tradeManager != null)
                {
                    var items = new Dictionary<Instrument, PortfolioItem>();
                    this.portfolios.Add(exchange, items);
                    PortfolioItem exchangeItem = new PortfolioItem()
                    {
                        ItemType = PortfolioItemType.Exchange,
                        Name = exchange.ToString(),
                        Parent = root,
                        Children = new List<PortfolioItem>(),
                        Index = index++,
                    };
                    root.Children.Add(exchangeItem);
                    displayItems.Add(exchangeItem);
                    var hedgeUnderlyings = from underlying in pm.GetHedgeUnderlyings() orderby underlying.Id select underlying;
                    foreach (var hedgeUnderlying in hedgeUnderlyings)
                    {
                        PortfolioItem hedgeUnderlyingItem = new PortfolioItem()
                        {
                            ItemType = PortfolioItemType.Underlying,
                            Name = hedgeUnderlying.Id,
                            Parent = exchangeItem,
                            Children = new List<PortfolioItem>(),
                            Index = -1,
                        };
                        exchangeItem.Children.Add(hedgeUnderlyingItem);
                        var underlyings = pm.GetUnderlyings(hedgeUnderlying);
                        underlyings.Sort((x, y) => x.Symbol.CompareTo(y.Symbol));
                        foreach (var underlying in underlyings)
                        {
                            PortfolioItem maturityItem = new PortfolioItem()
                            {
                                ItemType = PortfolioItemType.Maturity,
                                Parent = hedgeUnderlyingItem,
                                Children = new List<PortfolioItem>(),
                                Index = -1,
                            };
                            hedgeUnderlyingItem.Children.Add(maturityItem);
                            PortfolioItem underlyingItem = new PortfolioItem(underlying)
                            {
                                ItemType = PortfolioItemType.Instrument,
                                Parent = maturityItem,
                                Contract = underlying,
                                Name = underlying.Id,
                                Index = -1,
                                Turnover = tradeManager.GetTurnover(underlying.Id),
                            };

                            var trades = tradeManager.GetTrades(underlying.Id);
                            if (trades.Count() > 0)
                            {
                                int turnover = 0;
                                double fee = 0;
                                foreach (var t in tradeManager.GetTrades(underlying.Id))
                                {
                                    turnover += t.Volume;
                                    fee += TradeManager.GetFee(underlying, t);
                                }
                                underlyingItem.Turnover = turnover;
                                underlyingItem.Fee += fee;
                                maturityItem.Fee += fee;
                                hedgeUnderlyingItem.Fee += fee;
                                exchangeItem.Fee += fee;
                                root.Fee += fee;
                            }

                            Proto.Position p = null;
                            if (positionManager.GetPosition(underlying.Id, out p))
                            {
                                SetPosition(underlyingItem, p);
                                underlyingItem.Delta = underlyingItem.Position;
                            }
                            items.Add(underlying, underlyingItem);
                            maturityItem.Children.Add(underlyingItem);
                            var options = pm.GetOptions(underlying);
                            if (options != null && options.Count > 0)
                            {
                                var sortedOptions = from option in options orderby option.Id select option;
                                maturityItem.Name = sortedOptions.First().Maturity.ToString("yyyyMMdd");
                                underlyingItem.SSRate = ssrateManager.GetSSRate(hedgeUnderlying.Id, sortedOptions.First().Maturity);

                                foreach (var option in sortedOptions)
                                {
                                    PortfolioItem optionItem = new PortfolioItem(option)
                                    {
                                        ItemType = PortfolioItemType.Instrument,
                                        Parent = maturityItem,
                                        Contract = option,
                                        Name = option.Id,
                                        Index = -1,
                                    };
                                    if (positionManager.GetPosition(option.Id, out p))
                                    {
                                        SetPosition(optionItem, p);
                                    }
                                    optionItem.SSRate = underlyingItem.SSRate;
                                    items.Add(option, optionItem);
                                    maturityItem.Children.Add(optionItem);

                                    trades = tradeManager.GetTrades(option.Id);
                                    if (trades.Count() > 0)
                                    {
                                        int turnover = 0;
                                        double fee = 0;
                                        foreach (var t in trades)
                                        {
                                            turnover += t.Volume;
                                            fee += TradeManager.GetFee(option, t);
                                        }
                                        optionItem.Turnover = turnover;
                                        optionItem.Fee = fee;
                                        maturityItem.Turnover += optionItem.Turnover;
                                        maturityItem.Fee += fee;
                                        hedgeUnderlyingItem.Turnover += optionItem.Turnover;
                                        hedgeUnderlyingItem.Fee += fee;
                                        exchangeItem.Turnover += optionItem.Turnover;
                                        exchangeItem.Fee += fee;
                                        root.Turnover += optionItem.Turnover;
                                        root.Fee += fee;
                                    }
                                }
                            }
                            else
                            {
                                maturityItem.Name = underlying.Maturity.ToString("yyyyMMdd");
                            }
                        }
                    }
                }
            }
            this.Display = displayItems;


            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Price>>().Subscribe(this.ReceivePrice, ThreadOption.BackgroundThread);
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Trade>>().Subscribe(this.ReceiveTrade, ThreadOption.BackgroundThread);
            this.container.Resolve<EventAggregator>().GetEvent<GreeksEvent>().Subscribe(this.ReceiveGreeks, ThreadOption.BackgroundThread);
        }

        void Expanding(PortfolioItem item)
        {
            int index = item.Index;
            foreach (var child in item.Children)
            {
                child.Index = ++index;
            }
            for (int i = item.Index + 1; i < this.Display.Count; ++i)
            {
                this.Display[i].Index = ++index;
            }
            this.Display.InsertRange(item.Index + 1, item.Children);
        }

        void Collapsing(PortfolioItem item)
        {
            var child = item.Children.Last();
            int end = child.Index;

            while (child.Children != null)
            {
                child = child.Children.Last();
                if (child.Index > 0)
                {
                    end = child.Index;
                }
                else
                {
                    break;
                }
            }

            for (int i = item.Index + 1; i <= end; ++i)
            {
                var it = this.Display[i];
                it.Index = -1;
                it.UnsetExpanded();
            }

            int idx = item.Index;
            for (int i = end + 1; i < this.Display.Count; ++i)
            {
                this.Display[i].Index = ++idx;
            }
            this.Display.RemoveRange(item.Index + 1, end - item.Index);
        }

        void ReceivePrice(Proto.Price price)
        {
            Dictionary<Instrument, PortfolioItem> items = null;
            if (this.portfolios.TryGetValue(price.Exchange, out items))
            {
                var pm = this.container.Resolve<ProductManager>(price.Exchange.ToString());
                var instrument = pm.FindId(price.Instrument);
                PortfolioItem item = null;
                if (items.TryGetValue(instrument, out item))
                {
                    this.dispatcher.BeginInvoke((MethodInvoker)delegate
                    {
                        item.Last = price.Last.Price;
                        if (item.PreSettlement == 0 && price.PreSettlement > 0)
                        {
                            item.PreSettlement = price.PreSettlement;
                        }
                        if (instrument.Type != Proto.InstrumentType.Option && instrument == instrument.HedgeUnderlying && price.AdjustedPrice > 0)
                        {
                            foreach (var underlying in pm.GetUnderlyings(instrument))
                            {
                                PortfolioItem underlyingItem = null;
                                if (items.TryGetValue(underlying, out underlyingItem) && (double.IsNaN(underlyingItem.SSRate) == false))
                                {
                                    underlyingItem.Theo = price.AdjustedPrice + underlyingItem.SSRate;
                                }
                            }
                        }
                        double marketValueDiff = item.Position * item.Last * item.Contract.Multiplier - item.MarketValue;
                        var parent = item.Parent;
                        while (parent != null)
                        {
                            parent.MarketValue += marketValueDiff;
                            parent = parent.Parent;
                        }
                        item.MarketValue += marketValueDiff;

                        if (instrument.Type == Proto.InstrumentType.Option)
                        {
                            int volumeDiff = price.Volume - item.Volume;
                            parent = item.Parent;
                            while (parent != null)
                            {
                                parent.Volume += volumeDiff;
                                parent = parent.Parent;
                            }
                        }
                        item.Volume = price.Volume;
                    });
                }
            }
        }

        void ReceiveTrade(Proto.Trade trade)
        {
             Dictionary<Instrument, PortfolioItem> items = null;
            if (this.portfolios.TryGetValue(trade.Exchange, out items))
            {
                var pm = this.container.Resolve<ProductManager>(trade.Exchange.ToString());
                var instrument = pm.FindId(trade.Instrument);
                PortfolioItem item = null;
                if (items.TryGetValue(instrument, out item))
                {
                    this.dispatcher.BeginInvoke((MethodInvoker)delegate
                        {
                            double fee = TradeManager.GetFee(instrument, trade);
                            var parent = item.Parent;
                            while (parent != null)
                            {
                                if (instrument.Type == Proto.InstrumentType.Option)
                                {
                                    parent.Turnover += trade.Volume;
                                }
                                parent.Fee += fee;
                                parent = parent.Parent;
                            }
                            item.Turnover += trade.Volume;
                            item.Fee += fee;
                        });
                }
            }
        }

        void ReceiveGreeks(GreeksData greeks)
        {
            Dictionary<Instrument, PortfolioItem> items = null;
            if (this.portfolios.TryGetValue(greeks.Option.Exchange, out items))
            {
                PortfolioItem item = null, underlyingItem = null;
                if (items.TryGetValue(greeks.Option, out item) && items.TryGetValue(greeks.Option.Underlying, out underlyingItem))
                {
                    this.dispatcher.BeginInvoke((MethodInvoker)delegate
                    {
                        item.Volatility = greeks.Greeks.vol;
                        item.TimeValue = greeks.Greeks.time_value;
                        item.Theo = greeks.Greeks.theo;

                        double ratio = item.Contract.Multiplier / item.Contract.Underlying.Multiplier;
                        double deltaDiff = greeks.Greeks.delta * item.Position * ratio - item.Delta;
                        double cashDeltaDiff = greeks.Greeks.delta * item.Position * underlyingItem.Last * item.Contract.Multiplier - item.CashDelta;
                        double dayDeltaDiff = greeks.Greeks.delta * item.ChangedPosition * ratio - item.DayDelta;
                        double dayCashDeltaDiff = greeks.Greeks.delta * item.ChangedPosition * underlyingItem.Last * item.Contract.Multiplier - item.DayCashDelta;
                        double gammaDiff = greeks.Greeks.gamma * item.Position * ratio - item.Gamma;
                        double cashGammaDiff = greeks.Greeks.gamma * item.Position * underlyingItem.Last * underlyingItem.Last * item.Contract.Multiplier / 100 - item.CashGamma;
                        double vegaDiff = greeks.Greeks.vega * item.Position * item.Contract.Multiplier - item.Vega;
                        double dayVegaDiff = greeks.Greeks.vega * item.ChangedPosition * item.Contract.Multiplier - item.DayVega;
                        double thetaDiff = greeks.Greeks.theta * item.Position * item.Contract.Multiplier - item.Theta;
                        double volThetaDiff = greeks.Greeks.vol_theta * item.Position * item.Contract.Multiplier - item.VolTheta;
                        double rateThetaDiff = greeks.Greeks.rate_theta * item.Position * item.Contract.Multiplier - item.RateTheta;
                        double rho1Diff = greeks.Greeks.rho1 / 100 * item.Position * item.Contract.Multiplier - item.Rho1;
                        double rho2Diff = greeks.Greeks.rho2 / 100 * item.Position * item.Contract.Multiplier - item.Rho2;
                        double charmDiff = greeks.Charm * item.Position * ratio - item.Charm;
                        double cashCharmDiff = greeks.Charm * item.Position * underlyingItem.Last * item.Contract.Multiplier - item.CashCharm;
                        double skewSensiDiff = greeks.SkewSensi * item.Position * item.Contract.Multiplier - item.SkewSensi;
                        double callConvexDiff = 0, putConvexDiff = 0;
                        if (double.IsNaN(greeks.CallConvexSensi) == false)
                        {
                            callConvexDiff = greeks.CallConvexSensi * item.Position * item.Contract.Multiplier - item.CallConvexSensi;
                        }
                        else if (double.IsNaN(greeks.PutConvexSensi) == false)
                        {
                            putConvexDiff = greeks.PutConvexSensi * item.Position * item.Contract.Multiplier - item.PutConvexSensi;
                        }
                        var parent = item.Parent;
                        while (parent != null)
                        {
                            parent.Delta += deltaDiff;
                            parent.CashDelta += cashDeltaDiff;
                            parent.DayDelta += dayDeltaDiff;
                            parent.DayCashDelta += dayCashDeltaDiff;
                            parent.Gamma += gammaDiff;
                            parent.CashGamma += cashGammaDiff;
                            parent.Vega += vegaDiff;
                            parent.DayVega += dayVegaDiff;
                            parent.Theta += thetaDiff;
                            parent.VolTheta += volThetaDiff;
                            parent.RateTheta += rateThetaDiff;
                            parent.Rho1 += rho1Diff;
                            parent.Rho2 += rho2Diff;
                            parent.Charm += charmDiff;
                            parent.CashCharm += cashCharmDiff;
                            parent.SkewSensi += skewSensiDiff;
                            parent.CallConvexSensi += callConvexDiff;
                            parent.PutConvexSensi += putConvexDiff;
                            parent = parent.Parent;
                        }
                        item.Delta += deltaDiff;
                        item.CashDelta += cashDeltaDiff;
                        item.DayDelta += dayDeltaDiff;
                        item.DayCashDelta += dayCashDeltaDiff;
                        item.Gamma += gammaDiff;
                        item.CashGamma += cashGammaDiff;
                        item.Vega += vegaDiff;
                        item.DayVega += dayVegaDiff;
                        item.Theta += thetaDiff;
                        item.VolTheta += volThetaDiff;
                        item.RateTheta += rateThetaDiff;
                        item.Rho1 += rho1Diff;
                        item.Rho2 += rho2Diff;
                        item.Charm += charmDiff;
                        item.CashCharm += cashCharmDiff;
                        item.SkewSensi += skewSensiDiff;
                        item.CallConvexSensi += callConvexDiff;
                        item.PutConvexSensi += putConvexDiff;
                    });
                }
            }
        }

        void SetPosition(PortfolioItem item, Proto.Position p)
        {
            item.LongPosition = p.TotalLong;
            item.ShortPosition = p.TotalShort;
            item.Position = item.LongPosition - item.ShortPosition;
            item.PrePosition = p.YesterdayLong - p.YesterdayShort;
            item.ChangedPosition = item.Position - item.PrePosition;
        }

        private IUnityContainer container;
        private Dispatcher dispatcher;
        Dictionary<Proto.Exchange, Dictionary<Instrument, PortfolioItem>> portfolios;
    }

    enum PortfolioItemType
    {
        Portfolio,
        Exchange,
        Underlying,
        Maturity,
        Instrument,
    }

    class PortfolioItem : BindableBase
    {
        public static Action<PortfolioItem> ExpandingAction;
        public static Action<PortfolioItem> CollapsingAction;

        //public PortfolioType Type  { get; set; }
        public PortfolioItem()
        {
            timeValue = double.NaN;
            volatility = double.NaN;
            rate = double.NaN;
            ssrate = double.NaN;
            theo = double.NaN;
            last = double.NaN;
            preSettlement = double.NaN;
            position = double.NaN;
            longPosition = double.NaN;
            shortPosition = double.NaN;
            prePosition = double.NaN;
            changedPosition = double.NaN;
        }

        public PortfolioItem(Instrument instrument)
        {
            this.Maturity = instrument.Maturity.ToString("yyyyMMdd");
            this.Multiple = instrument.Multiplier.ToString();
            this.Type = instrument.Type.ToString();
            timeValue = double.NaN;
            volatility = double.NaN;
            rate = double.NaN;
            ssrate = double.NaN;
        }

        public PortfolioItem(Option option)
        {
            this.Strike = option.Strike.ToString();
            this.Maturity = option.Maturity.ToString("yyyyMMdd");
            this.Multiple = option.Multiplier.ToString();
            this.Type = option.OptionType.ToString();
            this.ssrate = double.NaN;
        }

        public string Strike { get; private set; }
        public string Product { get; private set; }
        public string Maturity { get; private set; }
        public string Type { get; private set; }
        public string Multiple { get; private set; }

        public PortfolioItemType ItemType { get; set; }
        public int Index { get; set; }

        private bool isExpanded;
        public bool IsExpanded
        {
            get { return isExpanded; }
            set
            {
                if (SetProperty(ref isExpanded, value))
                {
                    if (isExpanded)
                    {
                        ExpandingAction(this);
                    }
                    else
                    {
                        CollapsingAction(this);
                    }
                }
            }
        }

        public void SetExpanded()
        {
            SetProperty(ref isExpanded, true);
        }

        public void UnsetExpanded()
        {
            SetProperty(ref isExpanded, false);
        }

        public bool IsExpandable
        {
            get { return Children != null && Children.Count > 0; }
        }

        public List<PortfolioItem> Children { get; set; }

        private PortfolioItem parent;
        public PortfolioItem Parent
        {
            get { return parent; }
            set
            {
                parent = value;
                if (parent != null)
                {
                    Level = parent.Level + 1;
                }
            }
        }        

        public int Level { get; set; }

        private string name;
        public string Name
        {
            get { return name; }
            set { SetProperty(ref name, value); }
        }

        private Instrument contract;
        public Instrument Contract
        {
            get { return contract; }
            set { SetProperty(ref contract, value); }
        }

        private double last;
        public double Last
        {
            get { return last; }
            set { SetProperty(ref last, value); }
        }

        private int volume;
        public int Volume
        {
            get { return volume; }
            set { SetProperty(ref volume, value); }
        }

        private int turnover;
        public int Turnover
        {
            get { return turnover; }
            set { SetProperty(ref turnover, value); }
        }        

        private double preSettlement;
        public double PreSettlement
        {
            get { return preSettlement; }
            set { SetProperty(ref preSettlement, value); }
        }

        private double rate;
        public double Rate
        {
            get { return rate; }
            set { SetProperty(ref rate, value); }
        }

        private double ssrate;
        public double SSRate
        {
            get { return ssrate; }
            set { SetProperty(ref ssrate, value); }
        }

        private double volatility;
        public double Volatility
        {
            get { return volatility; }
            set { SetProperty(ref volatility, value); }
        }

        private double timeValue;
        public double TimeValue
        {
            get { return timeValue; }
            set { SetProperty(ref timeValue, value); }
        }

        private double destrike;
        public double Destrike
        {
            get { return destrike; }
            set { SetProperty(ref destrike, value); }
        }

        private double theo;
        public double Theo
        {
            get { return theo; }
            set { SetProperty(ref theo, value); }
        }

        private double delta;
        public double Delta
        {
            get { return delta; }
            set { SetProperty(ref delta, value); }
        }

        private double dayDelta;
        public double DayDelta
        {
            get { return dayDelta; }
            set { SetProperty(ref dayDelta, value); }
        }

        private double cashDelta;
        public double CashDelta
        {
            get { return cashDelta; }
            set { SetProperty(ref cashDelta, value); }
        }

        private double dayCashDelta;
        public double DayCashDelta
        {
            get { return dayCashDelta; }
            set { SetProperty(ref dayCashDelta, value); }
        }

        private double gamma;
        public double Gamma
        {
            get { return gamma; }
            set { SetProperty(ref gamma, value); }
        }

        private double cashGamma;
        public double CashGamma
        {
            get { return cashGamma; }
            set { SetProperty(ref cashGamma, value); }
        }

        private double vega;
        public double Vega
        {
            get { return vega; }
            set { SetProperty(ref vega, value); }
        }

        private double dayVega;
        public double DayVega
        {
            get { return dayVega; }
            set { SetProperty(ref dayVega, value); }
        }

        private double theta;
        public double Theta
        {
            get { return theta; }
            set { SetProperty(ref theta, value); }
        }

        private double volTheta;
        public double VolTheta
        {
            get { return volTheta; }
            set { SetProperty(ref volTheta, value); }
        }

        private double rateTheta;
        public double RateTheta
        {
            get { return rateTheta; }
            set { SetProperty(ref rateTheta, value); }
        }

        private double rho1;
        public double Rho1
        {
            get { return rho1; }
            set { SetProperty(ref rho1, value); }
        }

        private double rho2;
        public double Rho2
        {
            get { return rho2; }
            set { SetProperty(ref rho2, value); }
        }

        private double charm;
        public double Charm
        {
            get { return charm; }
            set { SetProperty(ref charm, value); }
        }

        private double cashCharm;
        public double CashCharm
        {
            get { return cashCharm; }
            set { SetProperty(ref cashCharm, value); }
        }

        private double skewSensi;
        public double SkewSensi
        {
            get { return skewSensi; }
            set { SetProperty(ref skewSensi, value); }
        }

        private double callConvexSensi;
        public double CallConvexSensi
        {
            get { return callConvexSensi; }
            set { SetProperty(ref callConvexSensi, value); }
        }

        private double putConvexSensi;
        public double PutConvexSensi
        {
            get { return putConvexSensi; }
            set { SetProperty(ref putConvexSensi, value); }
        }

        private double position;
        public double Position
        {
            get { return position; }
            set { SetProperty(ref position, value); }
        }

        private double longPosition;
        public double LongPosition
        {
            get { return longPosition; }
            set { SetProperty(ref longPosition, value); }
        }

        private double shortPosition;
        public double ShortPosition
        {
            get { return shortPosition; }
            set { SetProperty(ref shortPosition, value); }
        }

        private double prePosition;
        public double PrePosition
        {
            get { return prePosition; }
            set { SetProperty(ref prePosition, value); }
        }

        private double changedPosition;
        public double ChangedPosition
        {
            get { return changedPosition; }
            set { SetProperty(ref changedPosition, value); }
        }

        private double theoPL;
        public double TheoPL
        {
            get { return theoPL; }
            set { SetProperty(ref theoPL, value); }
        }

        private double theoDayPL;
        public double TheoDayPL
        {
            get { return theoDayPL; }
            set { SetProperty(ref theoDayPL, value); }
        }

        private double theoPosPL;
        public double TheoPosPL
        {
            get { return theoPosPL; }
            set { SetProperty(ref theoPosPL, value); }
        }

        private double lastPL;
        public double LastPL
        {
            get { return lastPL; }
            set { SetProperty(ref lastPL, value); }
        }

        private double lastDayPL;
        public double LastDayPL
        {
            get { return lastDayPL; }
            set { SetProperty(ref lastDayPL, value); }
        }

        private double lastPosPL;
        public double LastPosPL
        {
            get { return lastPosPL; }
            set { SetProperty(ref lastPosPL, value); }
        }

        private double marketValue;
        public double MarketValue
        {
            get { return marketValue; }
            set { SetProperty(ref marketValue, value); }
        }

        private double margin;
        public double Margin
        {
            get { return margin; }
            set { SetProperty(ref margin, value); }
        }

        private double fee;
        public double Fee
        {
            get { return fee; }
            set { SetProperty(ref fee, value); }
        }
        
    }
}
