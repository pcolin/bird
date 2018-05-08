using client.Models;
using Microsoft.Practices.Unity;
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
                return Proto.Exchange.De;
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


        private Proto.Instrument hedgeUnderlying;
        public Proto.Instrument HedgeUnderlying
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

        private Dictionary<string, UnderlyingItem> underlyingItems;
        private ICollectionView underlyings;
        public ICollectionView Underlyings
        {
            get { return underlyings; }
            set { SetProperty(ref underlyings, value); }
        }

        private Dictionary<string, OptionItem> optionItems;
        private ObservableCollection<OptionPairItem> options;
        public ObservableCollection<OptionPairItem> Options
        {
            get { return options; }
            set { SetProperty(ref options, value); }
        }

        //public OptionUserControlViewModel()
        //{
        //    List<UnderlyingItem> underlyingItems = new List<UnderlyingItem>();
        //    this.underlyings = new ListCollectionView(underlyingItems);
        //    List<OptionPairItem> optionItems = new List<OptionPairItem>();
        //    this.options = new ListCollectionView(optionItems);

        //}

        public OptionUserControlViewModel(string hedgeUnderlying, ProductManager products, IUnityContainer container)
        {
            var positions = container.Resolve<PositionManager>();

            List<UnderlyingItem> items = new List<UnderlyingItem>();
            underlyingItems = new Dictionary<string, UnderlyingItem>();
            var underlyings = products.GetUnderlyings(hedgeUnderlying);
            underlyings.Sort((x, y) => x.Symbol.CompareTo(y.Symbol));
            foreach (var inst in underlyings)
            {                
                var item = new UnderlyingItem() { Underlying = inst, Position = positions.GetPosition(inst.Id)};
                if (inst.Underlying == hedgeUnderlying)
                {
                    this.HedgeUnderlying = inst;
                    items.Insert(0, item);
                }
                else
                {
                    items.Add(item);
                }

                underlyingItems[inst.Id] = item;
            }
            this.underlyings = new ListCollectionView(items);

            List<OptionPairItem> optionPairItems = new List<OptionPairItem>();
            List<OptionPairItem> dominantOptionPairItems = new List<OptionPairItem>();
            this.optionItems = new Dictionary<string, OptionItem>();
            //List<OptionItem> optionItems = new List<OptionItem>();
            var options = products.GetOptionsByHedgeUnderlying(hedgeUnderlying);
            if (options != null)
            {
                var ret = options.OrderBy(x => x.Underlying).ThenBy(x => x.Strike).ThenBy(x => x.CallPut);
                Proto.Instrument call = null;
                string maturity = null;
                int groupNo = 0;
                foreach (var inst in ret)
                {
                    if (inst.CallPut == Proto.OptionType.Put)
                    {
                        if (call != null && call.Underlying == inst.Underlying && call.Strike == inst.Strike)
                        {
                            bool isFirst = false;
                            if (inst.Maturity != maturity && inst.Underlying != hedgeUnderlying)
                            {
                                ++groupNo;
                                isFirst = true;
                            }
                            var callItem = new OptionItem() { Option = call };
                            Proto.Position p = null;
                            if (positions.GetPosition(call.Id, out p))
                            {
                                callItem.Position = p.TotalLong - p.TotalShort;
                                callItem.LongPosition = p.TotalLong;
                                callItem.ShortPosition = p.TotalShort;
                                callItem.AvailableLongPosition = p.LiquidLong;
                                callItem.AvailableShortPosition = p.LiquidShort;
                            }
                            var putItem = new OptionItem() { Option = inst };
                            if (positions.GetPosition(inst.Id, out p))
                            {
                                putItem.Position = p.TotalLong - p.TotalShort;
                                putItem.LongPosition = p.TotalLong;
                                putItem.ShortPosition = p.TotalShort;
                                putItem.AvailableLongPosition = p.LiquidLong;
                                putItem.AvailableShortPosition = p.LiquidShort;
                                putItem.ChangedPosition = putItem.Position - (p.YesterdayLong - p.YesterdayShort);
                            }
                            if (inst.Underlying == hedgeUnderlying)
                            {
                                dominantOptionPairItems.Add(new OptionPairItem() { Call = callItem, Put = putItem, IsFirst = isFirst, GroupNo = 0 });
                            }
                            else
                            {
                                optionPairItems.Add(new OptionPairItem() { Call = callItem, Put = putItem, IsFirst = isFirst, GroupNo = groupNo });
                            }
                            optionItems[call.Id] = callItem;
                            optionItems[inst.Id] = putItem;
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

        //public delegate void ReceivePriceDelegate(Proto.Instrument inst, Proto.Price price);
        public void ReceivePrice(Proto.Instrument inst, Proto.Price price)
        {
            if (inst.Type == Proto.InstrumentType.Option)
            {
                OptionItem item = null;
                if (this.optionItems.TryGetValue(inst.Id, out item))
                {
                    if (price.Bids.Count > 0)
                    {
                        item.MarketBidPrice = price.Bids[0].Price;
                        item.MarketBidVolume = price.Bids[0].Volume;
                    }
                    if (price.Asks.Count > 0)
                    {
                        item.MarketAskPrice = price.Asks[0].Price;
                        item.MarketAskVolume = price.Asks[0].Volume;
                    }
                    item.LastPrice = price.Last.Price;
                    item.LastVolume = price.Last.Volume;
                }
            }
            else
            {
                UnderlyingItem item = null;
                if (this.underlyingItems.TryGetValue(inst.Id, out item))
                {
                    if (price.Bids.Count > 0)
                    {
                        item.MarketBidPrice = price.Bids[0].Price;
                        item.MarketBidVolume = price.Bids[0].Volume;
                    }
                    if (price.Asks.Count > 0)
                    {
                        item.MarketAskPrice = price.Asks[0].Price;
                        item.MarketAskVolume = price.Asks[0].Volume;
                    }
                    item.LastPrice = price.Last.Price;
                    item.LastVolume = price.Last.Volume;
                    item.PreClose = price.PreClose;
                    item.PreSettlement = price.PreSettlement;
                    //if (price.Bids.Count > 0 && price.Asks.Count > 0)
                    {
                        item.Theo = price.AdjustedPrice;
                        int oldAtmIdx = -1, newAtmIdx = -1;
                        double minSpread = double.MaxValue;
                        for (int i = 0; i < this.options.Count; ++i)
                        {
                            if (this.options[i].Call.Option.Underlying == inst.Id)
                            {
                                double spread = Math.Abs(this.options[i].Call.Option.Strike - item.Theo);
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
            }
        }

        private void QuoterCallBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.QuoterBidOn = !item.Call.QuoterBidOn;
        }

        private void QuoterCallAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.QuoterAskOn = !item.Call.QuoterAskOn;
        }

        private void QuoterPutBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.QuoterBidOn = !item.Put.QuoterBidOn;
        }

        private void QuoterPutAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.QuoterAskOn = !item.Put.QuoterAskOn;
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
        }

        private void HitterCallAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.HitterAskOn = !item.Call.HitterAskOn;
        }

        private void HitterPutBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.HitterBidOn = !item.Put.HitterBidOn;
        }

        private void HitterPutAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.HitterAskOn = !item.Put.HitterAskOn;
        }

        private void DimerCallBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.DimerBidOn = !item.Call.DimerBidOn;
        }

        private void DimerCallAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.DimerAskOn = !item.Call.DimerAskOn;
        }

        private void DimerPutBidSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.DimerBidOn = !item.Put.DimerBidOn;
        }

        private void DimerPutAskSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.DimerAskOn = !item.Put.DimerAskOn;
        }

        private void CoverCallSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.CoverOn = !item.Call.CoverOn;
        }

        private void CoverPutSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.CoverOn = !item.Put.CoverOn;
        }

        private void EnquiryResponseCallSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Call.EnquiryResponseOn = !item.Call.EnquiryResponseOn;
        }

        private void EnquiryResponsePutSwitchExecute(object obj)
        {
            DataGridCellInfo cell = (DataGridCellInfo)obj;
            OptionPairItem item = cell.Item as OptionPairItem;
            item.Put.EnquiryResponseOn = !item.Put.EnquiryResponseOn;
        }
    }

    class UnderlyingItem : BindableBase
    {
        public Proto.Instrument Underlying { get; set; }
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

        private int lastVolume = 123456789;
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
    }

    public class OptionPairItem : BindableBase
    {
        public OptionItem Call { get; set; }
        public OptionItem Put { get; set; }

        private double skewSense;
        public double SkewSense
        {
            get { return skewSense; }
            set { SetProperty(ref skewSense, value); }
        }

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
        public Proto.Instrument Option { get; set; }

        private Proto.InstrumentStatus status;
        public Proto.InstrumentStatus Status
        {
            get { return status; }
            set { SetProperty(ref status, value); }
        }

        private double quoterCredit;
        public double QuoterCredit
        {
            get { return quoterCredit; }
            set { SetProperty(ref quoterCredit, value); }
        }

        private string quoterCreditFormat;
        public string QuoterCreditFormat
        {
            get { return quoterCreditFormat; }
            set { SetProperty(ref quoterCreditFormat, value); }
        }        

        private double hitterCredit;
        public double HitterCredit
        {
            get { return hitterCredit; }
            set { SetProperty(ref hitterCredit, value); }
        }

        private double dimerCredit;
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

        private double theo;
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

        private double delta;
        public double Delta
        {
            get { return delta; }
            set { SetProperty(ref delta, value); }
        }

        private double gamma;
        public double Gamma
        {
            get { return gamma; }
            set { SetProperty(ref gamma, value); }
        }

        private double vega;
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

        private double volatility;
        public double Volatility
        {
            get { return volatility; }
            set { SetProperty(ref volatility, value); }
        }

        private double convex;
        public double Convex
        {
            get { return convex; }
            set { SetProperty(ref convex, value); }
        }        
        
        //public OptionItem()
        //{
        //    string symbol = this.CallOption.Symbol;
        //}
    }
}
