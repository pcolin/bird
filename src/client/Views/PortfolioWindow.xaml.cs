using client.ViewModels;
using Microsoft.Practices.Unity;
using System;
using System.Collections.Generic;
using System.Globalization;
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
    /// Interaction logic for PortfolioWindow.xaml
    /// </summary>
    public partial class PortfolioWindow : Window
    {
        public PortfolioWindow(IUnityContainer container)
        {
            Formats = new Dictionary<int, string>();
            InitializeComponent();
            this.DataContext = new PortfolioWindowViewModel(container, this.Dispatcher);
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
            {
                MainWindow.SaveDataGridLayout("Portfolio.xml", this.PortfolioDataGrid, Formats);
                MainWindow.WriteWindowPlacement(writer, this, "PortfolioWindow");

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

        public Dictionary<int, string> Formats { get; set; }

        private bool close = false;

        private void SettingMenuItem_Click(object sender, RoutedEventArgs e)
        {
            ColumnSettingWindow.ShowColumnSettingWindow(this, this.PortfolioDataGrid, this.Formats);
        }

        private void Window_Initialized(object sender, EventArgs e)
        {
            /// load layout
            MainWindow.LoadDataGridLayout("Portfolio.xml", this.PortfolioDataGrid, this.Formats);
        }
    }

    /// <summary>
    /// Convert Level to left margin
    /// </summary>
    internal class LevelToIndentConverter : IValueConverter
    {
        private const double IndentSize = 10.0;

        public object Convert(object o, Type type, object parameter, CultureInfo culture)
        {
            return new Thickness((int)o * IndentSize, 0, 0, 0);
        }

        public object ConvertBack(object o, Type type, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException();
        }
    }

    internal class CanExpandConverter : IValueConverter
    {
        public object Convert(object o, Type type, object parameter, CultureInfo culture)
        {
            if ((bool)o)
                return Visibility.Visible;
            else
                return Visibility.Hidden;
        }

        public object ConvertBack(object o, Type type, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException();
        }
    }
}
