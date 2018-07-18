using client.Models;
using client.ViewModels;
using Microsoft.Practices.Unity;
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
    /// Interaction logic for PricerSettingWindow.xaml
    /// </summary>
    public partial class PricerSettingWindow : Window
    {
        public PricerSettingWindow(PricerUserControlViewModel viewModel)
        {
            InitializeComponent();
            this.DataContext = viewModel;
            this.UnderlyingComboBox.ItemsSource = viewModel.GetHedgeUnderlyings();
        }

        private void OkButton_Click(object sender, RoutedEventArgs e)
        {
            var vm = this.DataContext as PricerUserControlViewModel;

            string name = this.NameTextBox.Text;
            if (string.IsNullOrEmpty(name))
            {
                MessageBox.Show("Name is empty", "Error");
                return;
            }
            else if (vm.IsPricerNameExisted(name))
            {
                MessageBox.Show("Name is existed", "Error");
                return;
            }

            if (this.UnderlyingComboBox.SelectedItem == null)
            {
                MessageBox.Show("Underlying is empty", "Error");
                return;
            }
            Instrument underlying = this.UnderlyingComboBox.SelectedItem as Instrument;

            if (this.ModelComboBox.SelectedItem == null)
            {
                MessageBox.Show("Model is empty", "Error");
                return;
            }
            Proto.PricingModel model = (Proto.PricingModel)this.ModelComboBox.SelectedItem;

            int interval = 0;
            if (string.IsNullOrEmpty(this.IntervalTextBox.Text) || int.TryParse(this.IntervalTextBox.Text, out interval) == false)
            {
                MessageBox.Show("Interval is illegal", "Error");
                return;
            }

            if (this.TheoTypeComboBox.SelectedItem == null)
            {
                MessageBox.Show("Underlying theo type is empty", "Error");
                return;
            }
            Proto.UnderlyingTheoType theoType = (Proto.UnderlyingTheoType)this.TheoTypeComboBox.SelectedItem;

            int maxTickChange = 0;
            if (string.IsNullOrEmpty(this.MaxTickChangeTextBox.Text) || int.TryParse(this.MaxTickChangeTextBox.Text, out maxTickChange) == false)
            {
                MessageBox.Show("Max tick change is illegal", "Error");
                return;
            }

            double elastic = 0;
            if (string.IsNullOrEmpty(this.ElasticTextBox.Text) || double.TryParse(this.ElasticTextBox.Text, out elastic) == false)
            {
                MessageBox.Show("Elastic is illegal", "Error");
                return;
            }

            double elasticLimit = 0;
            if (string.IsNullOrEmpty(this.ElasticLimitTextBox.Text) || double.TryParse(this.ElasticLimitTextBox.Text, out elasticLimit) == false)
            {
                MessageBox.Show("Elastic Limit is illegal", "Error");
                return;
            }

            vm.AddPricer(name, underlying, model, interval, theoType, maxTickChange, elastic, elasticLimit, this.OptionsCheckBox.IsChecked.Value);
            this.Close();
        }

        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
    }



    public class UnderlyingToPricerNameConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value is Instrument)
            {
                var underlying = value as Instrument;
                if (underlying != null)
                {
                    return underlying.Id + "_P";
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
