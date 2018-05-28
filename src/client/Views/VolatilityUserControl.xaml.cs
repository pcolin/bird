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

namespace client.Views
{
    /// <summary>
    /// Interaction logic for VolatilityUserControl.xaml
    /// </summary>
    public partial class VolatilityUserControl : UserControl
    {
        public VolatilityUserControl()
        {
            InitializeComponent();
        }

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
                        vcr = item.VCR,
                        scr = item.SCR,
                        ccr = item.CCR,
                        spcr = item.SPCR,
                        sccr = item.SCCR
                    };
                var rm = vm.Container.Resolve<InterestRateManager>(vm.Exchange.ToString());
                if (rm != null)
                {
                    rate = rm.GetInterestRate(item.Maturity).Value;
                }

                //var sm = vm.Container.Resolve<SSRateManager>(vm.Exchange.ToString());
                //if (sm != null)
                //{
                //    ssrate = sm.GetSSRate(item.Underlying.Id, item.Maturity).Value;
                //}

                var em = vm.Container.Resolve<ExchangeParameterManager>(vm.Exchange.ToString());
                if (em != null)
                {
                    timeValue = em.GetTimeValue(item.Maturity).Value;
                }
            }

            this.UnderlyingMaturity.Text = item.Underlying.Id + "@" + item.Maturity.ToString("yyyyMMdd");

            var pm = vm.Container.Resolve<ProductManager>(vm.Exchange.ToString());
            var options = pm.GetOptionsByHedgeUnderlying(item.Underlying, item.Maturity);
            var calculator = vm.Container.Resolve<TheoCalculator>(vm.Exchange.ToString());

            var strikers = new List<double>();
            var oldIV = new List<double>();
            var newIV = new List<double>();
            var callBidIV = new List<double>();
            var callAskIV = new List<double>();
            var putBidIV = new List<double>();
            var putAskIV = new List<double>();
            foreach (var op in options)
            {
                if (op.OptionType == Proto.OptionType.Call)
                {
                    strikers.Add(op.Strike);
                    ImpliedVolatilityData iv = calculator.GetIV(op);
                    if (iv != null)
                    {
                        callBidIV.Add(iv.BidIV * 100);
                        callAskIV.Add(iv.AskIV * 100);
                    }
                    else
                    {
                        callBidIV.Add(0);
                        callAskIV.Add(0);
                    }
                    
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
                        putBidIV.Add(iv.BidIV * 100);
                        putAskIV.Add(iv.AskIV * 100);
                    }
                    else
                    {
                        putBidIV.Add(0);
                        putAskIV.Add(0);
                    }
                }
            }

            this.CallBidIVGraph.Plot(strikers, callBidIV);
            this.CallAskIVGraph.Plot(strikers, callBidIV);
            this.PutBidIVGraph.Plot(strikers, putBidIV);
            this.PutAskIVGraph.Plot(strikers, putAskIV);

            this.OldVolGraph.Plot(strikers, oldIV);
            if (item.Modified)
            {
                this.NewVolGraph.Visibility = System.Windows.Visibility.Visible;
                this.NewVolGraph.Plot(strikers, newIV);
                this.VolGraph.Plot(strikers, newIV);
            }
            else
            {
                this.NewVolGraph.Visibility = System.Windows.Visibility.Collapsed;
                this.VolGraph.Plot(strikers, oldIV);
            }
        }

        private VolatilityModelWrapper model = new VolatilityModelWrapper();
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

    public class PriceMultiValueConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            double spot = (double)values[0];
            double ssrate = (double)values[1];
            return (spot + ssrate).ToString("F2");
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
