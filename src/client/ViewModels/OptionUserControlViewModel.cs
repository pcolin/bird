using client.Models;
using Microsoft.Practices.Unity;
using ModelLibrary;
using Prism.Commands;
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
using System.Windows.Input;

namespace client.ViewModels
{
    public class OptionUserControlViewModel : BindableBase
    {
        public ICommand QuoterCallBidSwitchCommand { get; set; }
        public ICommand QuoterCallAskSwitchCommand { get; set; }
        public ICommand QuoterPutBidSwitchCommand { get; set; }
        public ICommand QuoterPutAskSwitchCommand { get; set; }

        public ICommand DummyQuoterCallBidSwitchCommand { get; set; }
        public ICommand DummyQuoterCallAskSwitchCommand { get; set; }
        public ICommand DummyQuoterPutBidSwitchCommand { get; set; }
        public ICommand DummyQuoterPutAskSwitchCommand { get; set; }

        public ICommand HitterCallBidSwitchCommand { get; set; }
        public ICommand HitterCallAskSwitchCommand { get; set; }
        public ICommand HitterPutBidSwitchCommand { get; set; }
        public ICommand HitterPutAskSwitchCommand { get; set; }

        public ICommand DimerCallBidSwitchCommand { get; set; }
        public ICommand DimerCallAskSwitchCommand { get; set; }
        public ICommand DimerPutBidSwitchCommand { get; set; }
        public ICommand DimerPutAskSwitchCommand { get; set; }

        public ICommand CoverCallSwitchCommand { get; set; }
        public ICommand CoverPutSwitchCommand { get; set; }

        public ICommand EnquiryResponseCallSwitchCommand { get; set; }
        public ICommand EnquiryResponsePutSwitchCommand { get; set; }

        public Proto.Exchange Exchange
        {
            get
            {
                if (hedgeUnderlying != null)
                {
                    return hedgeUnderlying.Exchange;
                }
                return Proto.Exchange.Dce;
            }
        }        

        public string Underlying
        {
            get
            {
                if (hedgeUnderlying != null)
                {
                    return hedgeUnderlying.Symbol;
                }
                return string.Empty;
            }
        }

        private Instrument hedgeUnderlying;
        public Instrument HedgeUnderlying
        {
            get { return hedgeUnderlying; }
            set
            {
                if (SetProperty(ref hedgeUnderlying, value))
                {
                    //Underlying = value.Symbol;
                    RaisePropertyChanged("Underlying");
                }
            }
        }

        private Dictionary<Instrument, UnderlyingItem> underlyingItems;
        private ICollectionView underlyings;
        public ICollectionView Underlyings
        {
            get { return underlyings; }
            set { SetProperty(ref underlyings, value); }
        }

        private Dictionary<Option, OptionItem> optionItems;
        private ObservableCollection<OptionPairItem> options;
        public ObservableCollection<OptionPairItem> Options
        {
            get { return options; }
            set { SetProperty(ref options, value); }
        }

