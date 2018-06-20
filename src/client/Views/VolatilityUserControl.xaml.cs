using client.Models;
using client.ViewModels;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.Practices.Unity;
using ModelLibrary;
using Xceed.Wpf.Toolkit;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for VolatilityUserControl.xaml
    /// </summary>
    public partial class VolatilityUserControl : UserControl
    {
        public VolatilityUserControl(VolatilityUserControlViewModel viewModel)
        {
            Formats = new Dictionary<int, string>();
            this.DataContext = viewModel;
            InitializeComponent();
        }

        public void SaveLayout()
        {
            var vm = this.DataContext as VolatilityUserControlViewModel;
            string name = "Volatility_" + vm.Exchange + ".xml";
            MainWindow.SaveDataGridLayout(name, this.VolatilityDataGrid, Formats);
        }

        public Dictionary<int, string> Formats { get; set; }
        private DoubleUpDown volDoubleUpDown;

        private void DrawButton_Click(object sender, RoutedEventArgs e)
        {
            VolatilityUserControlViewModel vm = this.DataContext as VolatilityUserControlViewModel;

            var item = vm.Volatilities[vm.SelectedVolatility];

            VolatilityParameterWrapper param = null;
            double rate = 0, timeValue = 0;
            if (item.Modified)
            {
                param = new VolatilityParameterWrapper()
                    {
                        spot = item.SpotRef,
                        skew = item.Skew,
                        atm_vol = item.AtmVol,
                        call_convex = item.CallConvex,
                        put_convex = item.PutConvex,
                        call_slope = item.CallSlope,
                        put_slope = item.PutSlope,
                        call_cutoff = item.CallCutoff,
                        put_cutoff = item.PutCutoff,
                        vcr = item.VCR,
                        scr = item.SCR,
                        ccr = item.CCR,
                        spcr = item.SPCR,
                        sccr = item.SCCR
                    };
                var rm = vm.Container.Resolve<InterestRateManager>(vm.Exchange.ToString());
                if (rm != null)
                {
                    rate = rm.GetInterestRate(item.Maturity);
                }

                //var sm = vm.Container.Resolve<SSRateManager>(vm.Exchange.ToString());
                //if (sm != null)
                //{
                //    ssrate = sm.GetSSRate(item.Underlying.Id, item.Maturity).Value;
                //}

                var em = vm.Container.Resolve<ExchangeParameterManager>(vm.Exchange.ToString());
                if (em != null)
                {
                    timeValue = em.GetTimeValue(item.Maturity);
                }
            }

            this.UnderlyingMaturity.Text = item.Underlying.Id + "@" + item.Maturity.ToString("yyyyMMdd");

            var pm = vm.Container.Resolve<ProductManager>(vm.Exchange.ToString());
            var options = pm.GetOptionsByHedgeUnderlying(item.Underlying, item.Maturity);
            var calculator = vm.Container.Resolve<TheoCalculator>(vm.Exchange.ToString());

            var strikes = new List<double>();
            var oldIV = new List<double>();
            var newIV = new List<double>();
            var callBidStrikes = new List<double>();
            var callBidIV = new List<double>();
            var callAskStrikes = new List<double>();
            var callAskIV = new List<double>();
            var putBidStrikes = new List<double>();
            var putBidIV = new List<double>();
            var putAskStrikes = new List<double>();
            var putAskIV = new List<double>();
            foreach (var op in options)
            {
                if (op.OptionType == Proto.OptionType.Call)
                {
                    strikes.Add(op.Strike);
                    ImpliedVolatilityData iv = calculator.GetIV(op);
                    if (iv != null)
                    {
                        if (iv.BidIV > 0.01)
                        {
                            callBidStrikes.Add(op.Strike);
                            callBidIV.Add(iv.BidIV * 100);
                        }
                        if (iv.AskIV > 0.01)
                        {
                            callAskStrikes.Add(op.Strike);
                            callAskIV.Add(iv.AskIV * 100);
                        }
                    }
                    //else
                    //{
                    //    callBidIV.Add(0);
                    //    callAskIV.Add(0);
                    //}
                    
                    GreeksData greeks = calculator.GetGreeks(op);
                    oldIV.Add((greeks != null) ? greeks.Greeks.vol * 100 : 0);

                    if (item.Modified)
                    {
                        newIV.Add(model.Calculate(param, op.HedgeUnderlying.Type == Proto.InstrumentType.Future, item.Spot, op.Strike, rate, item.SSRate, timeValue) * 100);
                    }
                }
                else
                {
                    ImpliedVolatilityData iv = calculator.GetIV(op);
                    if (iv != null)
                    {
                        if (iv.BidIV > 0.01)
                        {
                            putBidStrikes.Add(op.Strike);
                            putBidIV.Add(iv.BidIV * 100);
                        }
                        if (iv.AskIV > 0.01)
                        {
                            putAskStrikes.Add(op.Strike);
                            putAskIV.Add(iv.AskIV * 100);
                        }
                    }
                    //else
                    //{
                    //    putBidIV.Add(0);
                    //    putAskIV.Add(0);
                    //}
                }
            }
            
            this.CallBidIVGraph.Plot(callBidStrikes, callBidIV);
            this.CallAskIVGraph.Plot(callAskStrikes, callAskIV);
            this.PutBidIVGraph.Plot(putBidStrikes, putBidIV);
            this.PutAskIVGraph.Plot(putAskStrikes, putAskIV);

            this.OldVolGraph.Plot(strikes, oldIV);
            if (item.Modified)
            {
                this.NewVolGraph.Visibility = System.Windows.Visibility.Visible;
                this.NewVolGraph.Plot(strikes, newIV);
                this.VolGraph.Plot(strikes, newIV);
            }
            else
            {
                this.NewVolGraph.Visibility = System.Windows.Visibility.Collapsed;
                this.VolGraph.Plot(strikes, oldIV);
            }
        }

        private VolatilityModelWrapper model = new VolatilityModelWrapper();

        private void UserControl_Initialized(object sender, EventArgs e)
        {
            var vm = this.DataContext as VolatilityUserControlViewModel;
            if (vm != null)
            {
                if (vm.Volatilities[vm.SelectedVolatility].Underlying.Type == Proto.InstrumentType.Future)
                {
                    this.SSRateColumn.Header = "Basis";
                }

                /// load layout
                string name = "Volatility_" + vm.Exchange + ".xml";
                MainWindow.LoadDataGridLayout(name, this.VolatilityDataGrid, this.Formats);
                //this.volDoubleUpDown.FormatString = this.Formats[this.AtmVolColumn.DisplayIndex];
            }
        }

        private void MenuItem_Click(object sender, RoutedEventArgs e)
        {
            ColumnSettingWindow.ShowColumnSettingWindow(this, this.VolatilityDataGrid, this.Formats);
            this.volDoubleUpDown.FormatString = this.Formats[this.AtmVolColumn.DisplayIndex];
        }

        private void DoubleUpDown_Initialized(object sender, EventArgs e)
        {
            this.volDoubleUpDown = sender as DoubleUpDown;
        }
    }

    public class BoolToStringConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            bool b = (bool)value;
            return b ? "*" : string.Empty;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class VolatilityPercentDoubleFormatConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            double result = (double)value;
            return result * 100;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            double result = (double)value;
            return result / 100;
        }
    }


    //public class DoubleFormatMultiConverter : IMultiValueConverter
    //{
    //    public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        if (values[0] is double)
    //        {
    //            double value = (double)values[0];
    //            Dictionary<int, string> formats = values[2] as Dictionary<int, string>;
    //            if (formats != null)
    //            {
    //                int index = (int)values[1];
    //                string format = null;
    //                if (formats.TryGetValue(index, out format))
    //                {
    //                    return value.ToString(format);
    //                }
    //            }
    //            return value.ToString("N2");
    //        }
    //        return null;
    //    }

    //    public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        double result = 0;
    //        double.TryParse(value.ToString(), out result);
    //        return new object[3] { result, null, null };
    //    }
    //}

    //public class PercentDoubleFormatMultiConverter : IMultiValueConverter
    //{
    //    public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        if (values[0] is double)
    //        {
    //            double value = (double)values[0];
    //            Dictionary<int, string> formats = values[2] as Dictionary<int, string>;
    //            if (formats != null)
    //            {
    //                int index = (int)values[1];
    //                string format = null;
    //                if (formats.TryGetValue(index, out format))
    //                {
    //                    return (value * 100).ToString(format);
    //                }
    //            }
    //            return (value * 100).ToString("N2");
    //        }
    //        return null;
    //    }

    //    public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        double result = 0;
    //        double.TryParse(value.ToString(), out result);
    //        return new object[3] { result / 100, null, null };
    //    }
    //}

    public class StringFormatConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            VolatilityUserControl control = values[1] as VolatilityUserControl;
            if (control != null)
            {
                int index = (int)values[0];
                string format = null;
                if (control.Formats.TryGetValue(index, out format))
                {
                    return format;
                }
            }
            return "N3";
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class PriceMultiValueConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double && values[1] is double)
            {
                double spot = (double)values[0];
                double ssrate = (double)values[1];

                if (double.IsNaN(spot) == false && double.IsNaN(ssrate) == false)
                {
                    Dictionary<int, string> formats = values[3] as Dictionary<int, string>;
                    if (formats != null)
                    {
                        int index = (int)values[2];
                        string format = null;
                        if (formats.TryGetValue(index, out format))
                        {
                            return (spot + ssrate).ToString(format);
                        }
                    }

                    return (spot + ssrate).ToString("N2");
                }
            }
            return null;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class SSRateBasisTemplateSelector : DataTemplateSelector
    {
        public DataTemplate SSRateTemplate { get; set; }
        public DataTemplate BasisTemplate { get; set; }

        public override DataTemplate SelectTemplate(object item, DependencyObject container)
        {
            if (item is VolatilityItem)
            {
                VolatilityItem underlying = (VolatilityItem)item;
                if (underlying.Underlying.Type == Proto.InstrumentType.Future)
                {
                    return BasisTemplate;
                }
                else
                {
                    return SSRateTemplate;
                }
            }
            return null;
        }
    }

}
