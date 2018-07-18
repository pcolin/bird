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
using System.Windows.Shapes;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for QuoterSettingWindow.xaml
    /// </summary>
    public partial class QuoterSettingWindow : Window
    {
        public QuoterSettingWindow(QuoterUserControlViewModel viewModel)
        {
            InitializeComponent();
            this.DataContext = viewModel;
            this.PricerComboBox.ItemsSource = viewModel.GetPricers();
        }

        private void OkButton_Click(object sender, RoutedEventArgs e)
        {
            var vm = this.DataContext as QuoterUserControlViewModel;

            string name = this.NameTextBox.Text;
            if (string.IsNullOrEmpty(name))
            {
                MessageBox.Show("Name is empty", "Error");
                return;
            }
            else if (vm.IsQuoterNameExisted(name))
            {
                MessageBox.Show("Name is existed", "Error");
                return;
            }

            if (this.PricerComboBox.SelectedItem == null)
            {
                MessageBox.Show("Pricer is empty", "Error");
                return;
            }
            var pricer = this.PricerComboBox.SelectedItem as Proto.Pricer;

            double deltaLimit = 0;
            if (string.IsNullOrEmpty(this.DeltaLimitTextBox.Text) || double.TryParse(this.DeltaLimitTextBox.Text, out deltaLimit) == false)
            {
                MessageBox.Show("DeltaLimit is illegal", "Error");
                return;
            }

            int orderLimit = 0;
            if (string.IsNullOrEmpty(this.OrderLimitTextBox.Text) || int.TryParse(this.OrderLimitTextBox.Text, out orderLimit) == false)
            {
                MessageBox.Show("OrderLimit is illegal", "Error");
                return;
            }

            int tradeLimit = 0;
            if (string.IsNullOrEmpty(this.TradeLimitTextBox.Text) || int.TryParse(this.TradeLimitTextBox.Text, out tradeLimit) == false)
            {
                MessageBox.Show("TradeLimit is illegal", "Error");
                return;
            }

            int bidVolume = 0;
            if (string.IsNullOrEmpty(this.BidVolumeTextBox.Text) || int.TryParse(this.BidVolumeTextBox.Text, out bidVolume) == false)
            {
                MessageBox.Show("BidVolume is illegal", "Error");
                return;
            }

            int askVolume = 0;
            if (string.IsNullOrEmpty(this.AskVolumeTextBox.Text) || int.TryParse(this.AskVolumeTextBox.Text, out askVolume) == false)
            {
                MessageBox.Show("AskVolume is illegal", "Error");
                return;
            }

            int qrVolume = 0;
            if (string.IsNullOrEmpty(this.QRVolumeTextBox.Text) || int.TryParse(this.QRVolumeTextBox.Text, out qrVolume) == false)
            {
                MessageBox.Show("QRVolume is illegal", "Error");
                return;
            }

            int maxDepth = 0;
            if (string.IsNullOrEmpty(this.MaxDepthTextBox.Text) || int.TryParse(this.MaxDepthTextBox.Text, out maxDepth) == false)
            {
                MessageBox.Show("MaxDepth is illegal", "Error");
                return;
            }

            int refillTimes = 0;
            if (string.IsNullOrEmpty(this.RefillTimesTextBox.Text) || int.TryParse(this.RefillTimesTextBox.Text, out refillTimes) == false)
            {
                MessageBox.Show("RefillTimes is illegal", "Error");
                return;
            }

            if (this.WideSpreadComboBox.SelectedItem == null)
            {
                MessageBox.Show("WideSpread is empty", "Error");
                return;
            }
            bool wideSpread = (bool)this.WideSpreadComboBox.SelectedItem;

            if (this.ProtectionComboBox.SelectedItem == null)
            {
                MessageBox.Show("Protection is empty", "Error");
                return;
            }
            bool isProtection = (bool)this.ProtectionComboBox.SelectedItem;

            var quoter = new Proto.QuoterSpec()
                {
                    Name = name,
                    Pricer = pricer.Name,
                    Underlying = pricer.Underlying,
                    DeltaLimit = deltaLimit,
                    OrderLimit = orderLimit,
                    TradeLimit = tradeLimit,
                    BidVolume = bidVolume,
                    AskVolume = askVolume,
                    ResponseVolume = qrVolume,
                    Depth = maxDepth,
                    RefillTimes = refillTimes,
                    WideSpread = wideSpread,
                    Protection = isProtection,
                };

            vm.AddQuoter(pricer, quoter, this.OptionsCheckBox.IsChecked.Value);
            this.Close();
        }

        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
    }

    public class UnderlyingToQuoterNameConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value is Proto.Pricer)
            {
                var pricer = value as Proto.Pricer;
                if (pricer != null)
                {
                    return pricer.Underlying + "_Q";
                }
            }
            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