        public OptionUserControlViewModel(Proto.Exchange exchange, Instrument hedgeUnderlying, ProductManager products, IUnityContainer container)
        {
            this.container = container;
            this.exchange = exchange;
            this.productManager = products;
            //productManager = container.Resolve<ProductManager>(exchange.ToString());
            ssrateManager = container.Resolve<SSRateManager>(exchange.ToString());
            serverService = container.Resolve<ServerService>(exchange.ToString());
            var positions = container.Resolve<PositionManager>(exchange.ToString());
            var dm = container.Resolve<DestrikerManager>(exchange.ToString());

            List<UnderlyingItem> items = new List<UnderlyingItem>();
            underlyingItems = new Dictionary<Instrument, UnderlyingItem>();
            var underlyings = products.GetUnderlyings(hedgeUnderlying);
            underlyings.Sort((x, y) => x.Symbol.CompareTo(y.Symbol));
            foreach (var inst in underlyings)
            {                
                var item = new UnderlyingItem() { Underlying = inst, Status = inst.Status, Position = positions.GetPosition(inst.Id)};
                if (inst.Underlying == hedgeUnderlying)
                {
                    this.HedgeUnderlying = inst;
                    items.Insert(0, item);
                }
                else
                {
                    items.Add(item);
                }

                var opts = products.GetOptions(inst);
                if (opts != null && opts.Count > 0)
                {
                    item.Maturity = opts[0].Maturity;
                }

                underlyingItems[inst] = item;
            }
            this.underlyings = new ListCollectionView(items);

            //var qm = this.container.Resolve<QuoterManager>(exchange.ToString());
            var cm = this.container.Resolve<CreditManager>(exchange.ToString());
            var sm = this.container.Resolve<StrategySwitchManager>(exchange.ToString());
            var tc = this.container.Resolve<TheoCalculator>(exchange.ToString());
            List<OptionPairItem> optionPairItems = new List<OptionPairItem>();
            List<OptionPairItem> dominantOptionPairItems = new List<OptionPairItem>();
            this.optionItems = new Dictionary<Option, OptionItem>();
            var options = products.GetOptionsByHedgeUnderlying(hedgeUnderlying);
            if (options != null)
            {
                var orders = new Dictionary<string, Tuple<List<Proto.Order>,List<Proto.Order>>>();
                var om = this.container.Resolve<OrderManager>(exchange.ToString());
                if (om != null)
                {
                    foreach(var ord in om.GetOrders())
                    {
                        Tuple<List<Proto.Order>, List<Proto.Order>> t = null;
                        if (orders.TryGetValue(ord.Instrument, out t) == false)
                        {
                            t = Tuple.Create(new List<Proto.Order>(), new List<Proto.Order>());
                            orders.Add(ord.Instrument, t);
                        }
                        if (OrderManager.IsInactive(ord) == false)
                        {
                            if (OrderManager.IsBid(ord.Side))
                            {
                                t.Item1.Add(ord);
                            }
                            else
                            {
                                t.Item2.Add(ord);
                            }
                        }
                    }
                }

                var ret = options.OrderBy(x => x.Underlying.Id).ThenBy(x => x.Strike).ThenBy(x => x.OptionType);
                Option call = null;
                DateTime maturity = DateTime.MinValue;
                Dictionary<string, double> quoterCredits = null;
                Dictionary<string, double> hitterCredits = null;
                Dictionary<string, double> dimerCredits = null;
                int groupNo = 0;
                foreach (var inst in ret)
                {
                    if (inst.OptionType == Proto.OptionType.Put)
                    {
                        if (call != null && call.Underlying == inst.Underlying && call.Strike == inst.Strike)
                        {
                            bool isFirst = false;
                            if (inst.Maturity != maturity)
                            {
                                if (inst.Underlying != hedgeUnderlying)
                                {
                                    ++groupNo;
                                    isFirst = true;
                                }
                                quoterCredits = GetCredits(cm, Proto.StrategyType.Quoter, inst.HedgeUnderlying.Id, inst.Maturity);
                                hitterCredits = GetCredits(cm, Proto.StrategyType.Hitter, inst.HedgeUnderlying.Id, inst.Maturity);
                                dimerCredits = GetCredits(cm, Proto.StrategyType.Dimer, inst.HedgeUnderlying.Id, inst.Maturity);
                            }
                            var callItem = new OptionItem()
                                {
                                    Option = call,
                                    Status = call.Status,
                                    Destriker = dm.GetDestriker(call.Id),
                                };
                            Proto.Price price = tc.GetPrice(call);
                            if (price != null)
                            {
                                ReceiveOptionPrice(callItem, price);
                            }
                            Tuple<List<Proto.Order>, List<Proto.Order>> t = null;
                            if (orders.TryGetValue(call.Id, out t))
                            {
                                callItem.Bids = (from order in t.Item1 orderby order.Price descending select order).ToList();
                                callItem.Asks = (from order in t.Item2 orderby order.Price ascending select order).ToList();
                            }
                            else
                            {
                                callItem.Bids = new List<Proto.Order>();
                                callItem.Asks = new List<Proto.Order>();
                            }
                            GreeksData greeks = tc.GetGreeks(call);
                            if (greeks != null)
                            {
                                ReceiveGreeks(callItem, greeks);
                            }
                            ImpliedVolatilityData iv = tc.GetIV(call);
                            if (iv != null)
                            {
                                ReceiveIV(callItem, iv);
                            }
                            Proto.Position p = null;
                            if (positions.GetPosition(call.Id, out p))
                            {
                                ReceiveOptionPosition(callItem, p);
                            }
                            double credit = 0;
                            if (quoterCredits.TryGetValue(call.Id, out credit))
                            {
                                callItem.QuoterCredit = credit;
                            }
                            if (hitterCredits.TryGetValue(call.Id, out credit))
                            {
                                callItem.HitterCredit = credit;
                            }
                            if (dimerCredits.TryGetValue(call.Id, out credit))
                            {
                                callItem.DimerCredit = credit;
                            }
                            var switches = sm.GetStrategySwitch(call.Id);
                            if (switches != null)
                            {
                                int idx = (int)Proto.StrategyType.Quoter;
                                if (switches[idx] != null)
                                {
                                    callItem.QuoterBidOn = switches[idx].IsBid;
                                    callItem.QuoterAskOn = switches[idx].IsAsk;
                                    callItem.EnquiryResponseOn = switches[idx].IsQrCover;
                                }
                                idx = (int)Proto.StrategyType.Hitter;
                                if (switches[idx] != null)
                                {
                                    callItem.HitterBidOn = switches[idx].IsBid;
                                    callItem.HitterAskOn = switches[idx].IsAsk;
                                    callItem.CoverOn = switches[idx].IsQrCover;
                                }
                                idx = (int)Proto.StrategyType.Dimer;
                                if (switches[idx] != null)
                                {
                                    callItem.DimerBidOn = switches[idx].IsBid;
                                    callItem.DimerAskOn = switches[idx].IsAsk;
                                    callItem.CoverOn = switches[idx].IsQrCover;
                                }
                                idx = (int)Proto.StrategyType.DummyQuoter;
                                if (switches[idx] != null)
                                {
                                    callItem.DummyQuoterBidOn = switches[idx].IsBid;
                                    callItem.DummyQuoterAskOn = switches[idx].IsAsk;
                                }
                            }
                            var putItem = new OptionItem()
                                {
                                    Option = inst,
                                    Status = call.Status,
                                    Destriker = dm.GetDestriker(inst.Id),
                                };
                            price = tc.GetPrice(inst);
                            if (price != null)
                            {
                                ReceiveOptionPrice(putItem, price);
                            }
                            if (orders.TryGetValue(inst.Id, out t))
                            {
                                putItem.Bids = (from order in t.Item1 orderby order.Price descending select order).ToList();
                                putItem.Asks = (from order in t.Item2 orderby order.Price ascending select order).ToList();
                            }
                            else
                            {
                                putItem.Bids = new List<Proto.Order>();
                                putItem.Asks = new List<Proto.Order>();
                            }
                            greeks = tc.GetGreeks(inst);
                            if (greeks != null)
                            {
                                ReceiveGreeks(putItem, greeks);
                            }
                            iv = tc.GetIV(inst);
                            if (iv != null)
                            {
                                ReceiveIV(putItem, iv);
                            }
                            if (positions.GetPosition(inst.Id, out p))
                            {
                                ReceiveOptionPosition(putItem, p);
                            }
                            if (quoterCredits.TryGetValue(call.Id, out credit))
                            {
                                putItem.QuoterCredit = credit;
                            }
                            if (hitterCredits.TryGetValue(call.Id, out credit))
                            {
                                putItem.HitterCredit = credit;
                            }
                            if (dimerCredits.TryGetValue(call.Id, out credit))
                            {
                                putItem.DimerCredit = credit;
                            }
                            switches = sm.GetStrategySwitch(inst.Id);
                            if (switches != null)
                            {
                                int idx = (int)Proto.StrategyType.Quoter;
                                if (switches[idx] != null)
                                {
                                    putItem.QuoterBidOn = switches[idx].IsBid;
                                    putItem.QuoterAskOn = switches[idx].IsAsk;
                                    putItem.EnquiryResponseOn = switches[idx].IsQrCover;
                                }
                                idx = (int)Proto.StrategyType.Hitter;
                                if (switches[idx] != null)
                                {
                                    putItem.HitterBidOn = switches[idx].IsBid;
                                    putItem.HitterAskOn = switches[idx].IsAsk;
                                    putItem.CoverOn = switches[idx].IsQrCover;
                                }
                                idx = (int)Proto.StrategyType.Dimer;
                                if (switches[idx] != null)
                                {
                                    putItem.DimerBidOn = switches[idx].IsBid;
                                    putItem.DimerAskOn = switches[idx].IsAsk;
                                    putItem.CoverOn = switches[idx].IsQrCover;
                                }
                                idx = (int)Proto.StrategyType.DummyQuoter;
                                if (switches[idx] != null)
                                {
                                    putItem.DummyQuoterBidOn = switches[idx].IsBid;
                                    putItem.DummyQuoterAskOn = switches[idx].IsAsk;
                                }
                            }

                            if (inst.Underlying == hedgeUnderlying)
                            {
                                dominantOptionPairItems.Add(new OptionPairItem() { Call = callItem, Put = putItem, IsFirst = isFirst, GroupNo = 0 });
                            }
                            else
                            {
                                optionPairItems.Add(new OptionPairItem() { Call = callItem, Put = putItem, IsFirst = isFirst, GroupNo = groupNo });
                            }
                            optionItems[call] = callItem;
                            optionItems[inst] = putItem;
                            maturity = inst.Maturity;
                        }
                        call = null;
                    }
                    else
                    {
                        call = inst;
                    }
                }
            }
            dominantOptionPairItems.AddRange(optionPairItems);
            this.options = new ObservableCollection<OptionPairItem>(dominantOptionPairItems);

            this.QuoterCallBidSwitchCommand = new DelegateCommand<object>(new Action<object>(this.QuoterCallBidSwitchExecute));
            this.QuoterCallAskSwitchCommand = new DelegateCommand<object>(new Action<object>(this.QuoterCallAskSwitchExecute));
            this.QuoterPutBidSwitchCommand = new DelegateCommand<object>(new Action<object>(this.QuoterPutBidSwitchExecute));
            this.QuoterPutAskSwitchCommand = new DelegateCommand<object>(new Action<object>(this.QuoterPutAskSwitchExecute));

            this.DummyQuoterCallBidSwitchCommand = new DelegateCommand<object>(new Action<object>(this.DummyQuoterCallBidSwitchExecute));
            this.DummyQuoterCallAskSwitchCommand = new DelegateCommand<object>(new Action<object>(this.DummyQuoterCallAskSwitchExecute));
            this.DummyQuoterPutBidSwitchCommand = new DelegateCommand<object>(new Action<object>(this.DummyQuoterPutBidSwitchExecute));
            this.DummyQuoterPutAskSwitchCommand = new DelegateCommand<object>(new Action<object>(this.DummyQuoterPutAskSwitchExecute));

            this.HitterCallBidSwitchCommand = new DelegateCommand<object>(new Action<object>(this.HitterCallBidSwitchExecute));
            this.HitterCallAskSwitchCommand = new DelegateCommand<object>(new Action<object>(this.HitterCallAskSwitchExecute));
            this.HitterPutBidSwitchCommand = new DelegateCommand<object>(new Action<object>(this.HitterPutBidSwitchExecute));
            this.HitterPutAskSwitchCommand = new DelegateCommand<object>(new Action<object>(this.HitterPutAskSwitchExecute));

            this.DimerCallBidSwitchCommand = new DelegateCommand<object>(new Action<object>(this.DimerCallBidSwitchExecute));
            this.DimerCallAskSwitchCommand = new DelegateCommand<object>(new Action<object>(this.DimerCallAskSwitchExecute));
            this.DimerPutBidSwitchCommand = new DelegateCommand<object>(new Action<object>(this.DimerPutBidSwitchExecute));
            this.DimerPutAskSwitchCommand = new DelegateCommand<object>(new Action<object>(this.DimerPutAskSwitchExecute));

            this.CoverCallSwitchCommand = new DelegateCommand<object>(new Action<object>(this.CoverCallSwitchExecute));
            this.CoverPutSwitchCommand = new DelegateCommand<object>(new Action<object>(this.CoverPutSwitchExecute));

            this.EnquiryResponseCallSwitchCommand = new DelegateCommand<object>(new Action<object>(this.EnquiryResponseCallSwitchExecute));
            this.EnquiryResponsePutSwitchCommand = new DelegateCommand<object>(new Action<object>(this.EnquiryResponsePutSwitchExecute));
        }

