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
    /// Interaction logic for TradeWindow.xaml
    /// </summary>
    public partial class TradeWindow : Window
    {
        public TradeWindow(IUnityContainer container)
        {
            Formats = new Dictionary<int, string>();
            InitializeComponent();
            this.DataContext = new TradeWindowViewModel(container, this.Dispatcher);
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
            {
                MainWindow.SaveDataGridLayout("Trade.xml", this.TradeDataGrid, Formats);
                MainWindow.WriteWindowPlacement(writer, this, "TradeWindow");

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
            MainWindow.LoadDataGridLayout("Trade.xml", this.TradeDataGrid, this.Formats);
        }

        public Dictionary<int, string> Formats { get; set; }
        private bool close = false;
    }
}
