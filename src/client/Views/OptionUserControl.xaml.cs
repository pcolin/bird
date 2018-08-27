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
using System.Xml;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for OptionUserControl.xaml
    /// </summary>
    public partial class OptionUserControl : UserControl
    {
        public OptionUserControl(OptionUserControlViewModel vm)
        {
            UnderlyingFormats = new Dictionary<int, string>();
            OptionFormats = new Dictionary<int, string>();
            this.DataContext = vm;
            InitializeComponent();
        }

        public void SaveLayout()
        {
            var vm = this.DataContext as OptionUserControlViewModel;
            string name = "Options_" + vm.Underlying + "_Underlyings.xml";
            MainWindow.SaveDataGridLayout(name, this.UnderlyingDataGrid, this.UnderlyingFormats);

            name = "Options_" + vm.Underlying + "_Options.xml";
            MainWindow.SaveDataGridLayout(name, this.OptionDataGrid, this.OptionFormats);
        }

        public Dictionary<int, string> UnderlyingFormats { get; set; }
        public Dictionary<int, string> OptionFormats { get; set; }

        private void OptionDataGrid_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.OemPlus)
            {
                HashSet<OptionItem>[] items = new HashSet<OptionItem>[(int)Proto.StrategyType.DummyQuoter + 1]
                    {
                        new HashSet<OptionItem>(),
                        new HashSet<OptionItem>(),
                        new HashSet<OptionItem>(),
                        new HashSet<OptionItem>(),
                    };
                foreach (var cell in this.OptionDataGrid.SelectedCells)
                {
                    if (cell.Column == this.QBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.QuoterBidOn)
                        {
                            item.Call.QuoterBidOn = true;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.QSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.QuoterAskOn)
                        {
                            item.Call.QuoterAskOn = true;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.MBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.DummyQuoterBidOn)
                        {
                            item.Call.DummyQuoterBidOn = true;
                            items[(int)Proto.StrategyType.DummyQuoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.MSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.DummyQuoterAskOn)
                        {
                            item.Call.DummyQuoterAskOn = true;
                            items[(int)Proto.StrategyType.DummyQuoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.HBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.HitterBidOn)
                        {
                            item.Call.HitterBidOn = true;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.HSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.HitterAskOn)
                        {
                            item.Call.HitterAskOn = true;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.DBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.DimerBidOn)
                        {
                            item.Call.DimerBidOn = true;
                            items[(int)Proto.StrategyType.Dimer].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.DSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.DimerAskOn)
                        {
                            item.Call.DimerAskOn = true;
                            items[(int)Proto.StrategyType.Dimer].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.CoverC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.CoverOn)
                        {
                            item.Call.CoverOn = true;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Call);
                            items[(int)Proto.StrategyType.Dimer].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.QRC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Call.EnquiryResponseOn)
                        {
                            item.Call.EnquiryResponseOn = true;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.CoverP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.CoverOn)
                        {
                            item.Put.CoverOn = true;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Put);
                            items[(int)Proto.StrategyType.Dimer].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.QBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.QuoterBidOn)
                        {
                            item.Put.QuoterBidOn = true;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.QSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.QuoterAskOn)
                        {
                            item.Put.QuoterAskOn = true;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.MBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.DummyQuoterBidOn)
                        {
                            item.Put.DummyQuoterBidOn = true;
                            items[(int)Proto.StrategyType.DummyQuoter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.MSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.DummyQuoterAskOn)
                        {
                            item.Put.DummyQuoterAskOn = true;
                            items[(int)Proto.StrategyType.DummyQuoter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.HBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.HitterBidOn)
                        {
                            item.Put.HitterBidOn = true;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.HSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.HitterAskOn)
                        {
                            item.Put.HitterAskOn = true;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.DBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.DimerBidOn)
                        {
                            item.Put.DimerBidOn = true;
                            items[(int)Proto.StrategyType.Dimer].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.DSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.DimerAskOn)
                        {
                            item.Put.DimerAskOn = true;
                            items[(int)Proto.StrategyType.Dimer].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.QRP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (!item.Put.EnquiryResponseOn)
                        {
                            item.Put.EnquiryResponseOn = true;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Put);
                        }
                    }
                }
                (this.DataContext as OptionUserControlViewModel).SetStrategySwitch(items);
            }
            else if (e.Key == Key.OemMinus)
            {
                HashSet<OptionItem>[] items = new HashSet<OptionItem>[(int)Proto.StrategyType.DummyQuoter + 1]
                    {
                        new HashSet<OptionItem>(),
                        new HashSet<OptionItem>(),
                        new HashSet<OptionItem>(),
                        new HashSet<OptionItem>(),
                    };
                foreach (var cell in this.OptionDataGrid.SelectedCells)
                {
                    if (cell.Column == this.QBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.QuoterBidOn)
                        {
                            item.Call.QuoterBidOn = false;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.QSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.QuoterAskOn)
                        {
                            item.Call.QuoterAskOn = false;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.MBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.DummyQuoterBidOn)
                        {
                            item.Call.DummyQuoterBidOn = false;
                            items[(int)Proto.StrategyType.DummyQuoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.MSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.DummyQuoterAskOn)
                        {
                            item.Call.DummyQuoterAskOn = false;
                            items[(int)Proto.StrategyType.DummyQuoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.HBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.HitterBidOn)
                        {
                            item.Call.HitterBidOn = false;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.HSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.HitterAskOn)
                        {
                            item.Call.HitterAskOn = false;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.DBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.DimerBidOn)
                        {
                            item.Call.DimerBidOn = false;
                            items[(int)Proto.StrategyType.Dimer].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.DSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.DimerAskOn)
                        {
                            item.Call.DimerAskOn = false;
                            items[(int)Proto.StrategyType.Dimer].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.CoverC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.CoverOn)
                        {
                            item.Call.CoverOn = false;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Call);
                            items[(int)Proto.StrategyType.Dimer].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.QRC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Call.EnquiryResponseOn)
                        {
                            item.Call.EnquiryResponseOn = false;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Call);
                        }
                    }
                    else if (cell.Column == this.CoverP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.CoverOn)
                        {
                            item.Put.CoverOn = false;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Put);
                            items[(int)Proto.StrategyType.Dimer].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.QBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.QuoterBidOn)
                        {
                            item.Put.QuoterBidOn = false;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.QSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.QuoterAskOn)
                        {
                            item.Put.QuoterAskOn = false;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.MBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.DummyQuoterBidOn)
                        {
                            item.Put.DummyQuoterBidOn = false;
                            items[(int)Proto.StrategyType.DummyQuoter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.MSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.DummyQuoterAskOn)
                        {
                            item.Put.DummyQuoterAskOn = false;
                            items[(int)Proto.StrategyType.DummyQuoter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.HBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.HitterBidOn)
                        {
                            item.Put.HitterBidOn = false;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.HSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.HitterAskOn)
                        {
                            item.Put.HitterAskOn = false;
                            items[(int)Proto.StrategyType.Hitter].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.DBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.DimerBidOn)
                        {
                            item.Put.DimerBidOn = false;
                            items[(int)Proto.StrategyType.Dimer].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.DSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.DimerAskOn)
                        {
                            item.Put.DimerAskOn = false;
                            items[(int)Proto.StrategyType.Dimer].Add(item.Put);
                        }
                    }
                    else if (cell.Column == this.QRP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        if (item.Put.EnquiryResponseOn)
                        {
                            item.Put.EnquiryResponseOn = false;
                            items[(int)Proto.StrategyType.Quoter].Add(item.Put);
                        }
                    }
                }
                (this.DataContext as OptionUserControlViewModel).SetStrategySwitch(items);
            }
            else if (e.Key == Key.D)
            {
                double? value = null;
                Proto.DestrikerReq req = new Proto.DestrikerReq();
                foreach (var cell in this.OptionDataGrid.SelectedCells)
                {
                    if (cell.Column == this.DestrikerC)
                    {
                        if (!value.HasValue)
                        {
                            value = ShowInputBox("destriker");
                            if (!value.HasValue) break;
                        }
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.Destriker = value.Value / 1000.0;
                        req.Destrikers.Add(new Proto.Destriker()
                            {
                                Instrument = item.Call.Option.Id,
                                Destriker_ = item.Call.Destriker,
                            });
                    }
                    else if (cell.Column == this.DestrikerP)
                    {
                        if (!value.HasValue)
                        {
                            value = ShowInputBox("destriker");
                            if (!value.HasValue) break;
                        }
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.Destriker = value.Value / 1000.0;
                        req.Destrikers.Add(new Proto.Destriker()
                        {
                            Instrument = item.Put.Option.Id,
                            Destriker_ = item.Call.Destriker,
                        });
                    }
                }
                if (value.HasValue)
                {
                    var vm = this.DataContext as OptionUserControlViewModel;
                    vm.SetDestrikers(req);
                }
            }
            else if (e.Key == Key.V)
            {
                double? value = null;
                foreach (var cell in this.OptionDataGrid.SelectedCells)
                {
                    if (cell.Column == this.VolC)
                    {
                        if (value == null)
                        {
                            value = ShowInputBox("volatility");
                            if (value == null) break;
                        }
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.Volatility = value.Value;
                    }
                    else if (cell.Column == this.VolP)
                    {
                        if (value == null)
                        {
                            value = ShowInputBox("volatility");
                            if (value == null) break;
                        }

                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.Volatility = value.Value;
                    }
                }
            }
        }

        private void UnderlyingMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ColumnSettingWindow.ShowColumnSettingWindow(this, this.UnderlyingDataGrid, this.UnderlyingFormats);
        }

        private void OptionMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ColumnSettingWindow.ShowColumnSettingWindow(this, this.OptionDataGrid, this.OptionFormats);
        }

        private double? ShowInputBox(string name)
        {
            InputBoxWindow w = new InputBoxWindow(name);
            w.Owner = Window.GetWindow(this);
            w.ShowInTaskbar = false;
            var ret = w.ShowDialog();
            if (ret != null && ret.Value)
            {
                return w.Value;
            }
            return null;
        }

        private void UserControl_Initialized(object sender, EventArgs e)
        {
            /// load layout
            var vm = this.DataContext as OptionUserControlViewModel;
            if (vm != null)
            {
                string name = "Options_" + vm.Underlying + "_Underlyings.xml";
                MainWindow.LoadDataGridLayout(name, this.UnderlyingDataGrid, this.UnderlyingFormats);

                name = "Options_" + vm.Underlying + "_Options.xml";
                MainWindow.LoadDataGridLayout(name, this.OptionDataGrid, this.OptionFormats);
            }
        }
    }

    //public class DoubleFormatConverter : IMultiValueConverter
    //{
    //    public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        double value = (double)values[1];
    //        if (value != 0.0)
    //        {
    //            OptionUserControl control = values[3] as OptionUserControl;
    //            if (control != null)
    //            {
    //                Proto.InstrumentType type = (Proto.InstrumentType)values[0];
    //                int index = (int)values[2];
    //                string format = null;
    //                if (type == Proto.InstrumentType.Option)
    //                {
    //                    if (control.OptionFormats.TryGetValue(index, out format))
    //                    {
    //                        return value.ToString(format);
    //                    }
    //                }
    //                else
    //                {
    //                    if (control.UnderlyingFormats.TryGetValue(index, out format))
    //                    {
    //                        return value.ToString(format);
    //                    }
    //                }
    //            }
    //            return value.ToString("N2");
    //        }
    //        else
    //        {
    //            return String.Empty;
    //        }
    //    }

    //    public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        throw new NotImplementedException();
    //    }
    //}

    //public class PercentDoubleFormatConverter : IMultiValueConverter
    //{
    //    public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        double value = (double)values[1];
    //        if (value != 0.0)
    //        {
    //            OptionUserControl control = values[3] as OptionUserControl;
    //            if (control != null)
    //            {
    //                Proto.InstrumentType type = (Proto.InstrumentType)values[0];
    //                int index = (int)values[2];
    //                string format = null;
    //                if (type == Proto.InstrumentType.Option)
    //                {
    //                    if (control.OptionFormats.TryGetValue(index, out format))
    //                    {
    //                        return (value * 100).ToString(format);
    //                    }
    //                }
    //                else
    //                {
    //                    if (control.UnderlyingFormats.TryGetValue(index, out format))
    //                    {
    //                        return (value * 100).ToString(format);
    //                    }
    //                }
    //            }
    //            return (value * 100).ToString("N2");
    //        }
    //        else
    //        {
    //            return String.Empty;
    //        }
    //    }

    //    public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        throw new NotImplementedException();
    //    }
    //}

    public class BackgroundConverter : IValueConverter
    {
        //Brush[] colors = new Brush[] { Brushes.LightGreen, Brushes.LightSalmon };

        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int groupNo = (int)value;
            return groupNo % 2 == 0 ? Brushes.LightGreen : Brushes.LightSalmon;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class BoolToColorConverter : IValueConverter
    {
        public Brush TrueValueBrush { get; set; }
        public Brush FalseValueBrush { get; set; }

        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            bool isMeetObligation = (bool)value;
            return isMeetObligation ? TrueValueBrush : FalseValueBrush;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }


    public class MultiBoolToColorConverter : IMultiValueConverter
    {
        public Brush Brush1 { get; set; }
        public Brush Brush2 { get; set; }
        public Brush Brush3 { get; set; }

        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if ((bool)values[0])
            {
                return Brush1;
            }
            else if ((bool)values[1])
            {
                return Brush2;
            }
            return Brush3;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class PositionColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int pos = (int)value;
            if (pos > 0)
            {
                return Brushes.Red;
            }
            else if (pos < 0)
            {
                return Brushes.Green;
            }
            else
            {
                return Brushes.Black;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ThicknessConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            bool isFirst = (bool)value;
            return isFirst ? "0, 2, 0, 0" : "0, 0, 0, 0";
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class OnConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            bool on = (bool)value;
            if (on)
            {
                return "√";
            }
            else
            {
                return "";
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            string on = value.ToString();
            return on == "√";
        }
    }

    public class TheoCrossColorConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double && values[1] is double && values[2] is double)
            {
                double theo = (double)values[0];
                double bid = (double)values[1];
                double ask = (double)values[2];

                if (bid > 0 && ask > 0)
                {
                    if (theo < bid)
                    {
                        return Brushes.Red;
                    }
                    else if (theo > ask)
                    {
                        return Brushes.Green;
                    }
                }
            }
            return Brushes.Black;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class TheoCrossFontWeightConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double && values[1] is double && values[2] is double)
            {
                double theo = (double)values[0];
                double bid = (double)values[1];
                double ask = (double)values[2];

                if (bid > 0 && ask > 0)
                {
                    if (theo < bid || theo > ask)
                    {
                        return FontWeights.Bold;
                    }
                }
            }
            return FontWeights.Normal;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class PricesToColorMultiConverter : IMultiValueConverter
    {
        public Brush DefaultBrush { get; set; }
        public Brush WarningBrush { get; set; }

        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double && values[1] is double)
            {
                double value1 = (double)values[0];
                double value2 = (double)values[1];

                if (value1 == value2 && value1 > 0)
                {
                    return WarningBrush;
                }
            }
            return DefaultBrush;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class PricesToFontWeightMultiConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double && values[1] is double)
            {
                double value1 = (double)values[0];
                double value2 = (double)values[1];

                if (value1 == value2 && value1 > 0)
                {
                    return FontWeights.Bold;
                }
            }
            return FontWeights.Normal;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NetToColorMultiConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double && values[1] is double)
            {
                double value1 = (double)values[0];
                double value2 = (double)values[1];

                if (value1 > value2)
                {
                    return Brushes.Red;
                }
                else if (value1 < value2)
                {
                    return Brushes.Green;
                }
            }
            return null;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    
    public class NetPercentFormatToStringMultiConverter : IMultiValueConverter
    {
        public string DefaultFormat { get; set; }

        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double && values[1] is double)
            {
                double value1 = (double)values[0];
                double value2 = (double)values[1];
                if (double.IsNaN(value1) == false && double.IsNaN(value2) == false && value2 != 0)
                {
                    double value = value1 / value2 - 1;
                    Dictionary<int, string> formats = values[3] as Dictionary<int, string>;
                    if (formats != null)
                    {
                        int index = (int)values[2];
                        string format = null;
                        if (formats.TryGetValue(index, out format))
                        {
                            return (value * 100).ToString(format);
                        }
                    }
                    return (value * 100).ToString(DefaultFormat);
                }
            }
            return null;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class WarningPositionToColorMultiConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int value1 = (int)values[0];
            int value2 = (int)values[1];
            
            if (value1 >= value2 * 0.8)
            {
                return Brushes.DarkRed;
            }
            else if (value1 >= value2 * 0.5)
            {
                return Brushes.Orange;
            }
            return null;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    

    //public class ColorConverter : IValueConverter
    //{
    //    private string defaultColor;

    //    public string DefaultColor
    //    {
    //        get { return defaultColor; }
    //        set
    //        {
    //            defaultColor = value;
    //            LastColor = defaultColor;
    //        }
    //    }

    //    public string AlternateColor { get; set; }
    //    private string LastMaturity { get; set; }
    //    private string LastColor { get; set; }

    //    public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        string maturity = (string)value;
    //        if (maturity == LastMaturity)
    //        {
    //            if (LastColor == null)
    //            {
    //                LastColor = DefaultColor;
    //            }
    //        }
    //        else
    //        {
    //            if (LastColor == null)
    //            {
    //                LastColor = DefaultColor;
    //            }
    //            else if (LastColor == DefaultColor)
    //            {
    //                LastColor = AlternateColor;
    //            }
    //            else
    //            {
    //                LastColor = DefaultColor;
    //            }
    //        }
    //        LastMaturity = maturity;
    //        return LastColor;
    //    }

    //    public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
    //    {
    //        throw new NotImplementedException();
    //    }
    //}
}
