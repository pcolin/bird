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
using System.Xml;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for OrderWindow.xaml
    /// </summary>
    public partial class OrderWindow : Window
    {
        public OrderWindow(IUnityContainer container)
        {
            Formats = new Dictionary<int, string>();
            InitializeComponent();
            var viewModel = new OrderWindowViewModel(container, this.Dispatcher);
            this.DataContext = viewModel;
            this.ListBoxExchanges.ItemsSource = viewModel.ExchangesView;
            this.ListBoxInstruments.ItemsSource = viewModel.InstrumentsView;
            this.ListBoxUnderlyings.ItemsSource = viewModel.UnderlyingsView;
            this.ListBoxSides.ItemsSource = viewModel.SidesView;
            this.ListBoxStatuses.ItemsSource = viewModel.StatusesView;
            this.ListBoxStrategies.ItemsSource = viewModel.StrategiesView;
            this.ListBoxTypes.ItemsSource = viewModel.TypesView;
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
            {
                MainWindow.SaveDataGridLayout("Order.xml", this.OrderDataGrid, Formats);
                MainWindow.WriteWindowPlacement(writer, this, "OrderWindow");

                this.close = true;
                this.Close();
            });
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (close == false)
            {
                this.Hide();
                e.Cancel = true;
                return;
            }
        }

        private void Window_Initialized(object sender, EventArgs e)
        {
            /// load layout
            MainWindow.LoadDataGridLayout("Order.xml", this.OrderDataGrid, this.Formats);
        }

        private void ExportMenuItem_Click(object sender, RoutedEventArgs e)
        {

        }

        private void SettingMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ColumnSettingWindow.ShowColumnSettingWindow(this, this.OrderDataGrid, this.Formats);
        }

        public Dictionary<int, string> Formats { get; set; }
        private bool close = false;
        
    }
}