        //public void RefreshInstrumentStatus(Proto.InstrumentStatusUpdate status)
        //{
        //    foreach (var kvp in this.underlyingItems)
        //    {
        //        kvp.Value.Status = status.Status;
        //    }
        //    foreach (var kvp in this.optionItems)
        //    {
        //        kvp.Value.Status = status.Status;
        //    }
        //}

        public void RefreshInstrumentStatus(Instrument inst)
        {
            if (inst.Type == Proto.InstrumentType.Option)
            {
                OptionItem item = null;
                if (this.optionItems.TryGetValue(inst as Option, out item))
                {
                    item.Status = inst.Status;
                }
            }
            else
            {
                UnderlyingItem item = null;
                if (this.underlyingItems.TryGetValue(inst, out item))
                {
                    item.Status = inst.Status;
                }
            }
        }

        public void RefreshDestriker(Option option, double destriker)
        {
            OptionItem item = null;
            if (this.optionItems.TryGetValue(option, out item))
            {
                item.Destriker = destriker;
            }
        }

        public void RefreshCredit(Proto.Credit credit)
        {
            switch (credit.Strategy)
            {
                case Proto.StrategyType.Quoter:
                    foreach (var c in credit.Records)
                    {
                        var option = productManager.FindId(c.Option) as Option;
                        OptionItem item = null;
                        if (option != null && this.optionItems.TryGetValue(option, out item))
                        {
                            item.QuoterCredit = c.Credit;
                        }
                    }
                    break;
                case Proto.StrategyType.Hitter:
                    foreach (var c in credit.Records)
                    {
                        var option = productManager.FindId(c.Option) as Option;
                        OptionItem item = null;
                        if (option != null && this.optionItems.TryGetValue(option, out item))
                        {
                            item.HitterCredit = c.Credit;
                        }
                    }
                    break;
                case Proto.StrategyType.Dimer:
                    foreach (var c in credit.Records)
                    {
                        var option = productManager.FindId(c.Option) as Option;
                        OptionItem item = null;
                        if (option != null && this.optionItems.TryGetValue(option, out item))
                        {
                            item.DimerCredit = c.Credit;
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        public void RefreshStrategySwitch(Option option, Proto.StrategySwitch sw)
        {
            OptionItem item = null;
            if (this.optionItems.TryGetValue(option, out item))
            {
                switch (sw.Strategy)
                {
                    case Proto.StrategyType.Quoter:
                        item.QuoterBidOn = sw.IsBid;
                        item.QuoterAskOn = sw.IsAsk;
                        item.EnquiryResponseOn = sw.IsQrCover;
                        break;
                    case Proto.StrategyType.DummyQuoter:
                        item.DummyQuoterBidOn = sw.IsBid;
                        item.DummyQuoterAskOn = sw.IsAsk;
                        break;
                    case Proto.StrategyType.Hitter:
                        item.HitterBidOn = sw.IsBid;
                        item.HitterAskOn = sw.IsAsk;
                        item.CoverOn = sw.IsQrCover;
                        break;
                    case Proto.StrategyType.Dimer:
                        item.DimerBidOn = sw.IsBid;
                        item.DimerAskOn = sw.IsAsk;
                        item.CoverOn = sw.IsQrCover;
                        break;
                    default:
                        break;
                }
            }
        }

        //public void RefreshQuoterRecord(IEnumerable<Proto.QuoterRecord> records)
        //{
        //    foreach(var r in records)
        //    {
        //        OptionItem item = null;
        //        Option option = this.productManager.FindId(r.Instrument) as Option;
        //        if (option != null && this.optionItems.TryGetValue(option, out item))
        //        {
        //            if (r.Credit > 0 || r.Multiplier > 0)
        //            {
        //                item.QuoterCredit = r.Credit;
        //            }
        //            else
        //            {
        //                item.QuoterBidOn = r.IsBid;
        //                item.QuoterAskOn = r.IsAsk;
        //                item.EnquiryResponseOn = r.IsQr;
        //            }
        //        }
        //    }
        //}

        //public delegate void ReceivePriceDelegate(Proto.Instrument inst, Proto.Price price);
        public void ReceivePrice(Instrument inst, Proto.Price price)
        {
            if (inst.Type == Proto.InstrumentType.Option)
            {
                OptionItem item = null;
                if (this.optionItems.TryGetValue(inst as Option, out item))
                {
                    ReceiveOptionPrice(item, price);
                }
            }
            else if (inst.HedgeUnderlying == inst)
            {
                foreach (var kvp in this.underlyingItems)
                {
                    if (kvp.Value.Underlying == inst)
                    {
                        ReceiveUnderlyingPrice(kvp.Value, price);
                    }

                    var ssr = ssrateManager.GetSSRate(inst.Id, kvp.Value.Maturity);
                    if (double.IsNaN(ssr) == false)
                    {
                        kvp.Value.Theo = price.AdjustedPrice + ssr;
                    }

                    int oldAtmIdx = -1, newAtmIdx = -1;
                    double minSpread = double.MaxValue;
                    for (int i = 0; i < this.options.Count; ++i)
                    {
                        if (this.options[i].Call.Option.Underlying == kvp.Value.Underlying)
                        {
                            double spread = Math.Abs(this.options[i].Call.Option.Strike - kvp.Value.Theo);
                            if (spread < minSpread)
                            {
                                minSpread = spread;
                                newAtmIdx = i;
                            }
                            if (this.options[i].IsAtmOption) oldAtmIdx = i;
                        }
                    }
                    if (oldAtmIdx != newAtmIdx)
                    {
                        if (oldAtmIdx != -1) this.options[oldAtmIdx].IsAtmOption = false;
                        if (newAtmIdx != -1) this.options[newAtmIdx].IsAtmOption = true;
                    }
                }
            }
            else
            {
                UnderlyingItem item = null;
                if (this.underlyingItems.TryGetValue(inst, out item))
                {
                    ReceiveUnderlyingPrice(item, price);
                }
            }
        }

        private void ReceiveOptionPrice(OptionItem item, Proto.Price price)
        {
            if (price.Bids.Count > 0)
            {
                item.MarketBidPrice = price.Bids[0].Price;
                item.MarketBidVolume = price.Bids[0].Volume;
            }
            else
            {
                item.MarketBidPrice = 0;
                item.MarketBidVolume = 0;
            }
            if (price.Asks.Count > 0)
            {
                item.MarketAskPrice = price.Asks[0].Price;
                item.MarketAskVolume = price.Asks[0].Volume;
            }
            else
            {
                item.MarketAskPrice = 0;
                item.MarketAskVolume = 0;
            }
            item.LastPrice = price.Last.Price;
            item.LastVolume = price.Last.Volume;
        }

        private void ReceiveUnderlyingPrice(UnderlyingItem item, Proto.Price price)
        {
            if (price.Bids.Count > 0)
            {
                item.MarketBidPrice = price.Bids[0].Price;
                item.MarketBidVolume = price.Bids[0].Volume;
            }
            else
            {
                item.MarketBidPrice = 0;
                item.MarketBidVolume = 0;
            }
            if (price.Asks.Count > 0)
            {
                item.MarketAskPrice = price.Asks[0].Price;
                item.MarketAskVolume = price.Asks[0].Volume;
            }
            else
            {
                item.MarketAskPrice = 0;
                item.MarketAskVolume = 0;
            }
            item.LastPrice = price.Last.Price;
            item.LastVolume = price.Last.Volume;
            item.PreClose = price.PreClose;
            item.PreSettlement = price.PreSettlement;
        }

        public void ReceiveOrder(Instrument inst, Proto.Order order)
        {
            if (inst.Type == Proto.InstrumentType.Option)
            {
                OptionItem item = null;
                if (this.optionItems.TryGetValue(inst as Option, out item))
                {
                    item.ReceiveOrder(order);
                }
            }
        }

        public void ReceivePosition(Instrument inst, Proto.Position position)
        {
            if (inst.Type == Proto.InstrumentType.Option)
            {
                OptionItem item = null;
                if (this.optionItems.TryGetValue(inst as Option, out item))
                {
                    ReceiveOptionPosition(item, position);
                }
            }
            else
            {
                UnderlyingItem item = null;
                if (this.underlyingItems.TryGetValue(inst, out item))
                {
                    item.Position = position.TotalLong - position.TotalShort;
                }
            }
        }

        private void ReceiveOptionPosition(OptionItem item, Proto.Position position)
        {
            UnderlyingItem underlyingItem = null;
            if (this.underlyingItems.TryGetValue(item.Option.Underlying, out underlyingItem))
            {
                int changedLong = position.TotalLong - item.LongPosition;
                int changedShort = position.TotalShort - item.ShortPosition;
                if (item.Option.OptionType == Proto.OptionType.Call)
                {
                    underlyingItem.BullPosition += changedLong;
                    underlyingItem.BearPosition += changedShort;
                }
                else
                {
                    underlyingItem.BullPosition += changedShort;
                    underlyingItem.BearPosition += changedLong;
                }
                underlyingItem.LongPosition += changedLong;
                underlyingItem.ShortPosition += changedShort;
            }

            item.Position = position.TotalLong - position.TotalShort;
            item.LongPosition = position.TotalLong;
            item.ShortPosition = position.TotalShort;
            item.AvailableLongPosition = position.LiquidLong;
            item.AvailableShortPosition = position.LiquidShort;
            item.ChangedPosition = item.Position - (position.YesterdayLong - position.YesterdayShort);
        }

        public void ReceiveGreeks(GreeksData greeks)
        {
            OptionItem item = null;
            if (this.optionItems.TryGetValue(greeks.Option, out item))
            {
                ReceiveGreeks(item, greeks);
            }
        }

        private void ReceiveGreeks(OptionItem item, GreeksData greeks)
        {
            item.Theo = greeks.Greeks.theo;
            item.Volatility = greeks.Greeks.vol;
            item.Delta = greeks.Greeks.delta;
            item.Gamma = greeks.Greeks.gamma;
            item.Vega = greeks.Greeks.vega;
            if (greeks.Option.OptionType == Proto.OptionType.Call)
            {
                item.SkewSensi = greeks.SkewSensi;
                item.ConvexSensi = greeks.CallConvexSensi * 1000;
            }
            else
            {
                item.ConvexSensi = greeks.PutConvexSensi * 1000;
            }
        }

        public void ReceiveIV(ImpliedVolatilityData data)
        {
            OptionItem item = null;
            if (this.optionItems.TryGetValue(data.Option, out item))
            {
                ReceiveIV(item, data);
            }
        }

        private void ReceiveIV(OptionItem item, ImpliedVolatilityData data)
        {
            item.ImpliedVol = data.LastIV;
            item.BidVol = data.BidIV;
            item.AskVol = data.AskIV;
        }

        public void SetStrategySwitch(HashSet<OptionItem>[] items)
        {
            Proto.StrategySwitchReq req = new Proto.StrategySwitchReq();
            foreach (var item in items[(int)Proto.StrategyType.Quoter])
            {
                req.Switches.Add(new Proto.StrategySwitch()
                    {
                        Strategy = Proto.StrategyType.Quoter,
                        Option = item.Option.Id,
                        IsBid = item.QuoterBidOn,
                        IsAsk = item.QuoterAskOn,
                        IsQrCover = item.EnquiryResponseOn,
                    });
            }
            foreach (var item in items[(int)Proto.StrategyType.Hitter])
            {
                req.Switches.Add(new Proto.StrategySwitch()
                {
                    Strategy = Proto.StrategyType.Hitter,
                    Option = item.Option.Id,
                    IsBid = item.HitterBidOn,
                    IsAsk = item.HitterAskOn,
                    IsQrCover = item.CoverOn,
                });
            }
            foreach (var item in items[(int)Proto.StrategyType.Dimer])
            {
                req.Switches.Add(new Proto.StrategySwitch()
                {
                    Strategy = Proto.StrategyType.Dimer,
                    Option = item.Option.Id,
                    IsBid = item.DimerBidOn,
                    IsAsk = item.DimerAskOn,
                    IsQrCover = item.CoverOn,
                });
            }
            foreach (var item in items[(int)Proto.StrategyType.DummyQuoter])
            {
                req.Switches.Add(new Proto.StrategySwitch()
                {
                    Strategy = Proto.StrategyType.DummyQuoter,
                    Option = item.Option.Id,
                    IsBid = item.DummyQuoterBidOn,
                    IsAsk = item.DummyQuoterAskOn,
                });
            }
            if (req.Switches.Count > 0)
            {
                req.Type = Proto.RequestType.Set;
                req.Exchange = this.exchange;
                req.User = serverService.User;
                serverService.Request(req);
            }
        }

        public void SetDestrikers(Proto.DestrikerReq req)
        {
            if (req.Destrikers.Count > 0)
            {
                req.Type = Proto.RequestType.Set;
                req.Exchange = this.exchange;
                req.User = serverService.User;
                serverService.Request(req);
            }
        }

        private void RequestStrategySwitch(Proto.StrategyType strategy, string option, bool isBid, bool isAsk, bool isQrCover)
        {
            Proto.StrategySwitchReq req = new Proto.StrategySwitchReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = this.exchange;
            req.User = serverService.User;

            req.Switches.Add(new Proto.StrategySwitch()
                {
                    Strategy = strategy,
                    Option = option,
                    IsBid = isBid,
                    IsAsk = isAsk,
                    IsQrCover = isQrCover,
                });
            serverService.Request(req);
        }

        private void RequestCoverStrategySwitch(OptionItem item)
        {
            Proto.StrategySwitchReq req = new Proto.StrategySwitchReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = this.exchange;
            req.User = serverService.User;

            req.Switches.Add(new Proto.StrategySwitch()
                {
                    Strategy = Proto.StrategyType.Hitter,
                    Option = item.Option.Id,
                    IsBid = item.HitterBidOn,
                    IsAsk = item.HitterAskOn,
                    IsQrCover = item.CoverOn,
                });
            req.Switches.Add(new Proto.StrategySwitch()
                {
                    Strategy = Proto.StrategyType.Dimer,
                    Option = item.Option.Id,
                    IsBid = item.DimerBidOn,
                    IsAsk = item.DimerAskOn,
                    IsQrCover = item.CoverOn,
                });
            serverService.Request(req);
        }

        private void QuoterCallBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.QuoterBidOn = !item.Call.QuoterBidOn;
            RequestStrategySwitch(Proto.StrategyType.Quoter, item.Call.Option.Id, item.Call.QuoterBidOn, item.Call.QuoterAskOn, item.Call.EnquiryResponseOn);
        }

        private void QuoterCallAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.QuoterAskOn = !item.Call.QuoterAskOn;
            RequestStrategySwitch(Proto.StrategyType.Quoter, item.Call.Option.Id, item.Call.QuoterBidOn, item.Call.QuoterAskOn, item.Call.EnquiryResponseOn);
        }

        private void QuoterPutBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.QuoterBidOn = !item.Put.QuoterBidOn;
            RequestStrategySwitch(Proto.StrategyType.Quoter, item.Put.Option.Id, item.Put.QuoterBidOn, item.Put.QuoterAskOn, item.Put.EnquiryResponseOn);
        }

        private void QuoterPutAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.QuoterAskOn = !item.Put.QuoterAskOn;
            RequestStrategySwitch(Proto.StrategyType.Quoter, item.Put.Option.Id, item.Put.QuoterBidOn, item.Put.QuoterAskOn, item.Put.EnquiryResponseOn);
        }

