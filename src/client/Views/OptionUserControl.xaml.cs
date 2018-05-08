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
            this.DataContext = vm;
            InitializeComponent();

            UnderlyingFormats = new Dictionary<int, string>();
            OptionFormats = new Dictionary<int, string>();
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
                foreach (var cell in this.OptionDataGrid.SelectedCells)
                {
                    if (cell.Column == this.QBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.QuoterBidOn = true;
                    }
                    else if (cell.Column == this.QSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.QuoterAskOn = true;
                    }
                    else if (cell.Column == this.MBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.DummyQuoterBidOn = true;
                    }
                    else if (cell.Column == this.MSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.DummyQuoterAskOn = true;
                    }
                    else if (cell.Column == this.HBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.HitterBidOn = true;                        
                    }
                    else if (cell.Column == this.HSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.HitterAskOn = true;
                    }
                    else if (cell.Column == this.DBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.DimerBidOn = true;                        
                    }
                    else if (cell.Column == this.DSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.DimerAskOn = true;
                    }
                    else if (cell.Column == this.CoverC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.CoverOn = true;
                    }
                    else if (cell.Column == this.QRC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.EnquiryResponseOn = true;
                    }
                    else if (cell.Column == this.CoverP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.CoverOn = true;
                    }
                    else if (cell.Column == this.QBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.QuoterBidOn = true;
                    }
                    else if (cell.Column == this.QSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.QuoterAskOn = true;
                    }
                    else if (cell.Column == this.MBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.DummyQuoterBidOn = true;
                    }
                    else if (cell.Column == this.MSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.DummyQuoterAskOn = true;
                    }
                    else if (cell.Column == this.HBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.HitterBidOn = true;
                    }
                    else if (cell.Column == this.HSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.HitterAskOn = true;
                    }
                    else if (cell.Column == this.DBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.DimerBidOn = true;
                    }
                    else if (cell.Column == this.DSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.DimerAskOn = true;
                    }
                    else if (cell.Column == this.QRP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.EnquiryResponseOn = true;
                    }
                }
            }
            else if (e.Key == Key.OemMinus)
            {
                foreach (var cell in this.OptionDataGrid.SelectedCells)
                {
                    if (cell.Column == this.QBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.QuoterBidOn = false;
                    }
                    else if (cell.Column == this.QSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.QuoterAskOn = false;
                    }
                    else if (cell.Column == this.MBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.DummyQuoterBidOn = false;
                    }
                    else if (cell.Column == this.MSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.DummyQuoterAskOn = false;
                    }
                    else if (cell.Column == this.HBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.HitterBidOn = false;
                    }
                    else if (cell.Column == this.HSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.HitterAskOn = false;
                    }
                    else if (cell.Column == this.DBC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.DimerBidOn = false;
                    }
                    else if (cell.Column == this.DSC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.DimerAskOn = false;
                    }
                    else if (cell.Column == this.CoverC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.CoverOn = false;
                    }
                    else if (cell.Column == this.QRC)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.EnquiryResponseOn = false;
                    }
                    else if (cell.Column == this.CoverP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.CoverOn = false;
                    }
                    else if (cell.Column == this.QBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.QuoterBidOn = false;
                    }
                    else if (cell.Column == this.QSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.QuoterAskOn = false;
                    }
                    else if (cell.Column == this.MBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.DummyQuoterBidOn = false;
                    }
                    else if (cell.Column == this.MSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.DummyQuoterAskOn = false;
                    }
                    else if (cell.Column == this.HBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.HitterBidOn = false;
                    }
                    else if (cell.Column == this.HSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.HitterAskOn = false;
                    }
                    else if (cell.Column == this.DBP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.DimerBidOn = false;
                    }
                    else if (cell.Column == this.DSP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.DimerAskOn = false;
                    }
                    else if (cell.Column == this.QRP)
                    {
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.EnquiryResponseOn = false;
                    }
                }
            }
            else if (e.Key == Key.D)
            {
                double? value = null;
                foreach (var cell in this.OptionDataGrid.SelectedCells)
                {
                    if (cell.Column == this.DestrikerC)
                    {
                        if (value == null)
                        {
                            value = ShowInputBox("destriker");
                            if (value == null) break;
                        }
                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Call.Destriker = value.Value;
                    }
                    else if (cell.Column == this.DestrikerP)
                    {
                        if (value == null)
                        {
                            value = ShowInputBox("destriker");
                            if (value == null) break;
                        }

                        OptionPairItem item = cell.Item as OptionPairItem;
                        item.Put.Destriker = value.Value;
                    }
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
            ColumnSettingWindow setting = new ColumnSettingWindow(this.UnderlyingDataGrid, this.UnderlyingFormats);
            setting.Show();
        }

        private void OptionMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ColumnSettingWindow setting = new ColumnSettingWindow(this.OptionDataGrid, this.OptionFormats);
            setting.Show();
        }

        private double? ShowInputBox(string name)
        {
            InputBoxWindow w = new InputBoxWindow("destriker");
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

    public class DoubleFormatConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            double value = (double)values[1];
            if (value != 0.0)
            {
                OptionUserControl control = values[3] as OptionUserControl;
                if (control != null)
                {
                    Proto.InstrumentType type = (Proto.InstrumentType)values[0];
                    int index = (int)values[2];
                    string format = null;
                    if (type == Proto.InstrumentType.Option)
                    {
                        if (control.OptionFormats.TryGetValue(index, out format))
                        {
                            return value.ToString(format);
                        }
                    }
                    else
                    {
                        if (control.UnderlyingFormats.TryGetValue(index, out format))
                        {
                            return value.ToString(format);
                        }
                    }
                }
                return value.ToString("N2");
            }
            else
            {
                return String.Empty;
            }
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class PercentDoubleFormatConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            double value = (double)values[1];
            if (value != 0.0)
            {
                OptionUserControl control = values[3] as OptionUserControl;
                if (control != null)
                {
                    Proto.InstrumentType type = (Proto.InstrumentType)values[0];
                    int index = (int)values[2];
                    string format = null;
                    if (type == Proto.InstrumentType.Option)
                    {
                        if (control.OptionFormats.TryGetValue(index, out format))
                        {
                            return (value * 100).ToString(format);
                        }
                    }
                    else
                    {
                        if (control.UnderlyingFormats.TryGetValue(index, out format))
                        {
                            return (value * 100).ToString(format);
                        }
                    }
                }
                return (value * 100).ToString("N2");
            }
            else
            {
                return String.Empty;
            }
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class BackgroundConverter : IValueConverter
    {
        string[] colors = new string[] { "LightGreen", "LightSalmon" };

        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int groupNo = (int)value;
            return colors[groupNo % 2];
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

    public class PositionColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int pos = (int)value;
            if (pos > 0)
                return "Red";
            else if (pos < 0)
                return "Green";
            return string.Empty;
            //return pos > 0 ? "Red" : "";
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