        private void DummyQuoterCallBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.DummyQuoterBidOn = !item.Call.DummyQuoterBidOn;
        }

        private void DummyQuoterCallAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.DummyQuoterAskOn = !item.Call.DummyQuoterAskOn;
        }

        private void DummyQuoterPutBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.DummyQuoterBidOn = !item.Put.DummyQuoterBidOn;
        }

        private void DummyQuoterPutAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.DummyQuoterAskOn = !item.Put.DummyQuoterAskOn;
        }

        private void HitterCallBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.HitterBidOn = !item.Call.HitterBidOn;
            RequestStrategySwitch(Proto.StrategyType.Hitter, item.Call.Option.Id, item.Call.HitterBidOn, item.Call.HitterAskOn, item.Call.CoverOn);
        }

        private void HitterCallAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.HitterAskOn = !item.Call.HitterAskOn;
            RequestStrategySwitch(Proto.StrategyType.Hitter, item.Call.Option.Id, item.Call.HitterBidOn, item.Call.HitterAskOn, item.Call.CoverOn);
        }

        private void HitterPutBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.HitterBidOn = !item.Put.HitterBidOn;
            RequestStrategySwitch(Proto.StrategyType.Hitter, item.Put.Option.Id, item.Put.HitterBidOn, item.Put.HitterAskOn, item.Put.CoverOn);
        }

        private void HitterPutAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.HitterAskOn = !item.Put.HitterAskOn;
            RequestStrategySwitch(Proto.StrategyType.Hitter, item.Put.Option.Id, item.Put.HitterBidOn, item.Put.HitterAskOn, item.Put.CoverOn);
        }

        private void DimerCallBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.DimerBidOn = !item.Call.DimerBidOn;
            RequestStrategySwitch(Proto.StrategyType.Dimer, item.Call.Option.Id, item.Call.DimerBidOn, item.Call.DimerAskOn, item.Call.CoverOn);
        }

        private void DimerCallAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.DimerAskOn = !item.Call.DimerAskOn;
            RequestStrategySwitch(Proto.StrategyType.Dimer, item.Call.Option.Id, item.Call.DimerBidOn, item.Call.DimerAskOn, item.Call.CoverOn);
        }

        private void DimerPutBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.DimerBidOn = !item.Put.DimerBidOn;
            RequestStrategySwitch(Proto.StrategyType.Dimer, item.Put.Option.Id, item.Put.DimerBidOn, item.Put.DimerAskOn, item.Put.CoverOn);
        }

        private void DimerPutAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.DimerAskOn = !item.Put.DimerAskOn;
            RequestStrategySwitch(Proto.StrategyType.Dimer, item.Put.Option.Id, item.Put.DimerBidOn, item.Put.DimerAskOn, item.Put.CoverOn);
        }

        private void CoverCallSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.CoverOn = !item.Call.CoverOn;
            RequestCoverStrategySwitch(item.Call);
        }

        private void CoverPutSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.CoverOn = !item.Put.CoverOn;
            RequestCoverStrategySwitch(item.Put);
        }

        private void EnquiryResponseCallSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.EnquiryResponseOn = !item.Call.EnquiryResponseOn;
            RequestStrategySwitch(Proto.StrategyType.Quoter, item.Call.Option.Id, item.Call.QuoterBidOn, item.Call.QuoterAskOn, item.Call.EnquiryResponseOn);
        }

        private void EnquiryResponsePutSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.EnquiryResponseOn = !item.Put.EnquiryResponseOn;
            RequestStrategySwitch(Proto.StrategyType.Quoter, item.Put.Option.Id, item.Put.QuoterBidOn, item.Put.QuoterAskOn, item.Put.EnquiryResponseOn);
        }

        private Dictionary<string, double> GetCredits(CreditManager cm, Proto.StrategyType strategy, string underlying, DateTime maturity)
        {
            var credits = new Dictionary<string, double>();
            var credit = cm.GetCredit(strategy, underlying, maturity);
            if (credit != null)
            {
                foreach (var c in credit.Records)
                {
                    credits[c.Option] = c.Credit;
                }
            }
            return credits;
        }

        private ProductManager productManager;
        private IUnityContainer container;
        private Proto.Exchange exchange;
        private SSRateManager ssrateManager;
        private ServerService serverService;
    }

    class UnderlyingItem : BindableBase
    {
        public Instrument Underlying { get; set; }
        public DateTime Maturity { get; set; }

        private Proto.InstrumentStatus status;
        public Proto.InstrumentStatus Status
        {
            get { return status; }
            set { SetProperty(ref status, value); }
        }

        private double marketBidPrice;
        public double MarketBidPrice
        {
            get { return marketBidPrice; }
            set { SetProperty(ref marketBidPrice, value); }
        }

        private int marketBidVolume;
        public int MarketBidVolume
        {
            get { return marketBidVolume; }
            set { SetProperty(ref marketBidVolume, value); }
        }

        private double marketAskPrice;
        public double MarketAskPrice
        {
            get { return marketAskPrice; }
            set { SetProperty(ref marketAskPrice, value); }
        }

        private int marketAskVolume;
        public int MarketAskVolume
        {
            get { return marketAskVolume; }
            set { SetProperty(ref marketAskVolume, value); }
        }

        private double theo;
        public double Theo
        {
            get { return theo; }
            set { SetProperty(ref theo, value); }
        }

        private double lastPrice;
        public double LastPrice
        {
            get { return lastPrice; }
            set { SetProperty(ref lastPrice, value); }
        }

        private int lastVolume;
        public int LastVolume
        {
            get { return lastVolume; }
            set { SetProperty(ref lastVolume, value); }
        }

        private double preClose;
        public double PreClose
        {
            get { return preClose; }
            set { SetProperty(ref preClose, value); }
        }

        private double preSettlement;
        public double PreSettlement
        {
            get { return preSettlement; }
            set { SetProperty(ref preSettlement, value); }
        }
        
        private int position;
        public int Position
        {
            get { return position; }
            set { SetProperty(ref position, value); }
        }

        private int bullPosition;
        public int BullPosition
        {
            get { return bullPosition; }
            set { SetProperty(ref bullPosition, value); }
        }

        private int bullLimit;
        public int BullLimit
        {
            get { return bullLimit; }
            set { SetProperty(ref bullLimit, value); }
        }        

        private int bearPosition;
        public int BearPosition
        {
            get { return bearPosition; }
            set { SetProperty(ref bearPosition, value); }
        }

        private int bearLimit;
        public int BearLimit
        {
            get { return bearLimit; }
            set { SetProperty(ref bearLimit, value); }
        }        

        private int longPosition;
        public int LongPosition
        {
            get { return longPosition; }
            set { SetProperty(ref longPosition, value); }
        }

        private int longLimit;
        public int LongLimit
        {
            get { return longLimit; }
            set { SetProperty(ref longLimit, value); }
        }        

        private int shortPosition;
        public int ShortPosition
        {
            get { return shortPosition; }
            set { SetProperty(ref shortPosition, value); }
        }

        private int shortLimit;
        public int ShortLimit
        {
            get { return shortLimit; }
            set { SetProperty(ref shortLimit, value); }
        }        
    }

    public class OptionPairItem : BindableBase
    {
        public OptionItem Call { get; set; }
        public OptionItem Put { get; set; }

        private bool isAtmOption;
        public bool IsAtmOption
        {
            get { return isAtmOption; }
            set { SetProperty(ref isAtmOption, value); }
        }        

        public bool IsFirst { get; set; }
        public int GroupNo { get; set; }
    }

    public class OptionItem : BindableBase
    {
        public void ReceiveOrder(Proto.Order order)
        {
            if (OrderManager.IsInactive(order))
            {
                if (OrderManager.IsBid(order.Side))
                {
                    for (int i = 0; i < Bids.Count && order.Price <= Bids[i].Price; ++i)
                    {
                        if (order.Id == Bids[i].Id)
                        {
                            Bids.RemoveAt(i);
                            if (i == 0)
                            {
                                BestBidPrice = (i < Bids.Count) ? Bids[i].Price : 0;
                            }

                            if (BidOrderID == order.ExchangeId)
                            {
                                for (int j = i; j < Bids.Count && order.Price <= Bids[j].Price; ++j)
                                {
                                    if (OrderManager.IsQuoter(Bids[j]))
                                    {
                                        BidOrderID = Bids[j].ExchangeId;
                                        BidPrice = Bids[j].Price;
                                        BidVolume = Bids[j].Volume - Bids[j].ExecutedVolume;
                                        return;
                                    }
                                }
                                BidOrderID = null;
                                BidPrice = 0;
                                BidVolume = 0;
                            }
                            return;
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < Asks.Count && order.Price >= Asks[i].Price; ++i)
                    {
                        if (order.Id == Asks[i].Id)
                        {
                            Asks.RemoveAt(i);
                            if (i == 0)
                            {
                                BestAskPrice = (i < Asks.Count) ? Asks[i].Price : 0;
                            }

                            if (AskOrderID == order.ExchangeId)
                            {
                                for (int j = i; j < Asks.Count && order.Price >= Asks[j].Price; ++j)
                                {
                                    if (OrderManager.IsQuoter(Asks[j]))
                                    {
                                        AskOrderID = Asks[j].ExchangeId;
                                        AskPrice = Asks[j].Price;
                                        AskVolume = Asks[j].Volume - Asks[j].ExecutedVolume;
                                        return;
                                    }
                                }
                                AskOrderID = null;
                                AskPrice = 0;
                                AskVolume = 0;
                            }
                            return;
                        }
                    }
                }
            }
            else if (order.Status > Proto.OrderStatus.Submitted)
            {
                if (OrderManager.IsBid(order.Side))
                {
                    for (int i = 0; i < Bids.Count; ++i)
                    {
                        if (order.Id == Bids[i].Id)
                        {
                            Bids[i] = order;
                            if (BidOrderID == order.ExchangeId)
                            {
                                BidVolume = order.Volume - order.ExecutedVolume;
                            }
                            return;
                        }
                        else if (order.Price >= Bids[i].Price)
                        {
                            Bids.Insert(i, order);
                            if (i == 0)
                            {
                                BestBidPrice = order.Price;
                            }
                            if (BidVolume == 0 && OrderManager.IsQuoter(order))
                            {
                                BidOrderID = order.ExchangeId;
                                BidPrice = order.Price;
                                BidVolume = order.Volume - order.ExecutedVolume;
                            }
                            return;
                        }
                    }
                    Bids.Add(order);
                    if (Bids.Count == 1)
                    {
                        BestBidPrice = order.Price;
                    }
                    if (BidVolume == 0 && OrderManager.IsQuoter(order))
                    {
                        BidOrderID = order.ExchangeId;
                        BidPrice = order.Price;
                        BidVolume = order.Volume - order.ExecutedVolume;
                    }
                }
                else
                {
                    for (int i = 0; i < Asks.Count; ++i)
                    {
                        if (order.Id == Asks[i].Id)
                        {
                            Asks[i] = order;
                            if (AskOrderID == order.ExchangeId)
                            {
                                AskVolume = order.Volume - order.ExecutedVolume;
                            }
                            return;
                        }
                        else if (order.Price <= Asks[i].Price)
                        {
                            Asks.Insert(i, order);
                            if (i == 0)
                            {
                                BestAskPrice = order.Price;
                            }
                            if (AskVolume == 0 && OrderManager.IsQuoter(order))
                            {
                                AskOrderID = order.ExchangeId;
                                AskPrice = order.Price;
                                AskVolume = order.Volume - order.ExecutedVolume;
                            }
                            return;
                        }
                    }
                    Asks.Add(order);
                    if (Asks.Count == 1)
                    {
                        BestAskPrice = order.Price;
                    }
                    if (AskVolume == 0 && OrderManager.IsQuoter(order))
                    {
                        AskOrderID = order.ExchangeId;
                        AskPrice = order.Price;
                        AskVolume = order.Volume - order.ExecutedVolume;
                    }
                }
            }
        }

        public Option Option { get; set; }

        private Proto.InstrumentStatus status;
        public Proto.InstrumentStatus Status
        {
            get { return status; }
            set { SetProperty(ref status, value); }
        }

        private double quoterCredit = double.NaN;
        public double QuoterCredit
        {
            get { return quoterCredit; }
            set { SetProperty(ref quoterCredit, value); }
        }        

        private double hitterCredit = double.NaN;
        public double HitterCredit
        {
            get { return hitterCredit; }
            set { SetProperty(ref hitterCredit, value); }
        }

        private double dimerCredit = double.NaN;
        public double DimerCredit
        {
            get { return dimerCredit; }
            set { SetProperty(ref dimerCredit, value); }
        }
        
        private bool quoterBidOn;
        public bool QuoterBidOn
        {
            get { return quoterBidOn; }
            set { SetProperty(ref quoterBidOn, value); }
        }

        private bool quoterAskOn;
        public bool QuoterAskOn
        {
            get { return quoterAskOn; }
            set { SetProperty(ref quoterAskOn, value); }
        }

        private bool dummyQuoterBidOn;
        public bool DummyQuoterBidOn
        {
            get { return dummyQuoterBidOn; }
            set { SetProperty(ref dummyQuoterBidOn, value); }
        }

        private bool dummyQuoterAskOn;
        public bool DummyQuoterAskOn
        {
            get { return dummyQuoterAskOn; }
            set { SetProperty(ref dummyQuoterAskOn, value); }
        }

        private bool hitterBidOn;
        public bool HitterBidOn
        {
            get { return hitterBidOn; }
            set { SetProperty(ref hitterBidOn, value); }
        }

        private bool hitterAskOn;
        public bool HitterAskOn
        {
            get { return hitterAskOn; }
            set { SetProperty(ref hitterAskOn, value); }
        }

        private bool dimerBidOn;
        public bool DimerBidOn
        {
            get { return dimerBidOn; }
            set { SetProperty(ref dimerBidOn, value); }
        }

        private bool dimerAskOn;
        public bool DimerAskOn
        {
            get { return dimerAskOn; }
            set { SetProperty(ref dimerAskOn, value); }
        }

        private bool coverOn;
        public bool CoverOn
        {
            get { return coverOn; }
            set { SetProperty(ref coverOn, value); }
        }

        private bool enquiryResponseOn;
        public bool EnquiryResponseOn
        {
            get { return enquiryResponseOn; }
            set { SetProperty(ref enquiryResponseOn, value); }
        }

        private double bestBidPrice;
        public double BestBidPrice
        {
            get { return bestBidPrice; }
            set { SetProperty(ref bestBidPrice, value); }
        }        

        private double bidPrice;
        public double BidPrice
        {
            get { return bidPrice; }
            set { SetProperty(ref bidPrice, value); }
        }        

        private int bidVolume;
        public int BidVolume
        {
            get { return bidVolume; }
            set { SetProperty(ref bidVolume, value); }
        }

        private double marketBidPrice;
        public double MarketBidPrice
        {
            get { return marketBidPrice; }
            set { SetProperty(ref marketBidPrice, value); }
        }

        private int marketBidVolume;
        public int MarketBidVolume
        {
            get { return marketBidVolume; }
            set { SetProperty(ref marketBidVolume, value); }
        }

        private double bestAskPrice;
        public double BestAskPrice
        {
            get { return bestAskPrice; }
            set { SetProperty(ref bestAskPrice, value); }
        }        

        private double askPrice;
        public double AskPrice
        {
            get { return askPrice; }
            set { SetProperty(ref askPrice, value); }
        }

        private int askVolume;
        public int AskVolume
        {
            get { return askVolume; }
            set { SetProperty(ref askVolume, value); }
        }

        private bool isMeetObligation = true;
        public bool IsMeetObligation
        {
            get { return isMeetObligation = true; }
            set { SetProperty(ref isMeetObligation, value); }
        }        

        private double marketAskPrice;
        public double MarketAskPrice
        {
            get { return marketAskPrice; }
            set { SetProperty(ref marketAskPrice, value); }
        }

        private int marketAskVolume;
        public int MarketAskVolume
        {
            get { return marketAskVolume; }
            set { SetProperty(ref marketAskVolume, value); }
        }

        private double theo = double.NaN;
        public double Theo
        {
            get { return theo; }
            set { SetProperty(ref theo, value); }
        }

        private double destriker;
        public double Destriker
        {
            get { return destriker; }
            set { SetProperty(ref destriker, value); }
        }

        private string bidOrderID;
        public string BidOrderID
        {
            get { return bidOrderID; }
            set { SetProperty(ref bidOrderID, value); }
        }

        private string askOrderID;
        public string AskOrderID
        {
            get { return askOrderID; }
            set { SetProperty(ref askOrderID, value); }
        }

        private double lastPrice;
        public double LastPrice
        {
            get { return lastPrice; }
            set { SetProperty(ref lastPrice, value); }
        }

        private int lastVolume;
        public int LastVolume
        {
            get { return lastVolume; }
            set { SetProperty(ref lastVolume, value); }
        }        

        private int position;
        public int Position
        {
            get { return position; }
            set { SetProperty(ref position, value); }
        }

        private int longPosition;
        public int LongPosition
        {
            get { return longPosition; }
            set { SetProperty(ref longPosition, value); }
        }

        private int availableLongPosition;
        public int AvailableLongPosition
        {
            get { return availableLongPosition; }
            set { SetProperty(ref availableLongPosition, value); }
        }        

        private int shortPosition;
        public int ShortPosition
        {
            get { return shortPosition; }
            set { SetProperty(ref shortPosition, value); }
        }

        private int availableShortPosition;
        public int AvailableShortPosition
        {
            get { return availableShortPosition; }
            set { SetProperty(ref availableShortPosition, value); }
        }        

        private int changedPosition;
        public int ChangedPosition
        {
            get { return changedPosition; }
            set { SetProperty(ref changedPosition, value); }
        }

        private double delta = double.NaN;
        public double Delta
        {
            get { return delta; }
            set { SetProperty(ref delta, value); }
        }

        private double gamma = double.NaN;
        public double Gamma
        {
            get { return gamma; }
            set { SetProperty(ref gamma, value); }
        }

        private double vega = double.NaN;
        public double Vega
        {
            get { return vega; }
            set { SetProperty(ref vega, value); }
        }

        private double impliedVol;
        public double ImpliedVol
        {
            get { return impliedVol; }
            set { SetProperty(ref impliedVol, value); }
        }

        private double bidVol;
        public double BidVol
        {
            get { return bidVol; }
            set { SetProperty(ref bidVol, value); }
        }

        private double askVol;
        public double AskVol
        {
            get { return askVol; }
            set { SetProperty(ref askVol, value); }
        }

        private double volatility = double.NaN;
        public double Volatility
        {
            get { return volatility; }
            set { SetProperty(ref volatility, value); }
        }

        private double skewSensi = double.NaN;
        public double SkewSensi
        {
            get { return skewSensi; }
            set { SetProperty(ref skewSensi, value); }
        }

        private double convexSensi = double.NaN;
        public double ConvexSensi
        {
            get { return convexSensi; }
            set { SetProperty(ref convexSensi, value); }
        }

        private List<Proto.Order> bids;
        public List<Proto.Order> Bids
        {
            get { return bids; }
            set
            {
                bids = value;
                if (value != null && value.Count > 0)
                {
                    BestBidPrice = value[0].Price;
                    foreach (var ord in value)
                    {
                        if (OrderManager.IsQuoter(ord) && ord.Status > Proto.OrderStatus.Submitted)
                        {
                            BidOrderID = ord.ExchangeId;
                            BidPrice = ord.Price;
                            BidVolume = ord.Volume - ord.ExecutedVolume;
                            break;
                        }
                    }
                }
            }
        }


        private List<Proto.Order> asks;
        public List<Proto.Order> Asks
        {
            get { return asks; }
            set
            {
                asks = value;
                if (value != null && value.Count > 0)
                {
                    BestAskPrice = value[0].Price;
                    foreach (var ord in value)
                    {
                        if (OrderManager.IsQuoter(ord) && ord.Status > Proto.OrderStatus.Submitted)
                        {
                            AskOrderID = ord.ExchangeId;
                            AskPrice = ord.Price;
                            AskVolume = ord.Volume - ord.ExecutedVolume;
                            break;
                        }
                    }
                }
            }
        }
                
        //public OptionItem()
        //{
        //    string symbol = this.CallOption.Symbol;
        //}
    }
}
